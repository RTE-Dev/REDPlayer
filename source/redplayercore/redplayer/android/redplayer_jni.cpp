#if defined(__ANDROID__)

#include <android/log.h>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <jni.h>
#include <map>
#include <pthread.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/log.h"
#ifdef __cplusplus
}
#endif

#include "Interface/RedPlayer.h"
#include "RedLog.h"
#include "RedMeta.h"
#include "android/redplayer_android_def.h"
#include "jni/ArrayList.h"
#include "jni/Bundle.h"
#include "jni/Debug.h"
#include "jni/Env.h"
#include "jni/Util.h"

#include "RedCore/module/sourcer/format/redioapplication.h"
#include "redrender/audio/android/jni/audio_track_jni.h"
#include "wrapper/reddownload_datasource_wrapper.h"

#define TAG "RedPlayerJNI"
#define JNI_CLASS_REDPLAYER_INTERFACE                                          \
  "com/xingin/openredplayercore/core/impl/redplayer/RedMediaPlayer"

using redPlayer_ns::cfgTypeFormat;
using redPlayer_ns::cfgTypePlayer;
using redPlayer_ns::CRedPlayer;
using redPlayer_ns::globalInit;
using redPlayer_ns::globalSetInjectCallback;
using redPlayer_ns::globalUninit;
using redPlayer_ns::Message;
using redPlayer_ns::setLogCallback;
using redPlayer_ns::setLogCallbackLevel;

static std::mutex *g_lock = new std::mutex();
static std::map<int, sp<CRedPlayer>> *g_map = new std::map<int, sp<CRedPlayer>>;

struct fields_t {
  jclass clazz;
  jfieldID log_cb_level;
  jmethodID post_event;
  jmethodID native_log;
};
static fields_t g_fields;

#define JNI_CHECK_MPRET_VOID(retval, env)                                      \
  JNI_CHECK_RET_VOID((retval != INVALID_OPERATION), env,                       \
                     JAVA_ILLEGAL_STATE_EXCEPTION, nullptr);                   \
  JNI_CHECK_RET_VOID((retval != NO_MEMORY), env, JAVA_OUT_OF_MEMORY_ERROR,     \
                     nullptr);                                                 \
  JNI_CHECK_RET_VOID((retval == OK), env, JAVA_RUNTIME_EXCEPTION, "ret != OK");

static int inject_callback(void *opaque, int what, void *data) { return 0; }

static void jniOnNativeLog(JNIEnv *env, jint level, jstring tag,
                           jbyteArray bytes) {
  return env->CallStaticVoidMethod(g_fields.clazz, g_fields.native_log, level,
                                   tag, bytes);
}

inline static void onNativeLog(JNIEnv *env, jint level, jstring tag,
                               jbyteArray bytes) {
  return jniOnNativeLog(env, level, tag, bytes);
}

static void redLogCallback(int level, const char *tag, const char *buffer) {
  JNIEnv *env = nullptr;
  jstring jtag = nullptr;
  std::unique_ptr<jni::JniEnvPtr> env_ptr = std::make_unique<jni::JniEnvPtr>();
  if (!env_ptr) {
    return;
  }
  env = env_ptr->env();
  if (!env || !buffer) {
    return;
  }
  jbyteArray bytes = JniNewByteArrayGlobalRefCatchAll(env, strlen(buffer));
  if (!bytes) {
    return;
  }
  env->SetByteArrayRegion(bytes, 0, strlen(buffer), (jbyte *)buffer);
  if (JniCheckExceptionCatchAll(env)) {
    goto fail;
  }
  jtag = JniNewStringUTFCatchAll(env, tag);
  if (JniCheckExceptionCatchAll(env) || !jtag) {
    goto fail;
  }
  onNativeLog(env, level, jtag, bytes);
fail:
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&jtag));
  JniDeleteGlobalRefP(env, reinterpret_cast<jobject *>(&bytes));
}

static void jniPostEventFromNative(JNIEnv *env, jobject weak_this, jlong ctime,
                                   int what, int arg1, int arg2, jobject obj) {
  if (!weak_this) {
    AV_LOGW(TAG, "%s null weak_this\n", __func__);
    return;
  }

  env->CallStaticVoidMethod(g_fields.clazz, g_fields.post_event, weak_this,
                            ctime, what, arg1, arg2, obj);
  JniCheckAndLogException(env);
}

inline static void postEvent(JNIEnv *env, jobject weak_this, jlong ctime,
                             int what, int arg1, int arg2) {
  jniPostEventFromNative(env, weak_this, ctime, what, arg1, arg2, nullptr);
}

