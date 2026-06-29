// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: LicenseRef-LGE-Proprietary

#ifndef IMAGECACHE_H_
#define IMAGECACHE_H_
#include <tuple>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <sqlite3.h>

class ImageCache {
public:
  static void set_maxbytes(size_t bytes);
  static bool clear();
  static bool open();
  static bool close();
  static bool insert_raw(const std::string& url,
                     uint8_t* data, int data_size,int width,int height,
                     int content_length,std::string etag);
  static bool insert_png(const std::string& url,
                        uint8_t* buf,int width,int height,int bpp,
                        int content_length,std::string etag);
  static bool insert_compression(const std::string& url,
                     uint8_t* buf,int width,int height,int bpp,
                     int content_length,std::string etag);
  static bool erase(const std::string& url);
  static std::tuple<uint8_t*,int,int,int> get_infos(const std::string& url,bool no_validate = false);
  static std::tuple<size_t,std::string> get_fileinfo(const std::string& url);
  static int get_contentLength(const std::string& response);
  static std::string get_etag(const std::string& response);
  static std::string get_cacheControl(const std::string& response);
  static std::string bind_url(const std::string& url,int required_width,int required_height);
  static std::string strip_url(const std::string& url);

private:
  #define UPDATE_UTC_INTERVAL 24*3600*1000
  #define IGNORE_VALIDATE_INTERVAL 3600*1000
  #define DB_NAME "image_texture.db"

  typedef struct {
    int64_t utc;
    int64_t access_utc;
    int size;
    int content_length;
    std::string etag;
    int width;
    int height;
  } Info;

  typedef struct {
    std::string url;
    int width;
    int height;
    bool alpha;
    int content_length;
    std::string etag;
  } CompressInfo;

  static bool open_http();
  static bool open_file();
  static int onOpen(
    void *,
    int argc,
    char **argv,
    char **azColName);
  static bool insert_compression(CompressInfo compress_info);
  static void tobe_inserted(int indx,int size);
  static uint8_t* get_data(const std::string& url);
  static bool ignore_validate(const std::string& url);
  static int validate(const std::string& url);

  static sqlite3* db_[2];
  static std::map<std::string,Info> infos_[2];
  static std::map<std::string, bool> invalid_urls_;
  static size_t size_[2];
  static size_t max_size_[2];
  static bool request_clear_;
  static std::recursive_mutex mutex_;
};
#endif
