#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <curl/curl.h>
#include <glib.h>

#include "http.h"
#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "Http::"

////////////////////////////////////////////////////////////////
// Time utils
////////////////////////////////////////////////////////////////
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

static std::string cut_url(const std::string& url) {
  size_t len = url.length();
  return len > 128
      ? "..." + url.substr(len - 125)
      : url;
}

Http::Http() {
  curl_global_init(CURL_GLOBAL_ALL);
}

Http::~Http() {
}

void Http::setDoneCallback(DoneCb cb) {
  done_cb_ = cb;
}

std::tuple<uint8_t*, size_t> Http::get(const std::string& url, uint64_t uid) {
  uint8_t* buf = nullptr;
  size_t buf_size = 0;
  if (infos_.find(url) == infos_.end()) {
    Info info;
    info.uids[uid] = true;
    infos_[url] = info;

    bool dummy = false;
    Info& rinfo = infos_[url];
    if (url.find("http") == 0) {
      rinfo.pthread = new std::thread([this](const std::string url, Info& info) {
        auto curl = Curl(info);

        std::tie(info.error_code, info.error_text) = curl.http(url);
        info.run = false;
        if (done_cb_) {
          done_cb_(url);
        }
      }, url, std::ref(infos_[url]));
    } else if (url.find("/") == 0) {
      dummy = true;
      rinfo.buf = nullptr;
      rinfo.buf_size = 0;
    } else if (url.find("file://") == 0) {
      const std::string file_protocol_prefix = "file://";
      std::string fname = url.substr(file_protocol_prefix.size());
      std::tie(rinfo.buf, rinfo.buf_size) = readBinaryFile(fname);
    } else if (url.find("data:") == 0) {
      std::tie(rinfo.buf, rinfo.buf_size) = dataParsing(url);
    }

    if (rinfo.pthread == nullptr) {
      buf = rinfo.buf;
      buf_size = rinfo.buf_size;
      if (!dummy && buf_size == 0) {
        LogWarn(TAG) << cut_url(url) << " read error";
        rinfo.error_code = 404;
        rinfo.error_text = "file open error";
      }
      info.run = false;
    }
  } else {
    Info& rinfo = infos_[url];
    buf = rinfo.buf;
    buf_size = rinfo.buf_size;

    std::map<uint64_t, bool>& ruids = rinfo.uids;
    ruids[uid] = true;
  }
  return std::make_tuple(buf, buf_size);
}

int Http::erase(const std::string& url, uint64_t uid) {
  if (infos_.find(url) == infos_.end()) {
    return -1;
  }

  Info& rinfo = infos_[url];
  std::map<uint64_t, bool>& ruids = rinfo.uids;
  ruids.erase(uid);
  if (ruids.size() != 0) {
    return 1;
  }

  if (rinfo.run) {
    LogInfo(TAG) << cut_url(url) << " is running";
    rinfo.run = false;
  }
  if (rinfo.pthread) {
    rinfo.pthread->join();
    delete rinfo.pthread;
  }

  if (rinfo.buf) {
    ::free(rinfo.buf);
  }
  infos_.erase(url);
  LogInfo(TAG) << "erased " << cut_url(url);
  return 0;
}

std::tuple<int, std::string> Http::get_error(const std::string& url) {
  int error_code = 0;
  std::string error_text;
  if (infos_.find(url) != infos_.end()) {
    Info info = infos_[url];
    error_code = info.error_code;
    error_text = info.error_text;
  }
  return std::make_tuple(error_code, error_text);
}

std::tuple<uint8_t*, size_t> Http::readBinaryFile(const std::string& filename) {
  uint8_t* buf = nullptr;
  size_t buf_size = 0;
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
    LogInfo(TAG) << "readBinaryFile(" << cut_url(filename) << ") open error";
    return std::make_tuple(buf, buf_size);
  }

  std::streamsize size = file.tellg();
  buf = (uint8_t*)::malloc(size);
  file.seekg(0, std::ios::beg);

  if (!file.read(reinterpret_cast<char*>(buf), size)) {
    LogInfo(TAG) << "readBinaryFile(" << cut_url(filename) << ") read " << size << " error";
    ::free(buf);
    buf = nullptr;
    size = 0;
  }

  buf_size = static_cast<size_t>(size);
  return std::make_tuple(buf, buf_size);
}