inline static void postEvent(JNIEnv *env, jobject weak_this, jlong ctime,
                             int what, int arg1, int arg2, jobject obj) {
  jniPostEventFromNative(env, weak_this, ctime, what, arg1, arg2, obj);
}

static void messageLoopN(JNIEnv *env, CRedPlayer *mp) {
  if (!mp) {
    AV_LOGE(TAG, "%s null mp\n", __func__);
    return;
  }
  jobject weak_thiz = static_cast<jobject>(mp->getWeakThiz());

  while (1) {
    sp<Message> msg = mp->getMessage(true);
    if (!msg)
      break;

    int64_t time = msg->mTime;
    int32_t arg1 = msg->mArg1, arg2 = msg->mArg2;
    void *obj1 = msg->mObj1, *obj2 = msg->mObj2;

    switch (msg->mWhat) {
    case RED_MSG_FLUSH:
      AV_LOGV_ID(TAG, mp->id(), "RED_MSG_FLUSH\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_NOP, 0, 0);
      break;
    case RED_MSG_ERROR:
      AV_LOGE_ID(TAG, mp->id(), "RED_MSG_ERROR: (%d, %d)\n", arg1, arg2);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_ERROR, arg1,
                arg2);
      break;
    case RED_MSG_BPREPARED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_BPREPARED\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_BPREPARED, 0,
                0);
      break;
    case RED_MSG_PREPARED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_PREPARED\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_PREPARED, 0, 0);
      break;
    case RED_MSG_COMPLETED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_COMPLETED\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time),
                MEDIA_PLAYBACK_COMPLETE, 0, 0);
      break;
    case RED_MSG_VIDEO_SIZE_CHANGED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_SIZE_CHANGED: %dx%d\n", arg1,
                 arg2);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_SET_VIDEO_SIZE,
                arg1, arg2);
      break;
    case RED_MSG_SAR_CHANGED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_SAR_CHANGED: %d/%d\n", arg1, arg2);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_SET_VIDEO_SAR,
                arg1, arg2);
      break;
    case RED_MSG_VIDEO_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_RENDERING_START\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_VIDEO_RENDERING_START, 0);
      break;
    case RED_MSG_AUDIO_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_AUDIO_RENDERING_START\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_AUDIO_RENDERING_START, 0);
      break;
    case RED_MSG_VIDEO_ROTATION_CHANGED:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_ROTATION_CHANGED: %d\n", arg1);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_VIDEO_ROTATION_CHANGED, arg1);
      break;
    case RED_MSG_AUDIO_DECODED_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_AUDIO_DECODED_START\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_AUDIO_DECODED_START, 0);
      break;
    case RED_MSG_VIDEO_DECODED_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_DECODED_START\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_VIDEO_DECODED_START, 0);
      break;
    case RED_MSG_OPEN_INPUT:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_OPEN_INPUT\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_OPEN_INPUT, 0);
      break;
    case RED_MSG_FIND_STREAM_INFO:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_FIND_STREAM_INFO\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_FIND_STREAM_INFO, 0);
      break;
    case RED_MSG_COMPONENT_OPEN:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_COMPONENT_OPEN\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_COMPONENT_OPEN, 0);
      break;
    case RED_MSG_BUFFERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_BUFFERING_START: %d\n", arg1);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_BUFFERING_START, arg1);
      break;
    case RED_MSG_BUFFERING_END:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_BUFFERING_END: %d\n", arg1);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_BUFFERING_END, arg1);
      break;
    case RED_MSG_BUFFERING_UPDATE:
      postEvent(env, weak_thiz, static_cast<jlong>(time),
                MEDIA_BUFFERING_UPDATE, arg1, arg2);
      break;
    case RED_MSG_BUFFERING_BYTES_UPDATE:
      break;
    case RED_MSG_BUFFERING_TIME_UPDATE:
      break;
    case RED_MSG_SEEK_COMPLETE:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_SEEK_COMPLETE: %d %d\n", arg1, arg2);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_SEEK_COMPLETE,
                arg1, arg2);
      break;
    case RED_MSG_ACCURATE_SEEK_COMPLETE:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_ACCURATE_SEEK_COMPLETE: %d\n", arg1);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_MEDIA_ACCURATE_SEEK_COMPLETE, arg1);
      break;
    case RED_MSG_SEEK_LOOP_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_SEEK_LOOP_START: %d\n", arg1);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_MEDIA_SEEK_LOOP_COMPLETE, arg1);
      break;
    case RED_MSG_PLAYBACK_STATE_CHANGED:
      break;
    case RED_MSG_TIMED_TEXT:
      if (obj1) {
        jstring text = JniNewStringUTFCatchAll(env, static_cast<char *>(obj1));
        postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_TIMED_TEXT, 0,
                  0, text);
        JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&text));
      } else {
        postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_TIMED_TEXT, 0,
                  0, nullptr);
      }
      break;
    case RED_MSG_VIDEO_SEEK_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_SEEK_RENDERING_START: %d\n",
                 arg1);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_VIDEO_SEEK_RENDERING_START, arg1);
      break;

    case RED_MSG_AUDIO_SEEK_RENDERING_START:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_AUDIO_SEEK_RENDERING_START: %d\n",
                 arg1);
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_AUDIO_SEEK_RENDERING_START, arg1);
      break;
    case RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_VIDEO_FIRST_PACKET_IN_DECODER, 0);
      break;
    case RED_MSG_VIDEO_START_ON_PLAYING:
      AV_LOGD_ID(TAG, mp->id(), "RED_MSG_VIDEO_START_ON_PLAYING\n");
      postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                MEDIA_INFO_VIDEO_START_ON_PLAYING, 0);
      break;
    case RED_MSG_URL_CHANGE:
      if (obj1 && obj2) {
        AV_LOGD_ID(TAG, mp->id(), "RED_MSG_URL_CHANGE\n");

        jstring current_url =
            JniNewStringUTFCatchAll(env, static_cast<char *>(obj1));
        jstring next_url =
            JniNewStringUTFCatchAll(env, static_cast<char *>(obj2));
        postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                  MEDIA_INFO_URL_CHANGE, arg1, current_url);
        postEvent(env, weak_thiz, static_cast<jlong>(time), MEDIA_INFO,
                  MEDIA_INFO_URL_CHANGE, -1, next_url);
        JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&current_url));
        JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&next_url));
      }
      break;
    default:
      AV_LOGW_ID(TAG, mp->id(), "unknown RED_MSG_xxx(%d):\n", msg->mWhat);
      break;
    }
    mp->recycleMessage(msg);
  }
}

