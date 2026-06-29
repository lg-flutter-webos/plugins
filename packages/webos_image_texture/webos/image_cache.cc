// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#include <filesystem>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <tuple>
#include <functional>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <curl/curl.h>
#include "log.h"
#include "image_factory.h"
#include "image_cache.h"
#include "third_party/etcpak/Application.hpp"

#ifdef TAG
#undef TAG
#endif
#define TAG "ImageCache::"

#define HTTP 0
#define kFILE 1

#define OK 0
#define FAIL -1
#define NO_NETWORK -2

static const float MB = 1024.*1024.;
static const int64_t NETWORK_CONNECTION_TIMEOUT_SEC = 1;
static const int64_t NETWORK_TIMEOUT_MS = 500;

sqlite3* ImageCache::db_[2] = { nullptr, nullptr};
std::map<std::string,ImageCache::Info> ImageCache::infos_[2];
std::map<std::string,bool> ImageCache::invalid_urls_;
size_t ImageCache::size_[2] = { 0, 0 };
size_t ImageCache::max_size_[2] = { (size_t)(20*MB), (size_t)(8*MB) };
bool ImageCache::request_clear_ = false;
std::recursive_mutex ImageCache::mutex_;

//////////////////////////////////////////////////////
//                 Common Funcs                     //
//////////////////////////////////////////////////////
static std::string cut_url(const std::string& url) {
  size_t len = url.length();
  return len > 128
      ? "..." + url.substr(len - 125)
      : url;
}

static std::string int64_to_hexstring(int64_t i) {
  std::stringstream stream;
  stream << "0x"
         << std::setfill ('0') << std::setw(sizeof(int64_t)*2)
         << std::hex << i;
  return stream.str();
}

static int64_t hexstring_to_int64(const std::string& hexstring) {
  if (hexstring.empty()) {
    LogWarn(TAG) << __func__ << " error";
    return 0;
  }
  return ::strtoll(hexstring.c_str(), NULL, 16);
}

static int64_t get_utc() {
  struct timeval val;
  ::gettimeofday(&val,NULL);

  int64_t sec = val.tv_sec;
  int64_t msec = val.tv_usec/1000;
  return sec*1000 + msec;
}

static int64_t getCurTick() {
  auto var0 = std::chrono::steady_clock::now();
  auto var1 = std::chrono::time_point_cast<std::chrono::milliseconds>(var0).time_since_epoch();
  int64_t msec = std::chrono::duration_cast<std::chrono::milliseconds>(var1).count();
  return msec;
}

static int64_t getDiffTick(int64_t t0) {
  int64_t t1 = getCurTick();
  return std::abs(t1-t0);
}

static void removeTempFiles() {
  const char *appIdEnv = ::getenv("FLUTTER_APP_ID");
  const char *tempHomeEnv = ::getenv("FLUTTER_TEMP_HOME");
  if (!appIdEnv || !tempHomeEnv) {
    return;
  }
  std::string appId = appIdEnv;
  // For HTTP
  std::string path = tempHomeEnv;
  path += "/";
  path += appId;
  std::filesystem::create_directories(path);

  path += "/tmp";
  std::filesystem::create_directories(path);
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    std::filesystem::remove_all(entry.path());
  }

  // For File
  path = "/tmp";
  path += "/";
  path += appId;
  std::filesystem::create_directories(path);
  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    std::filesystem::remove_all(entry.path());
  }
}

static std::string getTempFilePath(const std::string& url) {
  int indx = (url.find("http") == 0) ? HTTP : kFILE;
  const char *appIdEnv = ::getenv("FLUTTER_APP_ID");
  if (!appIdEnv) return std::string();
  std::string appId = appIdEnv;
  std::string path;
  if (indx == HTTP) {
    const char *tempHomeEnv = ::getenv("FLUTTER_TEMP_HOME");
    if (!tempHomeEnv) return std::string();
    path = tempHomeEnv;
    path += "/";
    path += appId;
    path += "/tmp";
  } else {
    path = "/tmp";
    path += "/";
    path += appId;
  }

  std::stringstream ss;
  ss << std::hash<std::string>{}(url);
  std::string fname = ss.str();

  path += "/";
  path += fname;
  return path;
}


