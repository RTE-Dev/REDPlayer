#if defined(__ANDROID__)
#include "./audio_track_jni.h"

#include "jni/Env.h"
#include "jni/Util.h"

USING_NS_REDRENDER_AUDIO
jclass AudioTrackJni::audio_track_class_ = nullptr;
jmethodID AudioTrackJni::constructor_audio_track_ = nullptr;
jmethodID AudioTrackJni::method_get_min_buffer_size_ = nullptr;
jmethodID AudioTrackJni::method_get_max_volume_ = nullptr;
jmethodID AudioTrackJni::method_get_min_volume_ = nullptr;
jmethodID AudioTrackJni::method_get_native_output_sample_rate_ = nullptr;
jmethodID AudioTrackJni::method_play_ = nullptr;
jmethodID AudioTrackJni::method_pause_ = nullptr;
jmethodID AudioTrackJni::method_stop_ = nullptr;
jmethodID AudioTrackJni::method_flush_ = nullptr;
jmethodID AudioTrackJni::method_release_ = nullptr;
jmethodID AudioTrackJni::method_write_ = nullptr;
jmethodID AudioTrackJni::method_set_stereo_volume_ = nullptr;
jmethodID AudioTrackJni::method_get_audio_session_id_ = nullptr;
jmethodID AudioTrackJni::method_get_playback_params_ = nullptr;
jmethodID AudioTrackJni::method_set_playback_params_ = nullptr;
jmethodID AudioTrackJni::method_get_stream_type_ = nullptr;
jmethodID AudioTrackJni::method_get_sample_rate_ = nullptr;
jmethodID AudioTrackJni::method_get_playback_rate_ = nullptr;
jmethodID AudioTrackJni::method_set_playback_rate_ = nullptr;
jclass AudioTrackJni::media_playback_params_class_ = nullptr;
jmethodID AudioTrackJni::method_playback_params_set_speed_ = nullptr;
jmethodID AudioTrackJni::method_get_latency_ = nullptr;