static RED_ERR messageLoop(CRedPlayer *mp) {
  if (!mp) {
    AV_LOGE(TAG, "%s: null mp\n", __func__);
    return ME_ERROR;
  }

  std::unique_ptr<jni::JniEnvPtr> env = std::make_unique<jni::JniEnvPtr>();
  if (!env || !env->env()) {
    AV_LOGE(TAG, "%s: null env\n", __func__);
    return NO_MEMORY;
  }

  AV_LOGI_ID(TAG, mp->id(), "%s start\n", __func__);

  messageLoopN(env->env(), mp);

  AV_LOGI_ID(TAG, mp->id(), "%s exit\n", __func__);
  return OK;
}

static sp<CRedPlayer> setRedPlayer(JNIEnv *env, jobject thiz,
                                   sp<CRedPlayer> &mp) {
  int id = 0;
  sp<CRedPlayer> old;

  jfieldID jid = JniGetFieldIdCatchAll(env, g_fields.clazz, "mID", "I");
  if (!jid) {
    AV_LOGW("%s Failed to get mID\n", __func__);
    return old;
  }
  id = static_cast<int>(env->GetIntField(thiz, jid));
  JniCheckAndLogException(env);

  CHECK_GT(id, 0);

  if (id <= 0) {
    AV_LOGW(TAG, "%s Invalid player id!\n", __func__);
    return old;
  }

  Autolock lck(*g_lock);
  auto it = (*g_map).find(id);
  if (it != (*g_map).end()) {
    old = it->second;
    (*g_map).erase(it);
  }
  if (mp) {
    (*g_map).emplace(id, mp);
  }

  return old;
}

static sp<CRedPlayer> getRedPlayer(JNIEnv *env, jobject thiz) {
  int id = 0;
  sp<CRedPlayer> mp;

  jfieldID jid = JniGetFieldIdCatchAll(env, g_fields.clazz, "mID", "I");
  if (!jid) {
    AV_LOGW("%s Failed to get mID\n", __func__);
    return mp;
  }
  id = static_cast<int>(env->GetIntField(thiz, jid));
  JniCheckAndLogException(env);

  CHECK_GT(id, 0);

  if (id <= 0) {
    AV_LOGW(TAG, "%s Invalid player id!\n", __func__);
    return mp;
  }

  Autolock lck(*g_lock);
  auto it = (*g_map).find(id);
  if (it != (*g_map).end()) {
    mp = it->second;
  }

  return mp;
}

static jstring RedPlayer_getVideoCodecInfo(JNIEnv *env, jobject thiz) {
  jstring jcodec_info = nullptr;
  RED_ERR ret = OK;
  std::string codec_info;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);

  JNI_CHECK_RET(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                "mpjni: getVideoCodecInfo: null mp", jcodec_info);

  ret = mp->getVideoCodecInfo(codec_info);
  if (ret != OK || codec_info.empty())
    return jcodec_info;

  jcodec_info = JniNewStringUTFCatchAll(env, codec_info);

  return jcodec_info;
}

