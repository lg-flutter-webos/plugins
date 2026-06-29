// SPDX-FileCopyrightText: Copyright 2023 LG Electronics Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "include/gamepads_webos/gamepads_webos_plugin.h"

#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar.h>
#include <flutter/standard_method_codec.h>
#include <flutter/basic_message_channel.h>
#include <flutter/binary_messenger.h>

#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <thread>

#include "connection_listener.h"
#include "gamepad.h"
#include "log.h"

#ifdef TAG
#undef TAG
#endif
#define TAG "GamepadsWebosPlugin::"

class GamepadsWebosPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrar* registrar);

  GamepadsWebosPlugin(flutter::PluginRegistrar* registrar);

  virtual ~GamepadsWebosPlugin();

  // Disallow copy and assign.
  GamepadsWebosPlugin(const GamepadsWebosPlugin&) = delete;
  GamepadsWebosPlugin& operator=(const GamepadsWebosPlugin&) = delete;

 private:
  static std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      channel_;

  bool keep_reading_events_;
  std::thread event_loop_thread_;
  std::map<std::string, gamepad::GamepadInfo> gamepads_ = {};

  void emit_gamepad_event(gamepad::GamepadInfo* gamepad,
                               const js_event& event);
  void process_connection_event(gamepad::GamepadInfo* gamepad);
  void event_loop_start();

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      GamepadsWebosPlugin::channel_;

void GamepadsWebosPlugin::emit_gamepad_event(gamepad::GamepadInfo* gamepad,
                               const js_event& event) {
  auto channel = this->channel_.get();
  if (channel) {
    gamepad::EventInfo evt = gamepad::parse_gamepad_event(gamepad, event);
    flutter::EncodableMap map;
    map[flutter::EncodableValue("gamepadId")] =
        flutter::EncodableValue(gamepad->device_id);
    map[flutter::EncodableValue("time")] = flutter::EncodableValue(evt.time);
    map[flutter::EncodableValue("type")] = flutter::EncodableValue(evt.type);
    map[flutter::EncodableValue("key")] = flutter::EncodableValue(evt.key);
    map[flutter::EncodableValue("value")] = flutter::EncodableValue(evt.value);
    channel->InvokeMethod("onGamepadEvent",
                           std::make_unique<flutter::EncodableValue>(
                               flutter::EncodableValue(map)));
  }
}

void GamepadsWebosPlugin::process_connection_event(gamepad::GamepadInfo* gamepad) {
  gamepad::listen(gamepad, [this, gamepad](const js_event& value) {
    emit_gamepad_event(gamepad, value);
  });
}

void GamepadsWebosPlugin::event_loop_start() {
  connection_listener::listen(
      &keep_reading_events_,
      [this](const connection_listener::ConnectionEvent& event) {
        std::string key = event.device_id;
        bool existing = (gamepads_.find(key) != gamepads_.end())
                      ? true
                      : false;
        if (event.type == connection_listener::ConnectionEventType::CONNECTED) {
          if (existing && gamepads_[key].alive) {
            return;
          }

          std::optional<gamepad::GamepadInfo> info =
              gamepad::get_gamepad_info(key);
          if (!info) {
            return;
          }

          LogInfo(TAG) << "Gamepad connected " << key << " - " << info->name;
          gamepads_[key] = *info;

          std::thread input_thread(
            [](GamepadsWebosPlugin* self, gamepad::GamepadInfo* gamepad) {
              self->process_connection_event(gamepad);
            },
            this,
            &gamepads_[key]);
          input_thread.detach();
        } else if (existing) {
          LogInfo(TAG) << "Gamepad disconnected " << key << " - " << gamepads_[key].name;
          gamepads_[key].alive = false;
          gamepads_.erase(key);
        }
      });
}

void GamepadsWebosPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrar* registrar) {
  channel_ = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
      registrar->messenger(), "xyz.luan/gamepads",
      &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<GamepadsWebosPlugin>(registrar);

  channel_->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto& call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

GamepadsWebosPlugin::GamepadsWebosPlugin(
    flutter::PluginRegistrar* registrar)
    : keep_reading_events_(true)
    , event_loop_thread_([](GamepadsWebosPlugin* self) {
        self->event_loop_start();
      }, this) {
  event_loop_thread_.detach();
}

GamepadsWebosPlugin::~GamepadsWebosPlugin() {
  keep_reading_events_ = false;
}

void GamepadsWebosPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare("listGamepads") == 0) {
    flutter::EncodableList list;
    for (auto [device_id, gamepad] : gamepads_) {
      flutter::EncodableMap map;
      map[flutter::EncodableValue("id")] =
          flutter::EncodableValue(device_id);
      map[flutter::EncodableValue("name")] =
          flutter::EncodableValue(gamepad.name);
      list.push_back(flutter::EncodableValue(map));
    }
    result->Success(flutter::EncodableValue(list));
  } else {
    result->NotImplemented();
  }
}

///////////////////////////
void GamepadsWebosPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  GamepadsWebosPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrar>(registrar));
}