//////////////////////////////////////////////////////
//                 Set_maxbytes                     //
//////////////////////////////////////////////////////
void ImageCache::set_maxbytes(size_t bytes) {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);

  max_size_[HTTP] = bytes;
  LogInfo(TAG) << "maximum network cache size: " << bytes/MB << " MB";
}

//////////////////////////////////////////////////////
//                 Open                             //
//////////////////////////////////////////////////////
int ImageCache::onOpen(
    void *,
    int argc,
    char **argv,
    char **azColName) {
  std::string url;
  Info info = {0,0,0,0,"",0,0};
  for (int i = 0; i < argc; i++) {
    std::string name = azColName[i];
    std::string value = argv[i];
    if (name == "Url") url = value;
    else if (name == "Utc") info.utc = hexstring_to_int64(value);
    else if (name == "DataSize") info.size = std::stoi(value);
    else if (name == "ContentLength") info.content_length = std::stoi(value);
    else if (name == "Etag") info.etag = value;
    else if (name == "Width") info.width = std::stoi(value);
    else if (name == "Height") info.height = std::stoi(value);
  }

  if (!url.empty()) {
    infos_[HTTP][url] = info;
    LogDebug(TAG) << "UTC: " << int64_to_hexstring(info.utc) << ","
                  << "data_size: " << info.size << ","
                  << "content_length: " << info.content_length << ","
                  << "etag: " << info.etag << ","
                  << "width: " << info.width << ","
                  << "height: " << info.height << ","
                  << "url: " << url;
    size_[HTTP] += info.size;
  }
  return 0;
}

bool ImageCache::clear() {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  if (!db_[HTTP]) {
    request_clear_ = true;
    return false;
  }
  request_clear_ = false;


  // Drop Table
  char *err_msg = 0;
  std::string sql = "DROP TABLE ImageCache;";
  if (SQLITE_OK != sqlite3_exec(db_[HTTP], sql.c_str(), 0, 0, &err_msg)) {
    LogError(TAG) << __func__ << " error :" << err_msg;
    sqlite3_free(err_msg);
    return false;
  }
  infos_[HTTP].clear();

  // Re Create Table
  err_msg = 0;
  sql = "CREATE TABLE IF NOT EXISTS ImageCache(";
  sql += "Url TEXT,";
  sql += "Utc TEXT,";
  sql += "Data BLOB,";
  sql += "DataSize INT,";
  sql += "ContentLength INT,";
  sql += "Etag TEXT,";
  sql += "Width INT,";
  sql += "Height INT);";
  if (SQLITE_OK != sqlite3_exec(db_[HTTP], sql.c_str(), 0, 0, &err_msg)) {
    LogError(TAG) << __func__ << " error1 :" << err_msg;
    sqlite3_free(err_msg);
    return false;
  }
  return true;
}

bool ImageCache::open() {
  open_file();
  if (!open_http()) {
    return false;
  }
  if (request_clear_) {
    clear();
  }

  removeTempFiles();
  return true;
}

bool ImageCache::open_http() {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  if (db_[HTTP]) {
    return false;
  }

  std::string appId = ::getenv("FLUTTER_APP_ID");
  std::string path = ::getenv("FLUTTER_TEMP_HOME");
  path += "/";
  path += appId;
  std::filesystem::create_directories(path);

  path += "/";
  path += DB_NAME;

  char *err_msg = 0;
  if (SQLITE_OK != sqlite3_open(path.c_str(), &db_[HTTP])) {
    LogError(TAG) << __func__ << " error0";
    sqlite3_close(db_[HTTP]);
    db_[HTTP] = nullptr;
    return false;
  }

  std::string sql = "CREATE TABLE IF NOT EXISTS ImageCache(";
  sql += "Url TEXT,";
  sql += "Utc TEXT,";
  sql += "Data BLOB,";
  sql += "DataSize INT,";
  sql += "ContentLength INT,";
  sql += "Etag TEXT,";
  sql += "Width INT,";
  sql += "Height INT);";
  if (SQLITE_OK != sqlite3_exec(db_[HTTP], sql.c_str(), 0, 0, &err_msg)) {
    LogError(TAG) << __func__ << " error1 :" << err_msg;
    sqlite3_free(err_msg);
    return false;
  }

  sql = "SELECT Url,Utc,DataSize,ContentLength,Etag,Width,Height FROM ImageCache";
  if (SQLITE_OK != sqlite3_exec(db_[HTTP], sql.c_str(), onOpen, 0, &err_msg)) {
    LogError(TAG) << __func__ << " error2 :" << err_msg;
    sqlite3_free(err_msg);

    clear();
    sqlite3_close(db_[HTTP]);
    db_[HTTP] = nullptr;
    return open_http();
  }

  LogInfo(TAG) << "open network cache " << path
               << ", current cache num is "
               << infos_[HTTP].size()
               << ", current cache size is "
               << size_[HTTP]/MB
               << "/" << max_size_[HTTP]/MB << " MB";
  return true;
}