static jstring RedPlayer_getAudioCodecInfo(JNIEnv *env, jobject thiz) {
  jstring jcodec_info = nullptr;
  RED_ERR ret = OK;
  std::string codec_info;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);

  JNI_CHECK_RET(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                "mpjni: getAudioCodecInfo: null mp", jcodec_info);

  ret = mp->getAudioCodecInfo(codec_info);
  if (ret != OK || codec_info.empty())
    return jcodec_info;

  jcodec_info = JniNewStringUTFCatchAll(env, codec_info);

  return jcodec_info;
}

static jstring RedPlayer_getPlayUrl(JNIEnv *env, jobject thiz) {
  jstring jurl = nullptr;
  RED_ERR ret = OK;
  std::string url;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);

  JNI_CHECK_RET(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                "mpjni: getPlayUrl: null mp", jurl);

  ret = mp->getPlayUrl(url);
  if (ret != OK || url.empty())
    return jurl;

  jurl = JniNewStringUTFCatchAll(env, url);

  return jurl;
}

static jint RedPlayer_getPlayerState(JNIEnv *env, jobject thiz) {
  int state = -1;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);

  JNI_CHECK_RET(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                "mpjni: getPlayerState: null mp", state);
  state = mp->getPlayerState();

  return state;
}

static void RedPlayer_setLoopCount(JNIEnv *env, jobject thiz, jint loop_count) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);

  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setLoopCount: null mp");

  mp->setLoop(loop_count);
}

static jint RedPlayer_getLoopCount(JNIEnv *env, jobject thiz) {
  jint loop_count = 1;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);

  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getLoopCount: null mp", loop_count);

  loop_count = mp->getLoop();

  return loop_count;
}

static void RedPlayer_native_init(JNIEnv *env, jobject thiz) {
  jclass clazz;

  clazz = JniGetClassCatchAll(env, JNI_CLASS_REDPLAYER_INTERFACE);
  CHECK(clazz);
  if (!clazz) {
    AV_LOGW("%s Failed to get class\n", __func__);
    return;
  }
  g_fields.clazz = static_cast<jclass>(JniNewGlobalRefCatchAll(env, clazz));

  g_fields.log_cb_level =
      JniGetStaticFieldIdCatchAll(env, clazz, "gLogCallBackLevel", "I");
  CHECK(g_fields.log_cb_level);
  if (!g_fields.log_cb_level) {
    AV_LOGW("%s Failed to get gLogCallBackLevel\n", __func__);
    return;
  }
  jint level = env->GetStaticIntField(g_fields.clazz, g_fields.log_cb_level);
  if (JniCheckExceptionCatchAll(env)) {
    level = 0;
  }
  setLogCallbackLevel(static_cast<int>(level));

  g_fields.post_event = JniGetStaticClassMethodCatchAll(
      env, clazz, "postEventFromNative",
      "(Ljava/lang/Object;JIIILjava/lang/Object;)V");
  if (!g_fields.post_event) {
    AV_LOGW("%s Failed to get post_event method\n", __func__);
    return;
  }

  g_fields.native_log = JniGetStaticClassMethodCatchAll(
      env, clazz, "onNativeLog", "(ILjava/lang/String;[B)V");
  if (!g_fields.native_log) {
    AV_LOGW("%s Failed to get native_log method\n", __func__);
    return;
  }
  setLogCallback(redLogCallback);

  JniDeleteLocalRef(env, clazz);
}

static void RedPlayer_native_setup(JNIEnv *env, jobject thiz,
                                   jobject weak_this) {
  int id = 0;
  jfieldID jid = JniGetFieldIdCatchAll(env, g_fields.clazz, "mID", "I");
  if (!jid) {
    AV_LOGW("%s Failed to get mID\n", __func__);
    return;
  }
  id = static_cast<int>(env->GetIntField(thiz, jid));
  JniCheckAndLogException(env);
  CHECK_GT(id, 0);
  if (id <= 0) {
    AV_LOGW(TAG, "%s Invalid player id!\n", __func__);
    return;
  }
  sp<CRedPlayer> mp =
      CRedPlayer::Create(id, std::bind(&messageLoop, std::placeholders::_1));
  JNI_CHECK_RET_VOID(mp, env, JAVA_OUT_OF_MEMORY_ERROR,
                     "mpjni: native_setup: mp oom");
  setRedPlayer(env, thiz, mp);
  mp->setWeakThiz(static_cast<void *>(JniNewGlobalRefCatchAll(env, weak_this)));
  mp->setInjectOpaque(mp->getWeakThiz());
}

