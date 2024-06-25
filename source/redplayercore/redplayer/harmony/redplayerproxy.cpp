#include "ace/xcomponent/native_interface_xcomponent.h"

#include "RedCore/module/sourcer/format/redioapplication.h"
#include "RedLog.h"
#include "RedMsg.h"
#include "preload_event_dispatcher.h"
#include "redplayer_event_dispatcher.h"
#include "redplayer_harmony_def.h"
#include "redplayerproxy.h"
#include "wrapper/reddownload_datasource_wrapper.h"

#define TAG "RedPlayerProxy"

PlayerManager *PlayerManager::instance_{nullptr};
std::once_flag PlayerManager::flag_;
NativeWindowManager *NativeWindowManager::instance_{nullptr};
std::once_flag NativeWindowManager::flag_;

void PlayerManager::addPlayer(std::shared_ptr<RedPlayerProxy> &player) {
  std::unique_lock<std::mutex> _(player_mutex_);
  players_[player->id] = player;
}

std::shared_ptr<RedPlayerProxy>
PlayerManager::getPlayer(const std::string &playerId) {
  std::unique_lock<std::mutex> _(player_mutex_);
  auto it = players_.find(playerId);
  if (it == players_.end()) {
    return nullptr;
  }
  return it->second;
}

void PlayerManager::removePlayer(const std::string &playerId) {
  std::unique_lock<std::mutex> _(player_mutex_);
  players_.erase(playerId);
}

int PlayerManager::addAndGetId() {
  std::unique_lock<std::mutex> _(player_mutex_);
  id_++;
  return id_.load();
}

PlayerManager::~PlayerManager() {
  std::unique_lock<std::mutex> _(player_mutex_);
  players_.clear();
}

PlayerManager::PlayerManager() {}

PlayerManager *PlayerManager::getInstance() {
  std::call_once(flag_, []() { instance_ = new PlayerManager(); });
  return instance_;
}

void NativeWindowManager::addNativeXComponent(
    std::shared_ptr<NativeWindowManager::NativeWindow> &window) {
  std::unique_lock<std::mutex> _(window_mutex);
  windows_[window->id] = window;
}

std::shared_ptr<NativeWindowManager::NativeWindow>
NativeWindowManager::getNativeXComponent(const std::string &playerId) {
  std::unique_lock<std::mutex> _(window_mutex);
  auto it = windows_.find(playerId);
  if (it == windows_.end()) {
    return nullptr;
  }
  return it->second;
}

void NativeWindowManager::removeNativeXComponent(const std::string &playerId) {
  std::unique_lock<std::mutex> _(window_mutex);
  windows_.erase(playerId);
}

NativeWindowManager::~NativeWindowManager() {
  std::unique_lock<std::mutex> _(window_mutex);
  windows_.clear();
}

NativeWindowManager::NativeWindowManager() {}

NativeWindowManager *NativeWindowManager::getInstance() {
  std::call_once(flag_, []() { instance_ = new NativeWindowManager(); });
  return instance_;
}

void RedPlayerOnSurfaceCreatedCallback(OH_NativeXComponent *component,
                                       void *window) {
  char name[OH_XCOMPONENT_ID_LEN_MAX] = {0};
  uint64_t name_len = OH_XCOMPONENT_ID_LEN_MAX;
  if (component != nullptr) {
    OH_NativeXComponent_GetXComponentId(component, name, &name_len);
  }
  std::shared_ptr<NativeWindowManager::NativeWindow> temp =
      std::make_shared<NativeWindowManager::NativeWindow>();
  temp->nativeWindow = window;
  temp->id = name;
  temp->nativeXComponent = component;
  NativeWindowManager::getInstance()->addNativeXComponent(temp);
}

void RedPlayerOnSurfaceChangedCallback(OH_NativeXComponent *component,
                                       void *window) {}

void RedPlayerOnSurfaceDestroyedCallback(OH_NativeXComponent *component,
                                         void *window) {
  char name[OH_XCOMPONENT_ID_LEN_MAX] = {0};
  uint64_t name_len = OH_XCOMPONENT_ID_LEN_MAX;
  if (component != nullptr) {
    OH_NativeXComponent_GetXComponentId(component, name, &name_len);
  }
  NativeWindowManager::getInstance()->removeNativeXComponent(name);
}

