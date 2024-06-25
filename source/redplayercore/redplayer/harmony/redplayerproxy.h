#pragma once

#include "Interface/RedPlayer.h"
#include "napi/native_api.h"
#include "redplayer_callarktslog.h"
#include "redplayer_event_dispatcher.h"
#include "redplayer_logcallback.h"

#include "ace/xcomponent/native_interface_xcomponent.h"
#include <map>
#include <string>

class RedPlayerProxy {
public:
  napi_value createPlayer(napi_env env, napi_callback_info info);
  napi_value setSurfaceId(napi_env env, napi_callback_info info,
                          std::string surfaceId);
  napi_value setDataSource(napi_env env, napi_callback_info info,
                           std::string url);
  napi_value setDataSourceFd(napi_env env, napi_callback_info info, int64_t fd);
  napi_value prepare(napi_env env, napi_callback_info info);
  napi_value seek(napi_env env, napi_callback_info info, int64_t seekPosition);
  napi_value setSpeed(napi_env env, napi_callback_info info, double speed);
  napi_value setVolume(napi_env env, napi_callback_info info, double volume);
  napi_value setLoop(napi_env env, napi_callback_info info, int64_t loop);
  napi_value start(napi_env env, napi_callback_info info);
  napi_value pause(napi_env env, napi_callback_info info);
  napi_value stop(napi_env env, napi_callback_info info);
  napi_value release(napi_env env, napi_callback_info info);
  napi_value reset(napi_env env, napi_callback_info info);
  napi_value getCurrentPosition(napi_env env, napi_callback_info info);
  napi_value getVideoDuration(napi_env env, napi_callback_info info);
  napi_value getVideoWidth(napi_env env, napi_callback_info info);
  napi_value getVideoHeight(napi_env env, napi_callback_info info);
  napi_value getUrl(napi_env env, napi_callback_info info);
  napi_value getAudioCodecInfo(napi_env env, napi_callback_info info);
  napi_value getVideoCodecInfo(napi_env env, napi_callback_info info);
  napi_value isPlaying(napi_env env, napi_callback_info info);
  napi_value isLooping(napi_env env, napi_callback_info info);
  napi_value getVideoCachedSizeBytes(napi_env env, napi_callback_info info);
  napi_value getAudioCachedSizeBytes(napi_env env, napi_callback_info info);
  napi_value getVideoCachedSizeMs(napi_env env, napi_callback_info info);
  napi_value getAudioCachedSizeMs(napi_env env, napi_callback_info info);
  RED_ERR setConfig(int type, std::string name, std::string value);
  RED_ERR setConfig(int type, std::string name, int64_t value);

  std::string id;

private:
  void messageLoopN(redPlayer_ns::CRedPlayer *mp);
  RED_ERR messageLoop(redPlayer_ns::CRedPlayer *mp);
  std::shared_ptr<redPlayer_ns::CRedPlayer> player_entity_;
  OH_NativeXComponent_Callback x_component_callback_;
};

class PlayerManager {
public:
  void addPlayer(std::shared_ptr<RedPlayerProxy> &player);

  std::shared_ptr<RedPlayerProxy> getPlayer(const std::string &playerId);

  void removePlayer(const std::string &playerId);

  int addAndGetId();

  ~PlayerManager();

  static PlayerManager *getInstance();

private:
  PlayerManager();

  std::map<std::string, std::shared_ptr<RedPlayerProxy>> players_;
  std::mutex player_mutex_;
  std::atomic<int> id_{0};
  static std::once_flag flag_;
  static PlayerManager *instance_;
};

class NativeWindowManager {
public:
  struct NativeWindow {
    std::string id;
    OH_NativeXComponent *nativeXComponent{nullptr};
    void *nativeWindow{nullptr};
  };

  void addNativeXComponent(std::shared_ptr<NativeWindow> &component);

  std::shared_ptr<NativeWindow>
  getNativeXComponent(const std::string &playerId);

  void removeNativeXComponent(const std::string &playerId);

  ~NativeWindowManager();

  static NativeWindowManager *getInstance();

private:
  NativeWindowManager();

  std::map<std::string, std::shared_ptr<NativeWindow>> windows_;

  std::mutex window_mutex;

  static std::once_flag flag_;
  static NativeWindowManager *instance_;
};
