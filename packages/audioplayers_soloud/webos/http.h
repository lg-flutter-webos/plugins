// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef HTTP_H_
#define HTTP_H_

#include <mutex>
#include <tuple>
#include <thread>
#include <string>
#include <map>
#include <vector>

class Http {
 public:
  typedef void(*DoneCb)(const std::string& url);

  Http();
  ~Http();

  void setDoneCallback(DoneCb);
  std::tuple<uint8_t*, size_t> get(const std::string& url, uint64_t uid);
  std::tuple<int, std::string> get_error(const std::string& url);
  int erase(const std::string& url, uint64_t uid);

 private:
  class Info {
   public:
    Info() = default;
    ~Info() = default;

    std::thread* pthread = nullptr;
    uint8_t* buf = nullptr;
    size_t buf_size = 0;
    int64_t error_code = 0;
    std::string error_text;

    std::map<uint64_t, bool> uids;
    bool run = true;
  };

  class Curl {
   public:
    Curl(Info& info);
    ~Curl();
    std::tuple<int,std::string> http(const std::string& url);

   private:
    const int64_t NETWORK_CONNECTION_TIMEOUT_SEC = 2;
    const int64_t NETWORK_TIMEOUT_MS = 10*1000;
    static const size_t MAX_BUFFER_SIZE = 10*1024*1024;

    static size_t onHttpBody(void* ptr, size_t size, size_t count, void* data);
    static std::tuple<uint8_t*, size_t> readBinaryFile(const std::string& filename);
    static std::tuple<uint8_t*, size_t> dataParsing(const std::string& url);

    void httpCleanUp();

    Info& parent_info_;
    std::string url_;
    uint8_t* http_buf_ = nullptr;
    size_t http_buf_size_ = 0;
    std::vector<uint8_t> http_packets_;
  };

  std::tuple<uint8_t*, size_t> readBinaryFile(const std::string& filename);
  std::tuple<uint8_t*, size_t> dataParsing(const std::string& url);

  DoneCb done_cb_ = nullptr;
  std::map<std::string, Info> infos_;
};

#endif  // HTTP_H_