bool ImageCache::open_file() {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  if (db_[kFILE]) {
    return false;
  }

  std::string appId = ::getenv("FLUTTER_APP_ID");
  std::string path = "/tmp";
  path += "/";
  path += appId;
  std::filesystem::create_directories(path);

  char *err_msg = 0;
  if (SQLITE_OK != sqlite3_open(":memory:", &db_[kFILE])) {
    LogError(TAG) << __func__ << " error0";
    sqlite3_close(db_[kFILE]);
    db_[kFILE] = nullptr;
    return false;
  }

  std::string sql = "CREATE TABLE IF NOT EXISTS ImageCache(";
  sql += "Url TEXT,";
  sql += "Utc TEXT,";
  sql += "Data BLOB,";
  sql += "DataSize INT,";
  sql += "ContentLength INT,";
  sql += "Etag TEXT,";
  sql += "Width INT,";
  sql += "Height INT);";
  if (SQLITE_OK != sqlite3_exec(db_[kFILE], sql.c_str(), 0, 0, &err_msg)) {
    LogError(TAG) << __func__ << " error1 :" << err_msg;
    sqlite3_free(err_msg);
    return false;
  }
  return true;
}

//////////////////////////////////////////////////////
//                 Close                            //
//////////////////////////////////////////////////////
bool ImageCache::close() {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  for (int i = 0; i < 2; i++) {
    if (!db_[i]) {
      return false;
    }

    sqlite3_close(db_[i]);
    db_[i] = nullptr;
    infos_[i].clear();
  }

  // Remove temp files.
  removeTempFiles();
  return true;
}

//////////////////////////////////////////////////////
//                 Insert                           //
//////////////////////////////////////////////////////
void ImageCache::tobe_inserted(int indx,int insert_size) {
  const size_t tobe_size = size_[indx] + insert_size;

  if (tobe_size < max_size_[indx]) {
    return;
  }
  if (infos_[indx].empty()) {
    return;
  }

  typedef struct {
    std::string url;
    int size;
  } candidate_info_t;

  // sort by UTC
  std::multimap<int64_t,candidate_info_t> candidates;
  for (auto it = infos_[indx].begin(); it != infos_[indx].end(); ++it) {
    const std::string url = it->first;
    const Info info = it->second;
    candidate_info_t candidate = { url, info.size };
    candidates.emplace(info.utc, candidate);
  }

  // Begin Transaction.
  char *err_msg = 0;
  if (SQLITE_OK != sqlite3_exec(db_[indx], "BEGIN TRANSACTION", NULL, NULL, &err_msg)) {
    LogError(TAG) << __func__ << " error0 :" << err_msg;
    sqlite3_free(err_msg);
    return;
  }

  const size_t tobe_erase_size = tobe_size - max_size_[indx];
  size_t erase_size = 0;
  for (auto it = candidates.begin(); it != candidates.end(); ++it) {
    const int64_t utc = it->first;
    const size_t size = it->second.size;
    const std::string url = it->second.url;

    // start erase
    std::string sql = "DELETE FROM ImageCache WHERE Url = ";
    sql += "'" + url + "'";
    if (SQLITE_OK != sqlite3_exec(db_[indx], sql.c_str(), 0, 0, &err_msg)) {
      LogError(TAG) << __func__ << " error1 :" << err_msg;
      sqlite3_free(err_msg);

      if (SQLITE_OK != sqlite3_exec(db_[indx], "END TRANSACTION", NULL, NULL, &err_msg)) {
        LogError(TAG) << __func__ << " error2 :" << err_msg;
        sqlite3_free(err_msg);
      }
      return;
    }
    size_[indx] -= size;
    infos_[indx].erase(url);
    LogInfo(TAG) << "UTC " << int64_to_hexstring(utc) << " was erased... "
                  << size_[indx]/MB
                  << "/" << max_size_[indx]/MB << " MB";
    // end erase
    erase_size += size;
    if (erase_size >= tobe_erase_size) {
      break;
    }
  }

  // End Transaction.
  if (SQLITE_OK != sqlite3_exec(db_[indx], "END TRANSACTION", NULL, NULL, &err_msg)) {
    LogError(TAG) << __func__ << " error3 :" << err_msg;
    sqlite3_free(err_msg);
  }
}

