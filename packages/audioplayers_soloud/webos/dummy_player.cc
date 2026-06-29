// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "dummy_player.h"
#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "DummyPlayer::"

DummyPlayer::DummyPlayer()
  : player_(this) {
  app_id_ = ::getenv("FLUTTER_APP_ID");
}

DummyPlayer::~DummyPlayer() {
}

void DummyPlayer::SetUrl(const std::string &uri) {
  Release();
  player_.load(app_id_, "", uri, "");
}

void DummyPlayer::Release() {
  player_.unload(true);
}

void DummyPlayer::OnStateChanged(int state) {
  switch (state) {
    case WebOSPlayer::LOADED:
      player_.play();
      break;

    case WebOSPlayer::REMOVED:
      LogError(TAG) << "REMOVED";
      player_.unload();
      break;

    case WebOSPlayer::UNLOADED:
      break;

    default:
      break;
  }
}

void DummyPlayer::OnInitialized() {
}

void DummyPlayer::OnPlayCompleted() {
}

void DummyPlayer::OnSeekCompleted() {
}

void DummyPlayer::OnBufferingStart() {
}

void DummyPlayer::OnBufferingEnd() {
}

void DummyPlayer::OnBuffered(int64_t start, int64_t end) {
}

void DummyPlayer::OnError(int64_t errorCode, const std::string& errorText) {
  LogError(TAG) << "OnError(" << errorCode << "," << errorText << ")";
}