int AudioTrackJni::LoadClass(JNIEnv *env) {
  if (!env) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] env null!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  if (audio_track_class_) {
    AV_LOGI(AUDIO_LOG_TAG, "[%s:%d] already loaded!\n", __FUNCTION__, __LINE__);
    return 0;
  }
  audio_track_class_ =
      JniGetClassGlobalRefCatchAll(env, "android/media/AudioTrack");
  if (!audio_track_class_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] FindClassAsGlobalRefCatchAll failed!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  constructor_audio_track_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "<init>", "(IIIIII)V");
  if (!constructor_audio_track_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] constructor_audio_track_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }
  method_get_min_buffer_size_ = JniGetStaticClassMethodCatchAll(
      env, audio_track_class_, "getMinBufferSize", "(III)I");
  if (!method_get_min_buffer_size_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] min_buff_size_id null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  method_get_max_volume_ = JniGetStaticClassMethodCatchAll(
      env, audio_track_class_, "getMaxVolume", "()F");
  if (!method_get_max_volume_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_max_volume_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  method_get_min_volume_ = JniGetStaticClassMethodCatchAll(
      env, audio_track_class_, "getMinVolume", "()F");
  if (!method_get_min_volume_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_min_volume_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  method_get_native_output_sample_rate_ = JniGetStaticClassMethodCatchAll(
      env, audio_track_class_, "getNativeOutputSampleRate", "(I)I");
  if (!method_get_native_output_sample_rate_) {
    AV_LOGE(AUDIO_LOG_TAG,
            "[%s:%d] method_get_native_output_sample_rate_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  method_play_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "play", "()V");
  if (!method_play_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_play_ null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  method_pause_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "pause", "()V");
  if (!method_pause_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_pause_ null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  method_stop_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "stop", "()V");
  if (!method_stop_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_stop_ null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  method_flush_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "flush", "()V");
  if (!method_flush_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_flush_ null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  method_release_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "release", "()V");
  if (!method_release_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_release_ null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  method_write_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "write", "([BII)I");
  if (!method_write_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_write_ null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  method_set_stereo_volume_ = JniGetClassMethodCatchAll(
      env, audio_track_class_, "setStereoVolume", "(FF)I");
  if (!method_set_stereo_volume_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_set_stereo_volume_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  method_get_audio_session_id_ = JniGetClassMethodCatchAll(
      env, audio_track_class_, "getAudioSessionId", "()I");
  if (!method_get_audio_session_id_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_audio_session_id_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  if (JniGetApiLevel() >= 23) {
    method_get_playback_params_ =
        JniGetClassMethodCatchAll(env, audio_track_class_, "getPlaybackParams",
                                  "()Landroid/media/PlaybackParams;");
    if (!method_get_playback_params_) {
      AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_playback_params_ null!\n",
              __FUNCTION__, __LINE__);
      return -1;
    }
  }

  if (JniGetApiLevel() >= 23) {
    method_set_playback_params_ =
        JniGetClassMethodCatchAll(env, audio_track_class_, "setPlaybackParams",
                                  "(Landroid/media/PlaybackParams;)V");
    if (!method_set_playback_params_) {
      AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_set_playback_params_ null!\n",
              __FUNCTION__, __LINE__);
      return -1;
    }
  }

  method_get_stream_type_ = JniGetClassMethodCatchAll(env, audio_track_class_,
                                                      "getStreamType", "()I");
  if (!method_get_stream_type_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_stream_type_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  method_get_sample_rate_ = JniGetClassMethodCatchAll(env, audio_track_class_,
                                                      "getSampleRate", "()I");
  if (!method_get_sample_rate_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_sample_rate_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  method_get_playback_rate_ = JniGetClassMethodCatchAll(
      env, audio_track_class_, "getPlaybackRate", "()I");
  if (!method_get_playback_rate_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_playback_rate_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  method_set_playback_rate_ = JniGetClassMethodCatchAll(
      env, audio_track_class_, "setPlaybackRate", "(I)I");
  if (!method_set_playback_rate_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_set_playback_rate_ null!\n",
            __FUNCTION__, __LINE__);
    return -1;
  }

  if (JniGetApiLevel() >= 23) {
    if (media_playback_params_class_) {
      AV_LOGI(AUDIO_LOG_TAG, "[%s:%d] already loaded!\n", __FUNCTION__,
              __LINE__);
      return 0;
    }
    media_playback_params_class_ =
        JniGetClassGlobalRefCatchAll(env, "android/media/PlaybackParams");
    if (!media_playback_params_class_) {
      AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] FindClassAsGlobalRefCatchAll failed!\n",
              __FUNCTION__, __LINE__);
      return -1;
    }
    method_playback_params_set_speed_ =
        JniGetClassMethodCatchAll(env, media_playback_params_class_, "setSpeed",
                                  "(F)Landroid/media/PlaybackParams;");
    if (!method_play_) {
      AV_LOGE(AUDIO_LOG_TAG,
              "[%s:%d] method_playback_params_set_speed_ null!\n", __FUNCTION__,
              __LINE__);
      return -1;
    }
  }

  method_get_latency_ =
      JniGetClassMethodCatchAll(env, audio_track_class_, "getLatency", "()I");
  if (!method_get_latency_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] method_get_latency_ null!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }

  AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] load successed!\n", __FUNCTION__, __LINE__);

  return 0;
}

jobject AudioTrackJni::AudioTrackAsGlobalRefCatchAll(
    JNIEnv *env, jint streamType, jint sample_rate_in_hz, jint channelConfig,
    jint audioFormat, jint bufferSizeInBytes, jint mode) {
  if (!env || !constructor_audio_track_ || !audio_track_class_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return nullptr;
  }
  jobject ret_value = env->NewObject(
      audio_track_class_, constructor_audio_track_, streamType,
      sample_rate_in_hz, channelConfig, audioFormat, bufferSizeInBytes, mode);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return nullptr;
  }
  jobject object_global = JniNewGlobalRefCatchAll(env, ret_value);
  if (!object_global) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] NewGlobalRefCatchAll falied!\n",
            __FUNCTION__, __LINE__);
    return nullptr;
  }

  JniDeleteLocalRefP(env, &ret_value);
  return object_global;
}
jint AudioTrackJni::AudioTrackGetMinBufferSizeCatchAll(JNIEnv *env,
                                                       jint sample_rate_in_hz,
                                                       jint channelConfig,
                                                       jint audioFormat) {
  if (!env || !method_get_min_buffer_size_ || !audio_track_class_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input![%d:%d:%d]\n", __FUNCTION__,
            __LINE__, !env, !method_get_min_buffer_size_, !audio_track_class_);
    return -1;
  }
  jint ret_value =
      env->CallStaticIntMethod(audio_track_class_, method_get_min_buffer_size_,
                               sample_rate_in_hz, channelConfig, audioFormat);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }
  return ret_value;
}
jfloat AudioTrackJni::AudioTrackGetMaxVolumeCatchAll(JNIEnv *env) {
  if (!env || !method_get_max_volume_ || !audio_track_class_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jfloat ret_value =
      env->CallStaticFloatMethod(audio_track_class_, method_get_max_volume_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }
  return ret_value;
}
jfloat AudioTrackJni::AudioTrackGetMinVolumeCatchAll(JNIEnv *env) {
  if (!env || !method_get_max_volume_ || !audio_track_class_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jfloat ret_value =
      env->CallStaticFloatMethod(audio_track_class_, method_get_max_volume_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }
  return ret_value;
}
jint AudioTrackJni::AudioTrackGetNativeOutputSampleRateCatchAll(
    JNIEnv *env, jint streamType) {
  if (!env || !method_get_native_output_sample_rate_ || !audio_track_class_) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jfloat ret_value = env->CallStaticIntMethod(
      audio_track_class_, method_get_native_output_sample_rate_, streamType);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }
  return ret_value;
}
void AudioTrackJni::AudioTrackPlayCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_play_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return;
  }
  env->CallVoidMethod(thiz, method_play_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallVoidMethod failed!\n", __FUNCTION__,
            __LINE__);
    return;
  }
}
void AudioTrackJni::AudioTrackPauseCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_pause_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return;
  }
  env->CallVoidMethod(thiz, method_pause_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallVoidMethod failed!\n", __FUNCTION__,
            __LINE__);
    return;
  }
}
void AudioTrackJni::AudioTrackStopCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_stop_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return;
  }
  env->CallVoidMethod(thiz, method_stop_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallVoidMethod failed!\n", __FUNCTION__,
            __LINE__);
    return;
  }
}
void AudioTrackJni::AudioTrackFlushCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_flush_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return;
  }
  env->CallVoidMethod(thiz, method_flush_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallVoidMethod failed!\n", __FUNCTION__,
            __LINE__);
    return;
  }
}
void AudioTrackJni::AudioTrackReleaseCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_release_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return;
  }
  env->CallVoidMethod(thiz, method_release_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallVoidMethod failed!\n", __FUNCTION__,
            __LINE__);
    return;
  }
}