bool ImageCache::insert_raw(const std::string& url,
                        uint8_t* data,
                        int data_size,
                        int width,
                        int height,
                        int content_length,
                        std::string etag) {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  int indx = (url.find("http") == 0) ? HTTP : kFILE;
  if (!db_[indx]) {
    return false;
  }
  if (infos_[indx].find(url) != infos_[indx].end()) {
    return true;
  }
  tobe_inserted(indx,data_size);
  int64_t utc = get_utc();

  // Insert
  sqlite3_stmt* res;
  std::string sql = "INSERT INTO ImageCache(Url,Utc,Data,DataSize,ContentLength,Etag,Width,Height) VALUES(";
  sql += "'" + url + "'" + ",";
  sql += "'" + int64_to_hexstring(utc) + "'" + ",";
  sql += "?,";
  sql += std::to_string(data_size) + ",";
  sql += std::to_string(content_length) + ",";
  sql += "'" + etag + "'" + ",";
  sql += std::to_string(width) + ",";
  sql += std::to_string(height) + ");";
  if (SQLITE_OK != sqlite3_prepare_v2(db_[indx], sql.c_str(), -1, &res, 0)) {
    LogError(TAG) << __func__ << " error0 : " << sqlite3_errmsg(db_[indx]);
    return false;
  }

  sqlite3_bind_blob(res, 1, data, data_size, SQLITE_TRANSIENT);
  if (SQLITE_DONE != sqlite3_step(res)) {
    LogError(TAG) << __func__ << " error1 : " << sqlite3_errmsg(db_[indx]);
    return false;
  }

  sqlite3_finalize(res);
  sqlite3_db_release_memory(db_[indx]);
  sqlite3_db_cacheflush(db_[indx]);

  // finalize
  size_[indx] += data_size;
  infos_[indx][url] = { utc, utc, data_size, content_length, etag, width, height};
  LogInfo(TAG) << "Insert " << data_size/1024 << " KB to the Cache "
               << infos_[indx].size() << "th "
               << size_[indx]/MB
               << "/" << max_size_[indx]/MB << " MB "
               << cut_url(ImageCache::strip_url(url));
  return true;
}

bool ImageCache::insert_png(const std::string& url,
                        uint8_t* buf,int width,int height,int bpp,
                        int content_length,std::string etag) {
  {
    std::lock_guard<std::recursive_mutex> autoLock(mutex_);
    int indx = (url.find("http") == 0) ? HTTP : kFILE;
    if (!db_[indx]) {
      return false;
    }
    if (infos_[indx].find(url) != infos_[indx].end()) {
      return true;
    }
  }

  std::string path = getTempFilePath(url);
  if (::access(path.c_str(),F_OK) == 0) {
    return false;
  }
  if (!ImageFactory::writePNG(path,buf,width,height,bpp)) {
    return false;
  }

  size_t size;
  std::string mtime;
  std::tie(size,mtime) = get_fileinfo(path);
  auto ubuf = std::make_unique<uint8_t[]>(size);
  uint8_t* pbuf = (uint8_t*)ubuf.get();

  std::ifstream is(path, std::ios::binary);
  if (is.is_open()) {
    is.read((char*)pbuf,size);
    is.close();

    std::filesystem::remove(path);
  } else {
    LogError(TAG) << "PNG file write fail";
    return false;
  }

  // push to DB
  return insert_raw(url,
           pbuf,size,
           width,
           height,
           content_length,
           etag);
}

