// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef DUMMY_PLAYER_H_
#define DUMMY_PLAYER_H_

#include <functional>
#include <string>
#include <vector>
#include <set>
#include "plugin_player_delegate.h"
#include "webos_player.h"

class DummyPlayer : public PluginPlayerDelegate {
 public:
  DummyPlayer();
  virtual ~DummyPlayer();

  void SetUrl(const std::string &url);
  void Release();

// Call at WebOSPlayer.
  virtual void OnStateChanged(int state) override;
  virtual void OnInitialized() override;
  virtual void OnPlayCompleted() override;
  virtual void OnSeekCompleted() override;
  virtual void OnBufferingStart() override;
  virtual void OnBufferingEnd() override;
  virtual void OnBuffered(int64_t start, int64_t end) override;
  virtual void OnError(int64_t errCode,const std::string& errText) override;

 private:
  std::string app_id_;
  WebOSPlayer player_;
};

#endif