void RedPlayerDispatchTouchEventCallback(OH_NativeXComponent *component,
                                         void *window) {}

void RedPlayerProxy::messageLoopN(redPlayer_ns::CRedPlayer *mp) {
  if (!mp) {
    AV_LOGE(TAG, "%s null mp\n", __func__);
    return;
  }

  while (1) {
    sp<redPlayer_ns::Message> msg = mp->getMessage(true);
    if (!msg)
      break;

    int64_t time = msg->mTime;
    int32_t arg1 = msg->mArg1, arg2 = msg->mArg2;
    napi_value params = nullptr;
    switch (msg->mWhat) {
    case RED_MSG_FLUSH:
      AV_LOGV_ID(TAG, mp->id(), "RED_MSG_FLUSH\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_NOP, 0, 0, params);
      break;
    case RED_MSG_ERROR:
      AV_LOGE_ID(TAG, mp->id(), "RED_MSG_ERROR: (%d, %d)\n", arg1, arg2);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_ERROR, arg1, arg2, params);
      break;
    case RED_MSG_BPREPARED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_BPREPARED\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_BPREPARED, 0, 0, params);
      break;
    case RED_MSG_PREPARED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_PREPARED\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_PREPARED, 0, 0, params);
      break;
    case RED_MSG_COMPLETED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_COMPLETED\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_PLAYBACK_COMPLETE, 0, 0,
                               params);
      break;
    case RED_MSG_VIDEO_SIZE_CHANGED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_SIZE_CHANGED: %dx%d\n", arg1,
                 arg2);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_SET_VIDEO_SIZE, arg1, arg2,
                               params);
      break;
    case RED_MSG_SAR_CHANGED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_SAR_CHANGED: %d/%d\n", arg1, arg2);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_SET_VIDEO_SAR, arg1, arg2,
                               params);
      break;
    case RED_MSG_VIDEO_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_RENDERING_START: %d\n", arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_VIDEO_RENDERING_START, arg1, params);
      break;
    case RED_MSG_AUDIO_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_AUDIO_RENDERING_START\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_AUDIO_RENDERING_START, 0, params);
      break;
    case RED_MSG_VIDEO_ROTATION_CHANGED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_ROTATION_CHANGED: %d\n", arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_VIDEO_ROTATION_CHANGED, arg1, params);
      break;
    case RED_MSG_AUDIO_DECODED_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_AUDIO_DECODED_START\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_AUDIO_DECODED_START, 0, params);
      break;
    case RED_MSG_VIDEO_DECODED_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_DECODED_START\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_VIDEO_DECODED_START, 0, params);
      break;
    case RED_MSG_OPEN_INPUT:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_OPEN_INPUT\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_OPEN_INPUT, 0, params);
      break;
    case RED_MSG_FIND_STREAM_INFO:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_FIND_STREAM_INFO\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_FIND_STREAM_INFO, 0, params);
      break;
    case RED_MSG_COMPONENT_OPEN:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_COMPONENT_OPEN\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_COMPONENT_OPEN, 0, params);
      break;
    case RED_MSG_BUFFERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_BUFFERING_START: %d\n", arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_BUFFERING_START, arg1, params);
      break;
    case RED_MSG_BUFFERING_END:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_BUFFERING_END: %d\n", arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_BUFFERING_END, arg1, params);
      break;
    case RED_MSG_BUFFERING_UPDATE:
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_BUFFERING_UPDATE, arg1,
                               arg2, params);
      break;
    case RED_MSG_BUFFERING_BYTES_UPDATE:
      break;
    case RED_MSG_BUFFERING_TIME_UPDATE:
      break;
    case RED_MSG_SEEK_COMPLETE:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_SEEK_COMPLETE: %d %d\n", arg1, arg2);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_SEEK_COMPLETE, arg1, arg2,
                               params);
      break;
    case RED_MSG_ACCURATE_SEEK_COMPLETE:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_ACCURATE_SEEK_COMPLETE: %d\n", arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_MEDIA_ACCURATE_SEEK_COMPLETE, arg1,
                               params);
      break;
    case RED_MSG_SEEK_LOOP_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_SEEK_LOOP_START: %d\n", arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_MEDIA_SEEK_LOOP_COMPLETE, arg1,
                               params);
      break;
    case RED_MSG_PLAYBACK_STATE_CHANGED:
      break;
    case RED_MSG_TIMED_TEXT:
      break;
    case RED_MSG_VIDEO_SEEK_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_SEEK_RENDERING_START: %d\n",
                 arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_VIDEO_SEEK_RENDERING_START, arg1,
                               params);
      break;
    case RED_MSG_AUDIO_SEEK_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_AUDIO_SEEK_RENDERING_START: %d\n",
                 arg1);
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_AUDIO_SEEK_RENDERING_START, arg1,
                               params);
      break;
    case RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_VIDEO_FIRST_PACKET_IN_DECODER, 0,
                               params);
      break;
    case RED_MSG_VIDEO_START_ON_PLAYING:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_START_ON_PLAYING\n");
      RedPlayerCallbackOnEvent(this->id, time, MEDIA_INFO,
                               MEDIA_INFO_VIDEO_START_ON_PLAYING, 0, params);
      break;
    case RED_MSG_URL_CHANGE:
      break;
    default:
      AV_LOGW_ID(TAG, mp->id(), "unknown RED_MSG_xxx(%d):\n", msg->mWhat);
      break;
    }
    mp->recycleMessage(msg);
  }
}