static void RedPlayer_setVideoSurface(JNIEnv *env, jobject thiz,
                                      jobject jsurface) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setVideoSurface: null mp");

  mp->setVideoSurface(env, jsurface);
}

static void RedPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(path, env, JAVA_ILLEGAL_ARGUMENT_EXCEPTION,
                     "mpjni: setDataSource: null path");
  JNI_CHECK_RET_VOID(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                     "mpjni: setDataSource: null mp");
  std::string c_path = JniGetStringUTFCharsCatchAll(env, path);
  JNI_CHECK_RET_VOID(!c_path.empty(), env, JAVA_OUT_OF_MEMORY_ERROR,
                     "mpjni: setDataSource: path.string oom");
  mp->setDataSource(c_path);
}

static void RedPlayer_setDataSourceJson(JNIEnv *env, jobject thiz,
                                        jstring json) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(json, env, JAVA_ILLEGAL_ARGUMENT_EXCEPTION,
                     "mpjni: setDataSource: null json");
  JNI_CHECK_RET_VOID(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                     "mpjni: setDataSource: null mp");
  std::string c_json = JniGetStringUTFCharsCatchAll(env, json);
  JNI_CHECK_RET_VOID(!c_json.empty(), env, JAVA_OUT_OF_MEMORY_ERROR,
                     "mpjni: setDataSource: json.string oom");
  mp->setConfig(cfgTypePlayer, "is-input-json", 2);
  mp->setConfig(cfgTypeFormat, "enable-url-list-fallback", 1);
  mp->setDataSource(c_json);
}

static void RedPlayer_prepareAsync(JNIEnv *env, jobject thiz) {
  RED_ERR ret = 0;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);

  JNI_CHECK_RET_VOID(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                     "mpjni: prepareAsync: null mp");

  ret = mp->prepareAsync();
  JNI_CHECK_RET_VOID(!ret, env, JAVA_ILLEGAL_ARGUMENT_EXCEPTION,
                     "mpjni: prepareAsync: fail");
}

static void RedPlayer_start(JNIEnv *env, jobject thiz) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                     "mpjni: start: null mp");
  mp->start();
}

static void RedPlayer_stop(JNIEnv *env, jobject thiz) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                     "mpjni: stop: null mp");
  mp->stop();
}

static void RedPlayer_pause(JNIEnv *env, jobject thiz) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                     "mpjni: pause: null mp");
  mp->pause();
}

static jboolean RedPlayer_isPlaying(JNIEnv *env, jobject thiz) {
  jboolean retval = JNI_FALSE;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: isPlaying: null mp", retval);

  retval = mp->isPlaying() ? JNI_TRUE : JNI_FALSE;

  return retval;
}

static void RedPlayer_seekTo(JNIEnv *env, jobject thiz, jlong msec) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, JAVA_ILLEGAL_STATE_EXCEPTION,
                     "mpjni: seekTo: null mp");
  mp->seekTo(msec);
}

static jlong RedPlayer_getCurrentPosition(JNIEnv *env, jobject thiz) {
  jlong cur_pos = 0;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getCurrentPosition: null mp", 0);
  mp->getCurrentPosition(cur_pos);
  return cur_pos;
}

static jlong RedPlayer_getDuration(JNIEnv *env, jobject thiz) {
  jlong duration = 0;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getDuration: null mp", 0);
  mp->getDuration(duration);
  return duration;
}

static void RedPlayer_release(JNIEnv *env, jobject thiz) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  if (!mp) {
    AV_LOGW(TAG, "%s RedPlayer already went away\n", __func__);
    return;
  }
  mp->stop();
  mp->setVideoSurface(env, nullptr);
  mp->setInjectOpaque(nullptr);
  jobject weak_thiz = static_cast<jobject>(mp->setWeakThiz(nullptr));
  sp<CRedPlayer> empty = nullptr;
  setRedPlayer(env, thiz, empty);
  mp->release();
  JniDeleteGlobalRef(env, weak_thiz);
}

static void RedPlayer_reset(JNIEnv *env, jobject thiz) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  if (!mp) {
    AV_LOGW(TAG, "%s RedPlayer already went away\n", __func__);
    return;
  }
  jobject weak_thiz = static_cast<jobject>(mp->setWeakThiz(nullptr));

  RedPlayer_release(env, thiz);
  RedPlayer_native_setup(env, thiz, weak_thiz);
}

static void RedPlayer_setVolume(JNIEnv *env, jobject thiz, jfloat left_volume,
                                jfloat right_volume) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setVolume: null mp");
  mp->setVolume(left_volume, right_volume);
}