//////////////////////////////////////////////////////
//              Insert Compression                  //
//////////////////////////////////////////////////////
bool ImageCache::insert_compression(const std::string& url,
                        uint8_t* buf,int width,int height,int bpp,
                        int content_length,std::string etag) {
  {
    std::lock_guard<std::recursive_mutex> autoLock(mutex_);
    int indx = (url.find("http") == 0) ? HTTP : kFILE;
    if (!db_[indx]) {
      return false;
    }
    // Must be return false, to prevent re-run
    if (infos_[indx].find(url) != infos_[indx].end()) {
      return false;
    }
    if (invalid_urls_.find(url) != invalid_urls_.end()) {
      return false;
    }
  }

  std::string path = getTempFilePath(url);
  if (::access(path.c_str(),F_OK) == 0) {
    return false;
  }
  if (!ImageFactory::writeBMP(path,buf,width,height,bpp)) {
    return false;
  }

  CompressInfo compress_info = { url, width, height, bpp == 32, content_length, etag };
  return insert_compression(compress_info);
}

bool ImageCache::insert_compression(CompressInfo compress_info) {
  std::string in_path = getTempFilePath(compress_info.url);
  std::string out_path = in_path;
  out_path += ".pvr";

  bool retv = false;
  try {
    int64_t start_tick = getCurTick();
    bool is_valid = etcpak_encoder(cut_url(ImageCache::strip_url(compress_info.url)),
                                   compress_info.alpha,
                                   in_path.c_str(),out_path.c_str());
    LogDebug(TAG) << "etcpak_encoder execute time is " << getDiffTick(start_tick) << " msec";

    size_t size;
    std::string mtime;
    std::tie(size,mtime) = get_fileinfo(out_path);
    auto buf = std::make_unique<uint8_t[]>(size);
    uint8_t* pbuf = (uint8_t*)buf.get();

    std::ifstream is(out_path, std::ios::binary);
    if (is.is_open()) {
      is.read((char*)pbuf,size);
      is.close();

      std::filesystem::remove(in_path);
      std::filesystem::remove(out_path);
    } else {
      LogError(TAG) << "etcpak_encoder file write fail";
      return retv;
    }
    if (!is_valid) {
      std::lock_guard<std::recursive_mutex> autoLock(mutex_);
      invalid_urls_[compress_info.url] = true;
      LogDebug(TAG) << "etcpak_encoder(" << compress_info.url << ") not-valid";
      return false;
    }

    // size = PVR header + ETC data
    struct pvr_header {
      uint32_t version;
      uint32_t flags;
      uint32_t pixelFormat;
      uint32_t dummy;
      uint32_t colorSpace;
      uint32_t channelType;
      uint32_t height;
      uint32_t width;
      uint32_t depth;
      uint32_t numSurfaces;
      uint32_t numFaces;
      uint32_t mipmapCount;
      uint32_t metaSize;
    };

    struct pvr_header* h = (struct pvr_header*)pbuf;
    size = h->width * h->height;
    if (!compress_info.alpha) {
      size = size >> 1;
    }
    size += sizeof(struct pvr_header);

    // push to DB
    retv = insert_raw(compress_info.url,
           pbuf,size,
           compress_info.width,
           compress_info.height,
           compress_info.content_length,
           compress_info.etag);
  } catch (...) {
    LogError(TAG) << "etcpak_encoder assert";
  }
  return retv;
}

//////////////////////////////////////////////////////
//                 Erase                            //
//////////////////////////////////////////////////////
bool ImageCache::erase(const std::string& url) {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  int indx = (url.find("http") == 0) ? HTTP : kFILE;
  if (!db_[indx] || infos_[indx].find(url) == infos_[indx].end()) {
    return false;
  }

  char *err_msg = 0;
  std::string sql = "DELETE FROM ImageCache WHERE Url = ";
  sql += "'" + url + "'";
  if (SQLITE_OK != sqlite3_exec(db_[indx], sql.c_str(), 0, 0, &err_msg)) {
    LogError(TAG) << __func__ << " error :" << err_msg;
    sqlite3_free(err_msg);
    return false;
  }
  sqlite3_db_release_memory(db_[indx]);
  sqlite3_db_cacheflush(db_[indx]);

  size_[indx] -= infos_[indx][url].size;
  infos_[indx].erase(url);
  return true;
}