std::tuple<uint8_t*, size_t> Http::dataParsing(const std::string& url) {
  uint8_t* buf = nullptr;
  size_t buf_size = 0;
  size_t position = url.find(",");
  if (position == std::string::npos) {
    return std::make_tuple(buf, buf_size);
  }

  std::string header = url.substr(0, position);
  std::string payload = url.substr(position + 1);
  if (header.find("base64") != std::string::npos) {
    gsize decSize = 0;
    buf = (uint8_t*)g_base64_decode((char*)payload.c_str(), &decSize);
    buf_size = decSize;
  }
  return std::make_tuple(buf, buf_size);
}
Http::Curl::Curl(Info& info)
 : parent_info_(info) {
}

Http::Curl::~Curl() {
}

std::tuple<int,std::string> Http::Curl::http(const std::string& url) {
  httpCleanUp();
  url_ = url;

  int errCode = 0;
  std::string errText;
  int64_t start_tick = getCurTick();

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    LogWarn(TAG) << "CURL: init error";
    errCode = CURLE_FAILED_INIT;
    errText = "Curl init error";
    return std::make_tuple(errCode,errText);
  }
  // Initialize
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "LGE Flutter Browser");
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onHttpBody);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, NETWORK_CONNECTION_TIMEOUT_SEC);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, NETWORK_TIMEOUT_MS);

  CURLcode res = CURLE_OK;
  while (parent_info_.run) {
    res = curl_easy_perform(curl);
    if (getDiffTick(start_tick) > NETWORK_TIMEOUT_MS) {
      break;
    }
    if (res != CURLE_OPERATION_TIMEDOUT &&
        res != CURLE_COULDNT_CONNECT) {
      break;
    }
    std::string err = curl_easy_strerror(res);
    LogWarn(TAG) << "CURL: retry "
                 << err << " "
                 << cut_url(url);
    httpCleanUp();
  }

  // Finalize
  if (res == CURLE_OK) {
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code > 300) {
      httpCleanUp();
      errCode = response_code;
      errText = "server error";
    } else {
      size_t size = http_packets_.size();
      http_buf_ = (uint8_t*)::malloc(size);
      http_buf_size_ = size;
      ::memcpy(http_buf_, http_packets_.data(), size);
      http_packets_.clear();

      parent_info_.buf = http_buf_;
      parent_info_.buf_size = http_buf_size_;
    }
  } else {
    httpCleanUp();
    if (parent_info_.run) {
      errCode = (int)res;
      errText = curl_easy_strerror(res);
    }
  }
  curl_easy_cleanup(curl);

  if (errCode) {
    LogWarn(TAG) << "CURL: error code " << errCode
                 << "(" << errText << ").."
                 << cut_url(url);
  }
  return std::make_tuple(errCode,errText);
}

void Http::Curl::httpCleanUp() {
 if (http_buf_) {
    ::free(http_buf_);
    http_buf_ = nullptr;
  }
  http_buf_size_ = 0;

  http_packets_.clear();
}

size_t Http::Curl::onHttpBody(void *ptr, size_t size, size_t count, void *data) {
  Curl* self = (Curl*)data;
  if (!self->parent_info_.run) {
    self->httpCleanUp();
    return 0;
  }

  size_t sz = size * count;
  uint8_t* buf = (uint8_t*)ptr;
  std::vector<uint8_t>& packets = self->http_packets_;

  packets.insert(packets.end(), buf, buf + sz);
  if (packets.size() > MAX_BUFFER_SIZE) {
    LogInfo(TAG) << "Too large " << cut_url(self->url_) << " buffer size !!!";
    self->httpCleanUp();
    return 0;
  }

  return sz;
}