jint AudioTrackJni::AudioTrackWriteCatchAll(JNIEnv *env, jobject thiz,
                                            jbyteArray audio_data,
                                            jint offset_in_bytes,
                                            jint size_in_bytes) {
  if (!env || !method_write_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jint ret_value = env->CallIntMethod(thiz, method_write_, audio_data,
                                      offset_in_bytes, size_in_bytes);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return -1;
  }
  return ret_value;
}
jint AudioTrackJni::AudioTrackSetStereoVolumeCatchAll(JNIEnv *env, jobject thiz,
                                                      jfloat left_gain,
                                                      jfloat right_gain) {
  if (!env || !method_set_stereo_volume_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jint ret_value = env->CallIntMethod(thiz, method_set_stereo_volume_,
                                      left_gain, right_gain);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return 0;
  }
  return ret_value;
}
jint AudioTrackJni::AudioTrackGetAudioSessionIdCatchAll(JNIEnv *env,
                                                        jobject thiz) {
  if (!env || !method_get_audio_session_id_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jint ret_value = env->CallIntMethod(thiz, method_get_audio_session_id_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return 0;
  }
  return ret_value;
}
jobject
AudioTrackJni::AudioTrackGetPlaybackParams__asGlobalRefCatchAll(JNIEnv *env,
                                                                jobject thiz) {
  if (!env || !method_get_playback_params_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return nullptr;
  }
  jobject ret_value = env->CallObjectMethod(thiz, method_get_playback_params_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return nullptr;
  }
  return ret_value;
}
void AudioTrackJni::AudioTrackSetPlaybackParamsCatchAll(JNIEnv *env,
                                                        jobject thiz,
                                                        jobject params) {
  if (!env || !method_set_playback_params_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return;
  }
  env->CallVoidMethod(thiz, method_set_playback_params_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return;
  }
}
jint AudioTrackJni::AudioTrackGetStreamTypeCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_get_stream_type_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jint ret_value = env->CallIntMethod(thiz, method_get_stream_type_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return 0;
  }
  return ret_value;
}
jint AudioTrackJni::AudioTrackGetSampleRateCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_get_stream_type_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jint ret_value = env->CallIntMethod(thiz, method_get_stream_type_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return 0;
  }
  return ret_value;
}
jint AudioTrackJni::AudioTrackGetPlaybackRateCatchAll(JNIEnv *env,
                                                      jobject thiz) {
  if (!env || !method_get_playback_rate_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jint ret_value = env->CallIntMethod(thiz, method_get_playback_rate_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return 0;
  }
  return ret_value;
}
jint AudioTrackJni::AudioTrackSetPlaybackRateCatchAll(JNIEnv *env, jobject thiz,
                                                      jint sample_rate_in_hz) {
  if (!env || method_set_playback_rate_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }
  jint ret_value =
      env->CallIntMethod(thiz, method_set_playback_rate_, sample_rate_in_hz);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return 0;
  }
  return ret_value;
}