//////////////////////////////////////////////////////
//                 get_infos                        //
//////////////////////////////////////////////////////
uint8_t* ImageCache::get_data(const std::string& url) {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  int indx = (url.find("http") == 0) ? HTTP : kFILE;
  sqlite3_stmt* res;

  std::string sql = "SELECT Data FROM ImageCache WHERE Url = ";
  sql += "'" + url + "'";
  if (SQLITE_OK != sqlite3_prepare_v2(db_[indx], sql.c_str(), -1, &res, 0)) {
    LogError(TAG) << __func__ << " error : " << sqlite3_errmsg(db_[indx]);
    return nullptr;
  }

  uint8_t* buf = nullptr;
  if (SQLITE_ROW == sqlite3_step(res)) {
    size_t size = sqlite3_column_bytes(res, 0);
    buf = (uint8_t*)::malloc(size);
    ::memcpy(buf,sqlite3_column_blob(res, 0),size);
  }
  sqlite3_finalize(res);
  sqlite3_db_release_memory(db_[indx]);
  return buf;
}

bool ImageCache::ignore_validate(const std::string& url) {
  std::lock_guard<std::recursive_mutex> autoLock(mutex_);
  int indx = (url.find("http") == 0) ? HTTP : kFILE;
  Info info = infos_[indx][url];
  int64_t utc = get_utc();

  int64_t cached_utc = info.utc;
  int64_t access_utc = info.access_utc;
  int64_t diff_cached = utc - cached_utc;
  int64_t diff_access = utc - access_utc;
  if (indx == HTTP &&
      diff_cached <= UPDATE_UTC_INTERVAL &&
      diff_access <= IGNORE_VALIDATE_INTERVAL) {
    return true;
  }

  utc = get_utc();
  // Update access_utc
  info.access_utc = utc;
  infos_[indx][url] = info;

  if (indx == kFILE ||
      diff_cached > UPDATE_UTC_INTERVAL) {
    // Update cached_utc
    char *err_msg = 0;
    std::string sql = "UPDATE ImageCache SET Utc = ";
    sql += "'" + int64_to_hexstring(utc) + "' WHERE Url = ";
    sql += "'" + url + "'";
    if (SQLITE_OK != sqlite3_exec(db_[indx], sql.c_str(), 0, 0, &err_msg)) {
      LogError(TAG) << __func__ << " error :" << err_msg;
      sqlite3_free(err_msg);
    } else {
      info.utc = utc;
      infos_[indx][url] = info;
    }
    sqlite3_db_release_memory(db_[indx]);
    sqlite3_db_cacheflush(db_[indx]);
  }
  return false;
}

static size_t onHttpHead(char* ptr, size_t size, size_t count, void* data) {
  std::string* response = (std::string*)data;
  *response += ptr;
  return size * count;
}

int ImageCache::validate(const std::string& url) {
  int retv = OK;
  int indx = (url.find("http") == 0) ? HTTP : kFILE;
  int content_length = 0;
  std::string etag;

  {
    std::lock_guard<std::recursive_mutex> autoLock(mutex_);
    content_length = infos_[indx][url].content_length;
    etag = infos_[indx][url].etag;
    if (content_length == 0 && etag.empty()) {
      return OK;
    }
  }

  std::string pure_url = strip_url(url);
  // FILE
  if (indx == kFILE) {
    size_t size;
    std::string mtime;
    std::tie(size,mtime) = get_fileinfo(pure_url);
    if (size != content_length) {
      retv = FAIL;
      LogInfo(TAG) << __func__ << " file length is different, the cache will be deleted";
    } else if (mtime != etag) {
      retv = FAIL;
      LogInfo(TAG) << __func__ << " file modity time is different, the cache will be deleted";
    }
    return retv;
  }

  // NETWORK
  int64_t start_tick = getCurTick();
  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    LogWarn(TAG) << __func__ << " CURL: init error";
    return OK;
  }

  std::string header_response;
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "flutter ImageTexture");
  curl_easy_setopt(curl, CURLOPT_URL, pure_url.c_str());
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, NETWORK_CONNECTION_TIMEOUT_SEC);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, NETWORK_TIMEOUT_MS);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, onHttpHead);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_response);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code > 300) {
      retv = FAIL;
      std::string errText = curl_easy_strerror(res);
      LogInfo(TAG) << __func__ << " CURL: respond(" << response_code << ") " << errText
                   << ", the cache will be deleted";
    } else if (response_code == CURLE_COULDNT_RESOLVE_HOST) {
      retv = NO_NETWORK;
      std::string errText = curl_easy_strerror(res);
      LogInfo(TAG) << __func__ << " CURL: respond(" << response_code << ") " << errText;
    }
  } else {
    LogDebug(TAG) << __func__ << " execute time is " << getDiffTick(start_tick) << " msec";
    if (content_length != ImageCache::get_contentLength(header_response)) {
      retv = FAIL;
      LogInfo(TAG) << __func__ << " content-length is different, the cache will be deleted";
    } else if (etag != ImageCache::get_etag(header_response)) {
      retv = FAIL;
      LogInfo(TAG) << __func__ << " etag is different, the cache will be deleted";
    }
  }

  curl_easy_cleanup(curl) ;
  return retv;
}