static void RedPlayer_native_setCallbackLogLevel(JNIEnv *env, jclass clazz,
                                                 jint level) {
  setLogCallbackLevel(static_cast<int>(level));
}

static void RedPlayer_setEnableMediaCodec(JNIEnv *env, jobject thiz,
                                          jboolean enable) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setEnableMediaCodec: null mp");

  mp->setConfig(cfgTypePlayer, "mediacodec-all-videos",
                static_cast<int64_t>(enable));
  mp->setConfig(cfgTypePlayer, "enable-ndkvdec", static_cast<int64_t>(enable));
}

static void RedPlayer_setVideoCacheDir(JNIEnv *env, jobject thiz, jstring dir) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(dir, env, nullptr, "mpjni: setVideoCacheDir: null dir");
  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setVideoCacheDir: null mp");

  std::string c_dir = JniGetStringUTFCharsCatchAll(env, dir);
  JNI_CHECK_RET_VOID(!c_dir.empty(), env, nullptr,
                     "mpjni: setVideoCacheDir: dir.string oom");
  mp->setConfig(cfgTypeFormat, "cache_file_dir", c_dir);
}

static jfloat RedPlayer_getVideoFileFps(JNIEnv *env, jobject thiz) {
  jfloat ret = 0.0;
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getVideoFileFps: null mp", ret);

  ret = static_cast<jfloat>(
      mp->getProp(RED_PROP_FLOAT_VIDEO_FILE_FRAME_RATE, 0.0f));
  return ret;
}

static void RedPlayer_setHeaders(JNIEnv *env, jobject thiz, jstring headers) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(headers, env, nullptr, "mpjni: setHeaders: null headers");
  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setHeaders: null mp");

  std::string c_headers = JniGetStringUTFCharsCatchAll(env, headers);
  JNI_CHECK_RET_VOID(!c_headers.empty(), env, nullptr,
                     "mpjni: setHeaders: headers.string oom");
  mp->setConfig(cfgTypeFormat, "headers", c_headers);
}

static void RedPlayer_setSpeed(JNIEnv *env, jobject thiz, jfloat speed) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setSpeed: null mp");

  mp->setProp(RED_PROP_FLOAT_PLAYBACK_RATE, speed);
}

static jfloat RedPlayer_getSpeed(JNIEnv *env, jobject thiz,
                                 jfloat default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getSpeed: null mp", default_value);

  return static_cast<jfloat>(mp->getProp(RED_PROP_FLOAT_PLAYBACK_RATE,
                                         static_cast<float>(default_value)));
}

static jint RedPlayer_getVideoDecoder(JNIEnv *env, jobject thiz,
                                      jint default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getVideoDecoder: null mp",
                default_value);

  return static_cast<jint>(mp->getProp(RED_PROP_INT64_VIDEO_DECODER,
                                       static_cast<int64_t>(default_value)));
}

static jfloat RedPlayer_getVideoOutputFramesPerSecond(JNIEnv *env, jobject thiz,
                                                      jfloat default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr,
                "mpjni: getVideoOutputFramesPerSecond: null mp", default_value);

  return static_cast<jfloat>(
      mp->getProp(RED_PROP_FLOAT_VIDEO_OUTPUT_FRAMES_PER_SECOND,
                  static_cast<float>(default_value)));
}

static jfloat RedPlayer_getVideoDecodeFramesPerSecond(JNIEnv *env, jobject thiz,
                                                      jfloat default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr,
                "mpjni: getVideoDecodeFramesPerSecond: null mp", default_value);

  return static_cast<jfloat>(
      mp->getProp(RED_PROP_FLOAT_VIDEO_DECODE_FRAMES_PER_SECOND,
                  static_cast<float>(default_value)));
}

static jfloat RedPlayer_getDropFrameRate(JNIEnv *env, jobject thiz,
                                         jfloat default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getDropFrameRate: null mp",
                default_value);

  return static_cast<jfloat>(mp->getProp(RED_PROP_FLOAT_DROP_FRAME_RATE,
                                         static_cast<float>(default_value)));
}

static jfloat RedPlayer_getDropPacketRateBeforeDecode(JNIEnv *env, jobject thiz,
                                                      jfloat default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr,
                "mpjni: getDropPacketRateBeforeDecode: null mp", default_value);

  return static_cast<jfloat>(
      mp->getProp(RED_PROP_FLOAT_DROP_PACKET_RATE_BEFORE_DECODE,
                  static_cast<float>(default_value)));
}