RED_ERR RedPlayerProxy::messageLoop(redPlayer_ns::CRedPlayer *mp) {
  if (!mp) {
    AV_LOGE(TAG, "%s: null mp\n", __func__);
    return ME_ERROR;
  }

  AV_LOGI_ID(TAG, mp->id(), "%s start\n", __func__);
  messageLoopN(mp);
  AV_LOGI_ID(TAG, mp->id(), "%s exit\n", __func__);
  return OK;
}

RED_ERR RedPlayerProxy::setConfig(int type, std::string name,
                                  std::string value) {
  if (player_entity_) {
    return player_entity_->setConfig(type, name, value);
  }
  return ME_ERROR;
}

RED_ERR RedPlayerProxy::setConfig(int type, std::string name, int64_t value) {
  if (player_entity_) {
    return player_entity_->setConfig(type, name, value);
  }
  return ME_ERROR;
}

static napi_value SetConfigStr(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // type
  int64_t type;
  napi_get_value_int64(env, args[1], &type);
  // name:
  char name[512] = {0};
  size_t nameLen = 0;
  napi_get_value_string_utf8(env, args[2], name, sizeof(name), &nameLen);
  std::string nameStr(name);
  // type
  char value[512] = {0};
  size_t valueLen = 0;
  napi_get_value_string_utf8(env, args[3], value, sizeof(value), &valueLen);
  std::string valueStr(value);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    player->setConfig(type, nameStr, valueStr);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value SetConfigInt(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // type
  int64_t type;
  napi_get_value_int64(env, args[1], &type);
  // name:
  char name[512] = {0};
  size_t nameLen = 0;
  napi_get_value_string_utf8(env, args[2], name, sizeof(name), &nameLen);
  std::string nameStr(name);
  // type
  int64_t value;
  napi_get_value_int64(env, args[3], &value);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    player->setConfig(type, nameStr, value);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

napi_value RedPlayerProxy::createPlayer(napi_env env, napi_callback_info info) {
  player_entity_ = redPlayer_ns::CRedPlayer::Create(
      PlayerManager::getInstance()->addAndGetId(),
      std::bind(&RedPlayerProxy::messageLoop, this, std::placeholders::_1));
  if (player_entity_ != nullptr) {
    x_component_callback_.OnSurfaceCreated = RedPlayerOnSurfaceCreatedCallback;
    x_component_callback_.OnSurfaceChanged = RedPlayerOnSurfaceChangedCallback;
    x_component_callback_.OnSurfaceDestroyed =
        RedPlayerOnSurfaceDestroyedCallback;
    x_component_callback_.DispatchTouchEvent =
        RedPlayerDispatchTouchEventCallback;

    player_entity_->setWeakThiz(this);
    player_entity_->setInjectOpaque(player_entity_->getWeakThiz());

    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value CreatePlayer(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player = std::make_shared<RedPlayerProxy>();
  if (player != nullptr) {
    player->id = playerIdStr;
    PlayerManager::getInstance()->addPlayer(player);
    return player->createPlayer(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::setSurfaceId(napi_env env, napi_callback_info info,
                                        std::string surfaceId) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s surfaceId:%s\n", __func__,
               surfaceId.c_str());
    void *window = NativeWindowManager::getInstance()
                       ->getNativeXComponent(surfaceId)
                       ->nativeWindow;
    player_entity_->setVideoSurface(reinterpret_cast<OHNativeWindow *>(window));
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value SetSurfaceId(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // surfaceId:
  char surfaceId[512] = {0};
  size_t surfaceIdLen = 0;
  napi_get_value_string_utf8(env, args[1], surfaceId, sizeof(surfaceId),
                             &surfaceIdLen);
  std::string surfaceIdStr(surfaceId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->setSurfaceId(env, info, surfaceIdStr);
  }
  return nullptr;
}

napi_value RedPlayerProxy::setDataSource(napi_env env, napi_callback_info info,
                                         std::string url) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s url:%s\n", __func__, url.c_str());
    player_entity_->setDataSource(url);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value SetDataSource(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // url:
  char url[512] = {0};
  size_t urlLen = 0;
  napi_get_value_string_utf8(env, args[1], url, sizeof(url), &urlLen);
  std::string urlStr(url);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->setDataSource(env, info, urlStr);
  }
  return nullptr;
}

napi_value RedPlayerProxy::setDataSourceFd(napi_env env,
                                           napi_callback_info info,
                                           int64_t fd) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s fd:%" PRId64 "\n", __func__, fd);
    std::map<std::string, std::string> headers;
    player_entity_->setDataSourceFd(fd);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value SetDataSourceFd(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // fd:
  int64_t fd;
  napi_get_value_int64(env, args[1], &fd);

  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->setDataSourceFd(env, info, fd);
  }
  return nullptr;
}

napi_value RedPlayerProxy::prepare(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s \n", __func__);
    player_entity_->prepareAsync();
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value Prepare(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->prepare(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::seek(napi_env env, napi_callback_info info,
                                int64_t seekPosition) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s seekPosition:%" PRId64 "\n",
               __func__, seekPosition);
    player_entity_->seekTo(seekPosition);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value Seek(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // seekPosition:
  int64_t seekPosition;
  napi_get_value_int64(env, args[1], &seekPosition);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->seek(env, info, seekPosition);
  }
  return nullptr;
}

napi_value RedPlayerProxy::setSpeed(napi_env env, napi_callback_info info,
                                    double speed) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s speed:%lf\n", __func__, speed);
    player_entity_->setProp(RED_PROP_FLOAT_PLAYBACK_RATE,
                            static_cast<float>(speed));
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value SetSpeed(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // speed:
  double speed;
  napi_get_value_double(env, args[1], &speed);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->setSpeed(env, info, speed);
  }
  return nullptr;
}

napi_value RedPlayerProxy::setVolume(napi_env env, napi_callback_info info,
                                     double volume) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s volume:%lf\n", __func__, volume);
    player_entity_->setVolume(volume, volume);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value SetVolume(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // volume:
  double volume;
  napi_get_value_double(env, args[1], &volume);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->setVolume(env, info, volume);
  }
  return nullptr;
}

napi_value RedPlayerProxy::setLoop(napi_env env, napi_callback_info info,
                                   int64_t loop) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s loop:%" PRId64 "\n", __func__,
               loop);
    player_entity_->setLoop(loop);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value SetLoop(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // loop:
  int64_t loop;
  napi_get_value_int64(env, args[1], &loop);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->setLoop(env, info, loop);
  }
  return nullptr;
}

napi_value RedPlayerProxy::start(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s \n", __func__);
    player_entity_->start();
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value Start(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->start(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::pause(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s \n", __func__);
    player_entity_->pause();
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value Pause(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->pause(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::stop(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s \n", __func__);
    player_entity_->stop();
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value Stop(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->stop(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::release(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s \n", __func__);
    player_entity_->stop();
    player_entity_->setVideoSurface(nullptr);
    player_entity_->setInjectOpaque(nullptr);
    player_entity_->setWeakThiz(nullptr);
    player_entity_->release();
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value Release(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    player->release(env, info);
  }
  PlayerManager::getInstance()->removePlayer(playerIdStr);
  return nullptr;
}

napi_value RedPlayerProxy::reset(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    AV_LOGD_ID(TAG, player_entity_->id(), "%s \n", __func__);
    release(env, info);
    createPlayer(env, info);
    napi_value ret;
    napi_create_int32(env, 0, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value Reset(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    player->reset(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getCurrentPosition(napi_env env,
                                              napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int64_t pos;
    player_entity_->getCurrentPosition(pos);
    napi_value ret;
    napi_create_int64(env, pos, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetCurrentPosition(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getCurrentPosition(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getVideoDuration(napi_env env,
                                            napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int64_t dur;
    player_entity_->getDuration(dur);
    napi_value ret;
    napi_create_int64(env, dur, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetVideoDuration(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getVideoDuration(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getVideoWidth(napi_env env,
                                         napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int32_t width;
    player_entity_->getWidth(width);
    napi_value ret;
    napi_create_int32(env, width, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetVideoWidth(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getVideoWidth(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getVideoHeight(napi_env env,
                                          napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int32_t height;
    player_entity_->getHeight(height);
    napi_value ret;
    napi_create_int32(env, height, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetVideoHeight(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getVideoHeight(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getUrl(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    std::string url;
    player_entity_->getPlayUrl(url);
    napi_value ret;
    napi_create_string_utf8(env, url.c_str(), url.length(), &ret);
    return ret;
  }
  return nullptr;
}

napi_value RedPlayerProxy::getAudioCodecInfo(napi_env env,
                                             napi_callback_info info) {
  if (player_entity_ != nullptr) {
    std::string info;
    player_entity_->getAudioCodecInfo(info);
    napi_value ret;
    napi_create_string_utf8(env, info.c_str(), info.length(), &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetAudioCodecInfo(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getAudioCodecInfo(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getVideoCodecInfo(napi_env env,
                                             napi_callback_info info) {
  if (player_entity_ != nullptr) {
    std::string info;
    player_entity_->getVideoCodecInfo(info);
    napi_value ret;
    napi_create_string_utf8(env, info.c_str(), info.length(), &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetVideoCodecInfo(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getVideoCodecInfo(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::isPlaying(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int isPlaying = player_entity_->isPlaying();
    napi_value ret;
    napi_create_int32(env, isPlaying, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value IsPlaying(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->isPlaying(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::isLooping(napi_env env, napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int isLooping = player_entity_->getLoop() > 1;
    napi_value ret;
    napi_create_int32(env, isLooping, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value IsLooping(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->isLooping(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getVideoCachedSizeBytes(napi_env env,
                                                   napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int64_t default_value = 0;
    int64_t bytes = player_entity_->getProp(RED_PROP_INT64_VIDEO_CACHED_BYTES,
                                            default_value);
    napi_value ret;
    napi_create_int64(env, bytes, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetVideoCachedSizeBytes(napi_env env,
                                          napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getVideoCachedSizeBytes(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getAudioCachedSizeBytes(napi_env env,
                                                   napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int64_t default_value = 0;
    int64_t bytes = player_entity_->getProp(RED_PROP_INT64_AUDIO_CACHED_BYTES,
                                            default_value);
    napi_value ret;
    napi_create_int64(env, bytes, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetAudioCachedSizeBytes(napi_env env,
                                          napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getAudioCachedSizeBytes(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getVideoCachedSizeMs(napi_env env,
                                                napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int64_t default_value = 0;
    int64_t bytes = player_entity_->getProp(
        RED_PROP_INT64_VIDEO_CACHED_DURATION, default_value);
    napi_value ret;
    napi_create_int64(env, bytes, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetVideoCachedSizeMs(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getVideoCachedSizeMs(env, info);
  }
  return nullptr;
}

napi_value RedPlayerProxy::getAudioCachedSizeMs(napi_env env,
                                                napi_callback_info info) {
  if (player_entity_ != nullptr) {
    int64_t default_value = 0;
    int64_t bytes = player_entity_->getProp(
        RED_PROP_INT64_AUDIO_CACHED_DURATION, default_value);
    napi_value ret;
    napi_create_int64(env, bytes, &ret);
    return ret;
  }
  return nullptr;
}

static napi_value GetAudioCachedSizeMs(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getAudioCachedSizeMs(env, info);
  }
  return nullptr;
}

static napi_value GetUrl(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // playerId:
  char playerId[512] = {0};
  size_t playerIdLen = 0;
  napi_get_value_string_utf8(env, args[0], playerId, sizeof(playerId),
                             &playerIdLen);
  std::string playerIdStr(playerId);
  // method call:
  std::shared_ptr<RedPlayerProxy> player =
      PlayerManager::getInstance()->getPlayer(playerIdStr);
  if (player != nullptr) {
    return player->getUrl(env, info);
  }
  return nullptr;
}

static int RedInjectCallback(void *opaque, int what, void *data) { return 0; }

napi_env g_preload_env;
static napi_value PreloadGlobalInit(napi_env env, napi_callback_info info) {
  g_preload_env = env;
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // path:
  char path[512] = {0};
  size_t pathLen = 0;
  napi_get_value_string_utf8(env, args[0], path, sizeof(path), &pathLen);
  std::string pathStr(path);
  // maxSize:
  int64_t maxSize;
  napi_get_value_int64(env, args[1], &maxSize);
  // threadPoolSize:
  int64_t threadPoolSize;
  napi_get_value_int64(env, args[2], &threadPoolSize);
  // maxDirSize:
  int64_t maxDirSize;
  napi_get_value_int64(env, args[3], &maxDirSize);
  DownLoadOptWrapper opt;
  reddownload_datasource_wrapper_opt_reset(&opt);
  opt.cache_file_dir = path;
  opt.cache_max_entries = maxSize;
  if (maxDirSize > 0)
    opt.cache_max_dir_capacity = maxDirSize;
  opt.threadpool_size = threadPoolSize;
  reddownload_datasource_wrapper_init(&opt);
  return nullptr;
}

void EventCbWrapperImpl(void *opaque, int val, void *a1, void *a2) {
  AV_LOGI(TAG, "%s event = 0x%x\n", __func__, val);
  if (opaque == nullptr)
    return;
  PreloadObject *preloadObject = (PreloadObject *)opaque;
  switch (val) {
  case RED_EVENT_DID_FRAGMENT_COMPLETE_W: {
    if (a1 == nullptr)
      return;
    RedIOTrafficWrapper *trafficEvent = (RedIOTrafficWrapper *)a1;
    Preload_callOnEvent(g_preload_env, preloadObject->instanceId, val,
                        trafficEvent->url);
  } break;
  case RED_EVENT_DID_RELEASE_W: {
    Preload_callOnEvent(g_preload_env, preloadObject->instanceId, val, "");
    if (preloadObject) {
      delete preloadObject;
      preloadObject = nullptr;
    }
  } break;
  case RED_EVENT_WILL_DNS_PARSE_W:
  case RED_EVENT_DID_DNS_PARSE_W: {
    if (a1 == nullptr)
      return;
    RedDnsInfoWrapper *dnsInfo = (RedDnsInfoWrapper *)a1;
    Preload_callOnEvent(g_preload_env, preloadObject->instanceId, val,
                        dnsInfo->domain);
  } break;
  case RED_EVENT_WILL_HTTP_OPEN_W: {
    Preload_callOnEvent(g_preload_env, preloadObject->instanceId, val, "");
  } break;
  case RED_EVENT_DID_HTTP_OPEN_W: {
    if (a1 == nullptr)
      return;
    RedHttpEventWrapper *httpEvent = (RedHttpEventWrapper *)a1;
    Preload_callOnEvent(g_preload_env, preloadObject->instanceId, val,
                        httpEvent->url);
  } break;
  case RED_CTRL_DID_TCP_OPEN_W: {
    if (a1 == nullptr)
      return;
    RedTcpIOControlWrapper *tcpEvent = (RedTcpIOControlWrapper *)a1;
    Preload_callOnEvent(g_preload_env, preloadObject->instanceId, val,
                        tcpEvent->url);
  } break;
  case RED_EVENT_ERROR_W: {
    if (a1 == nullptr)
      return;
    RedErrorEventWrapper *errorEvent = (RedErrorEventWrapper *)a1;
    Preload_callOnEvent(g_preload_env, preloadObject->instanceId, val,
                        errorEvent->url);
  } break;
  default:
    break;
  }
}

static napi_value PreloadOpen(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // instanceId:
  char instanceId[512] = {0};
  size_t instanceIdLen = 0;
  napi_get_value_string_utf8(env, args[0], instanceId, sizeof(instanceId),
                             &instanceIdLen);
  std::string instanceIdStr(instanceId);
  // url:
  char url[512] = {0};
  size_t urlLen = 0;
  napi_get_value_string_utf8(env, args[1], url, sizeof(url), &urlLen);
  std::string urlStr(url);
  // path:
  char path[512] = {0};
  size_t pathLen = 0;
  napi_get_value_string_utf8(env, args[2], path, sizeof(path), &pathLen);
  std::string pathStr(path);
  // downsize:
  int64_t downsize;
  napi_get_value_int64(env, args[3], &downsize);
  DownLoadOptWrapper opt;
  reddownload_datasource_wrapper_opt_reset(&opt);
  PreloadObject *preloadObj = new PreloadObject();
  preloadObj->instanceId = instanceIdStr;
  opt.cache_file_dir = pathStr.c_str();
  opt.PreDownLoadSize = downsize;
  DownLoadCbWrapper wrapper = {0};
  wrapper.appctx = (void *)preloadObj;
  wrapper.ioappctx = wrapper.appctx;
  wrapper.appcb = EventCbWrapperImpl;
  wrapper.ioappcb = nullptr;
  wrapper.urlcb = nullptr;
  wrapper.speedcb = nullptr;
  reddownload_datasource_wrapper_open(urlStr.c_str(), &wrapper, &opt);
  return nullptr;
}

static napi_value PreloadOpenJson(napi_env env, napi_callback_info info) {
  size_t argc = 4;
  napi_value args[4] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // instanceId:
  char instanceId[512] = {0};
  size_t instanceIdLen = 0;
  napi_get_value_string_utf8(env, args[0], instanceId, sizeof(instanceId),
                             &instanceIdLen);
  std::string instanceIdStr(instanceId);
  // urlJson:
  char urlJson[512] = {0};
  size_t urlJsonLen = 0;
  napi_get_value_string_utf8(env, args[1], urlJson, sizeof(urlJson),
                             &urlJsonLen);
  std::string urlJsonStr(urlJson);
  // path:
  char path[512] = {0};
  size_t pathLen = 0;
  napi_get_value_string_utf8(env, args[2], path, sizeof(path), &pathLen);
  std::string pathStr(path);
  // downsize:
  int64_t downsize;
  napi_get_value_int64(env, args[3], &downsize);

  DownLoadOptWrapper opt;
  reddownload_datasource_wrapper_opt_reset(&opt);
  PreloadObject *preloadObj = new PreloadObject();
  preloadObj->instanceId = instanceIdStr;
  opt.cache_file_dir = pathStr.c_str();
  opt.PreDownLoadSize = downsize;
  DownLoadCbWrapper wrapper = {0};
  wrapper.appctx = (void *)preloadObj;
  wrapper.ioappctx = wrapper.appctx;
  wrapper.appcb = EventCbWrapperImpl;
  wrapper.ioappcb = nullptr;
  wrapper.urlcb = nullptr;
  wrapper.speedcb = nullptr;
  reddownload_datasource_wrapper_open(urlJsonStr.c_str(), &wrapper, &opt);
  return nullptr;
}

static napi_value PreloadRelease(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  // instanceId:
  char url[512] = {0};
  size_t urlLen = 0;
  napi_get_value_string_utf8(env, args[0], url, sizeof(url), &urlLen);
  (void)reddownload_datasource_wrapper_close(url, 0);
  return nullptr;
}

static napi_value PreloadStop(napi_env env, napi_callback_info info) {
  reddownload_datasource_wrapper_stop(NULL, 0);
  return nullptr;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor desc[] = {
      // player control function part:
      {"createPlayer", nullptr, CreatePlayer, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setSurfaceId", nullptr, SetSurfaceId, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setDataSource", nullptr, SetDataSource, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setDataSourceFd", nullptr, SetDataSourceFd, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setConfigStr", nullptr, SetConfigStr, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"setConfigInt", nullptr, SetConfigInt, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"prepare", nullptr, Prepare, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"seek", nullptr, Seek, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"setSpeed", nullptr, SetSpeed, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"setVolume", nullptr, SetVolume, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"setLoop", nullptr, SetLoop, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"start", nullptr, Start, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"pause", nullptr, Pause, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"stop", nullptr, Stop, nullptr, nullptr, nullptr, napi_default, nullptr},
      {"release", nullptr, Release, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"reset", nullptr, Reset, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      // player get function part:
      {"getCurrentPosition", nullptr, GetCurrentPosition, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      {"getVideoDuration", nullptr, GetVideoDuration, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"getVideoWidth", nullptr, GetVideoWidth, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"getVideoHeight", nullptr, GetVideoHeight, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"getUrl", nullptr, GetUrl, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"getAudioCodecInfo", nullptr, GetAudioCodecInfo, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      {"getVideoCodecInfo", nullptr, GetVideoCodecInfo, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      {"isPlaying", nullptr, IsPlaying, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"isLooping", nullptr, IsLooping, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"getVideoCachedSizeBytes", nullptr, GetVideoCachedSizeBytes, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"getAudioCachedSizeBytes", nullptr, GetAudioCachedSizeBytes, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      {"getVideoCachedSizeMs", nullptr, GetVideoCachedSizeMs, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      {"getAudioCachedSizeMs", nullptr, GetAudioCachedSizeMs, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      // player event part:
      {"registerPlayerEventListener", nullptr, RegisterEventListener, nullptr,
       nullptr, nullptr, napi_default, nullptr},
      // player logger part:
      {"registerLogListener", nullptr, RegisterLogListener, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      // video preload part:
      {"preloadGlobalInit", nullptr, PreloadGlobalInit, nullptr, nullptr,
       nullptr, napi_default, nullptr},
      {"preloadRegisterCallback", nullptr, PreloadRegisterEventListener,
       nullptr, nullptr, nullptr, napi_default, nullptr},
      {"preloadOpen", nullptr, PreloadOpen, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"preloadOpenJson", nullptr, PreloadOpenJson, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"preloadRelease", nullptr, PreloadRelease, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"preloadStop", nullptr, PreloadStop, nullptr, nullptr, nullptr,
       napi_default, nullptr}};
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);

  OH_NativeXComponent *nativeXComponent = nullptr;
  static OH_NativeXComponent_Callback nativeXComponentCallback;
  napi_status status;
  napi_value exportInstance = nullptr;
  status = napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ,
                                   &exportInstance);
  if (status == napi_ok) {
    status = napi_unwrap(env, exportInstance,
                         reinterpret_cast<void **>(&nativeXComponent));
    if (status == napi_ok) {
      nativeXComponentCallback.OnSurfaceCreated =
          RedPlayerOnSurfaceCreatedCallback;
      nativeXComponentCallback.OnSurfaceChanged =
          RedPlayerOnSurfaceChangedCallback;
      nativeXComponentCallback.OnSurfaceDestroyed =
          RedPlayerOnSurfaceDestroyedCallback;
      nativeXComponentCallback.DispatchTouchEvent =
          RedPlayerDispatchTouchEventCallback;
      OH_NativeXComponent_RegisterCallback(nativeXComponent,
                                           &nativeXComponentCallback);
    }
  }
  RedLogSetLevel(AV_LEVEL_DEBUG);
  redPlayer_ns::setLogCallback(redLogCallback);
  redPlayer_ns::globalInit();
  redPlayer_ns::globalSetInjectCallback(RedInjectCallback);
  return exports;
}
EXTERN_C_END

static napi_module redplayerproxy = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "redplayerproxy",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterNativeModule(void) {
  napi_module_register(&redplayerproxy);
}