std::tuple<uint8_t*,int,int,int> ImageCache::get_infos(const std::string& url, bool no_validate) {
  int indx = (url.find("http") == 0) ? HTTP : kFILE;
  uint8_t* buf = nullptr;
  int buf_size = 0;
  int width = 0;
  int height = 0;

  {
    std::lock_guard<std::recursive_mutex> autoLock(mutex_);
    if (!db_[indx] || infos_[indx].find(url) == infos_[indx].end()) {
      return std::make_tuple(buf,buf_size,width,height);
    }
  }

  if (!no_validate && !ignore_validate(url)) {
    int errCode = validate(url);
    if (errCode != OK) {
      if (errCode == FAIL) {
        erase(url);
      }
      return std::make_tuple(buf,buf_size,width,height);
    }
  }

  {
    std::lock_guard<std::recursive_mutex> autoLock(mutex_);
    buf = get_data(url);
    buf_size = infos_[indx][url].size;
    width = infos_[indx][url].width;
    height = infos_[indx][url].height;
  }
  return std::make_tuple(buf,buf_size,width,height);
}

std::tuple<size_t,std::string> ImageCache::get_fileinfo(const std::string& url) {
  std::string fname = url;
  size_t size = 0;
  std::string etag;
  struct stat fileState;
  if (::stat(fname.c_str(),&fileState) != 0) {
    return std::make_tuple(size,etag);
  }

  char buff[32];
  ::strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", ::localtime(&fileState.st_mtime));

  size = fileState.st_size;
  etag = buff;
  return std::make_tuple(size,etag);
}

//////////////////////////////////////////////////////
//       Parse Http header response                 //
//////////////////////////////////////////////////////
static std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c){ return std::tolower(c); });
  return s;
}

int ImageCache::get_contentLength(const std::string& response) {
  std::string key = "content-length:";
  std::string r = toLower(response);
  size_t start = r.find(key);
  if (start == std::string::npos) {
    return 0;
  }
  start += key.length();
  size_t end = r.length() - start;
  size_t pos = response.find('\n',start);
  end = (pos != std::string::npos) ? pos : end;

  std::string contentSize = response.substr(start,end-start);
  return std::stoi(contentSize);
}

std::string ImageCache::get_etag(const std::string& response) {
  std::string etag;
  std::string key = "etag:";
  std::string r = toLower(response);
  size_t start = r.find(key);
  if (start == std::string::npos) {
    return etag;
  }
  start += key.length();
  size_t end = r.length() - start;
  size_t pos = response.find('\n',start);
  end = (pos != std::string::npos) ? pos : end;

  std::string line = response.substr(start,end-start);
  start = line.find('"');
  if (start == std::string::npos) {
    return etag;
  }
  start += 1;
  end = line.find('"',start);
  if (end == std::string::npos) {
    return etag;
  }
  etag = line.substr(start,end-start);
  return etag;
}

std::string ImageCache::get_cacheControl(const std::string& response) {
  std::string cacheControl;
  std::string key = "cache-control:";
  std::string r = toLower(response);
  size_t start = r.find(key);
  if (start == std::string::npos) {
    return cacheControl;
  }
  start += key.length();
  size_t end = r.length() - start;
  size_t pos = response.find('\n',start);
  end = (pos != std::string::npos) ? pos : end;

  std::string line = response.substr(start,end-start);
  cacheControl = toLower(line);
  return cacheControl;
}

std::string ImageCache::bind_url(const std::string& url,
                              int required_width, int required_height) {
  std::stringstream ssw;
  std::stringstream ssh;
  ssw << std::setfill('0') << std::setw(5) << required_width;
  ssh << std::setfill('0') << std::setw(5) << required_height;
  return url + ssw.str() + ssh.str();
}

std::string ImageCache::strip_url(const std::string& url) {
  size_t size = url.length();
  return size > 10 ? url.substr(0,size-10) : url;
}