static jlong RedPlayer_getVideoCachedSizeMs(JNIEnv *env, jobject thiz,
                                            jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getVideoCachedSizeMs: null mp",
                default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_VIDEO_CACHED_DURATION,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getAudioCachedSizeMs(JNIEnv *env, jobject thiz,
                                            jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getAudioCachedSizeMs: null mp",
                default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_AUDIO_CACHED_DURATION,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getVideoCachedSizeBytes(JNIEnv *env, jobject thiz,
                                               jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getVideoCachedSizeBytes: null mp",
                default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_VIDEO_CACHED_BYTES,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getAudioCachedSizeBytes(JNIEnv *env, jobject thiz,
                                               jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getAudioCachedSizeBytes: null mp",
                default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_AUDIO_CACHED_BYTES,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getVideoCachedSizePackets(JNIEnv *env, jobject thiz,
                                                 jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getVideoCachedSizePackets: null mp",
                default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_VIDEO_CACHED_PACKETS,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getAudioCachedSizePackets(JNIEnv *env, jobject thiz,
                                                 jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getAudioCachedSizePackets: null mp",
                default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_AUDIO_CACHED_PACKETS,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getTrafficStatisticByteCount(JNIEnv *env, jobject thiz,
                                                    jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr,
                "mpjni: getTrafficStatisticByteCount: null mp", default_value);

  return static_cast<jlong>(
      mp->getProp(RED_PROP_INT64_TRAFFIC_STATISTIC_BYTE_COUNT,
                  static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getRealCacheBytes(JNIEnv *env, jobject thiz,
                                         jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getRealCacheBytes: null mp",
                default_value);

  return static_cast<jlong>(
      mp->getProp(RED_PROP_INT64_CACHED_STATISTIC_REAL_BYTE_COUNT,
                  static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getFileSize(JNIEnv *env, jobject thiz,
                                   jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getFileSize: null mp", default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_LOGICAL_FILE_SIZE,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getBitRate(JNIEnv *env, jobject thiz,
                                  jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getBitRate: null mp", default_value);

  return static_cast<jlong>(mp->getProp(RED_PROP_INT64_BIT_RATE,
                                        static_cast<int64_t>(default_value)));
}

static jlong RedPlayer_getSeekCostTimeMs(JNIEnv *env, jobject thiz,
                                         jlong default_value) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET(mp, env, nullptr, "mpjni: getSeekCostTimeMs: null mp",
                default_value);

  return static_cast<jlong>(
      mp->getProp(RED_PROP_INT64_LATEST_SEEK_LOAD_DURATION,
                  static_cast<int64_t>(default_value)));
}

static void RedPlayer_setLiveMode(JNIEnv *env, jobject thiz, jboolean enable) {
  sp<CRedPlayer> mp = getRedPlayer(env, thiz);
  JNI_CHECK_RET_VOID(mp, env, nullptr, "mpjni: setLiveMode: null mp");

  if (enable) {
    mp->setConfig(cfgTypeFormat, "load_file", 0);
    mp->setConfig(cfgTypeFormat, "connect_timeout", 0);
  }
}

static JNINativeMethod g_methods[] = {
    {"native_init", "()V", reinterpret_cast<void *>(RedPlayer_native_init)},
    {"native_setup", "(Ljava/lang/Object;)V",
     reinterpret_cast<void *>(RedPlayer_native_setup)},
    {"_setVideoSurface", "(Landroid/view/Surface;)V",
     reinterpret_cast<void *>(RedPlayer_setVideoSurface)},
    {"_setDataSource", "(Ljava/lang/String;)V",
     reinterpret_cast<void *>(RedPlayer_setDataSource)},
    {"_setDataSourceJson", "(Ljava/lang/String;)V",
     reinterpret_cast<void *>(RedPlayer_setDataSourceJson)},
    {"_prepareAsync", "()V", reinterpret_cast<void *>(RedPlayer_prepareAsync)},
    {"_start", "()V", reinterpret_cast<void *>(RedPlayer_start)},
    {"_stop", "()V", reinterpret_cast<void *>(RedPlayer_stop)},
    {"_seekTo", "(J)V", reinterpret_cast<void *>(RedPlayer_seekTo)},
    {"_pause", "()V", reinterpret_cast<void *>(RedPlayer_pause)},
    {"isPlaying", "()Z", reinterpret_cast<void *>(RedPlayer_isPlaying)},
    {"_getCurrentPosition", "()J",
     reinterpret_cast<void *>(RedPlayer_getCurrentPosition)},
    {"_getDuration", "()J", reinterpret_cast<void *>(RedPlayer_getDuration)},
    {"_release", "()V", reinterpret_cast<void *>(RedPlayer_release)},
    {"_reset", "()V", reinterpret_cast<void *>(RedPlayer_reset)},
    {"_getVideoCodecInfo", "()Ljava/lang/String;",
     reinterpret_cast<void *>(RedPlayer_getVideoCodecInfo)},
    {"_getAudioCodecInfo", "()Ljava/lang/String;",
     reinterpret_cast<void *>(RedPlayer_getAudioCodecInfo)},
    {"_getPlayUrl", "()Ljava/lang/String;",
     reinterpret_cast<void *>(RedPlayer_getPlayUrl)},
    {"_getPlayerState", "()I",
     reinterpret_cast<void *>(RedPlayer_getPlayerState)},
    {"_setLoopCount", "(I)V", reinterpret_cast<void *>(RedPlayer_setLoopCount)},
    {"_getLoopCount", "()I", reinterpret_cast<void *>(RedPlayer_getLoopCount)},
    {"_setVolume", "(FF)V", reinterpret_cast<void *>(RedPlayer_setVolume)},
    {"native_setCallbackLogLevel", "(I)V",
     reinterpret_cast<void *>(RedPlayer_native_setCallbackLogLevel)},
    {"_setEnableMediaCodec", "(Z)V",
     reinterpret_cast<void *>(RedPlayer_setEnableMediaCodec)},
    {"_setVideoCacheDir", "(Ljava/lang/String;)V",
     reinterpret_cast<void *>(RedPlayer_setVideoCacheDir)},
    {"_getVideoFileFps", "()F",
     reinterpret_cast<void *>(RedPlayer_getVideoFileFps)},
    {"_setHeaders", "(Ljava/lang/String;)V",
     reinterpret_cast<void *>(RedPlayer_setHeaders)},
    {"_setSpeed", "(F)V", reinterpret_cast<void *>(RedPlayer_setSpeed)},
    {"_getSpeed", "(F)F", reinterpret_cast<void *>(RedPlayer_getSpeed)},
    {"_getVideoDecoder", "(I)I",
     reinterpret_cast<void *>(RedPlayer_getVideoDecoder)},
    {"_getVideoOutputFramesPerSecond", "(F)F",
     reinterpret_cast<void *>(RedPlayer_getVideoOutputFramesPerSecond)},
    {"_getVideoDecodeFramesPerSecond", "(F)F",
     reinterpret_cast<void *>(RedPlayer_getVideoDecodeFramesPerSecond)},
    {"_getDropFrameRate", "(F)F",
     reinterpret_cast<void *>(RedPlayer_getDropFrameRate)},
    {"_getDropPacketRateBeforeDecode", "(F)F",
     reinterpret_cast<void *>(RedPlayer_getDropPacketRateBeforeDecode)},
    {"_getVideoCachedSizeMs", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getVideoCachedSizeMs)},
    {"_getAudioCachedSizeMs", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getAudioCachedSizeMs)},
    {"_getVideoCachedSizeBytes", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getVideoCachedSizeBytes)},
    {"_getAudioCachedSizeBytes", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getAudioCachedSizeBytes)},
    {"_getVideoCachedSizePackets", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getVideoCachedSizePackets)},
    {"_getAudioCachedSizePackets", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getAudioCachedSizePackets)},
    {"_getTrafficStatisticByteCount", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getTrafficStatisticByteCount)},
    {"_getRealCacheBytes", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getRealCacheBytes)},
    {"_getFileSize", "(J)J", reinterpret_cast<void *>(RedPlayer_getFileSize)},
    {"_getBitRate", "(J)J", reinterpret_cast<void *>(RedPlayer_getBitRate)},
    {"_getSeekCostTimeMs", "(J)J",
     reinterpret_cast<void *>(RedPlayer_getSeekCostTimeMs)},
    {"_setLiveMode", "(Z)V", reinterpret_cast<void *>(RedPlayer_setLiveMode)}};

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *jvm, void *reserved) {
  JNIEnv *env;
  if (JNI_OK != jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
    return JNI_ERR;
  }

  jni::JniEnvPtr::GlobalInit(jvm);
  redrender::audio::AudioTrackJni::LoadClass(env);

  jclass clazz = env->FindClass(JNI_CLASS_REDPLAYER_INTERFACE);

  if (env->RegisterNatives(clazz, g_methods,
                           sizeof(g_methods) / sizeof((g_methods)[0])) < 0) {
    return JNI_ERR;
  }

  globalInit();
  globalSetInjectCallback(inject_callback);

  return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNI_OnUnload(JavaVM *jvm, void *reserved) {
  JNIEnv *env;
  if (JNI_OK != jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
    return;
  }

  globalUninit();
  JniDeleteGlobalRef(env, g_fields.clazz);
}

#endif