void AudioTrackJni::AudioTracksetSpeedCatchAll(JNIEnv *env, jobject thiz,
                                               jfloat speed) {
  if (JniGetApiLevel() < 23) {
    jint sample_rate = env->CallIntMethod(thiz, method_get_sample_rate_);
    if (JniCheckException(env)) {
      AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
              __LINE__);
      return;
    }
    env->CallIntMethod(thiz, method_set_playback_rate_,
                       (jint)(sample_rate * speed));
    if (JniCheckExceptionCatchAll(env)) {
      AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
              __LINE__);
      return;
    }
    return;
  }

  jobject temp = nullptr;
  jobject params = env->CallObjectMethod(thiz, method_get_playback_params_);
  if (JniCheckException(env) || !params) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&params));
    return;
  }

  temp =
      env->CallObjectMethod(params, method_playback_params_set_speed_, speed);
  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&temp));
  if (JniCheckException(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&params));
    return;
  }

  env->CallVoidMethod(thiz, method_set_playback_params_, params);
  if (JniCheckException(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&params));
    return;
  }

  JniDeleteLocalRefP(env, reinterpret_cast<jobject *>(&params));
}

jint AudioTrackJni::AudioTrackGetLatencyCatchAll(JNIEnv *env, jobject thiz) {
  if (!env || !method_get_latency_ || !thiz) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] null input!\n", __FUNCTION__, __LINE__);
    return -1;
  }

  jint ret_value = env->CallIntMethod(thiz, method_get_latency_);
  if (JniCheckExceptionCatchAll(env)) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] CallIntMethod failed!\n", __FUNCTION__,
            __LINE__);
    return 0;
  }

  return ret_value;
}

#endif
