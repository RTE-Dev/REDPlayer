#if defined(__ANDROID__)
#pragma once

#include <jni.h>

#include "../../audio_info.h"
#include "../audio_track_common.h"

NS_REDRENDER_AUDIO_BEGIN

class AudioTrackJni {
public:
  AudioTrackJni() = default;
  ~AudioTrackJni() = default;
  static jobject AudioTrackAsGlobalRefCatchAll(
      JNIEnv *env, jint streamType, jint sample_rate_in_hz, jint channelConfig,
      jint audioFormat, jint bufferSizeInBytes, jint mode);
  static jint AudioTrackGetMinBufferSizeCatchAll(JNIEnv *env,
                                                 jint sample_rate_in_hz,
                                                 jint channelConfig,
                                                 jint audioFormat);
  static jfloat AudioTrackGetMaxVolumeCatchAll(JNIEnv *env);
  static jfloat AudioTrackGetMinVolumeCatchAll(JNIEnv *env);
  static jint AudioTrackGetNativeOutputSampleRateCatchAll(JNIEnv *env,
                                                          jint streamType);
  static void AudioTrackPlayCatchAll(JNIEnv *env, jobject thiz);
  static void AudioTrackPauseCatchAll(JNIEnv *env, jobject thiz);
  static void AudioTrackStopCatchAll(JNIEnv *env, jobject thiz);
  static void AudioTrackFlushCatchAll(JNIEnv *env, jobject thiz);
  static void AudioTrackReleaseCatchAll(JNIEnv *env, jobject thiz);
  static jint AudioTrackWriteCatchAll(JNIEnv *env, jobject thiz,
                                      jbyteArray audio_data,
                                      jint offset_in_bytes, jint size_in_bytes);
  static jint AudioTrackSetStereoVolumeCatchAll(JNIEnv *env, jobject thiz,
                                                jfloat left_gain,
                                                jfloat right_gain);
  static jint AudioTrackGetAudioSessionIdCatchAll(JNIEnv *env, jobject thiz);
  static jobject AudioTrackGetPlaybackParams__asGlobalRefCatchAll(JNIEnv *env,
                                                                  jobject thiz);
  static void AudioTrackSetPlaybackParamsCatchAll(JNIEnv *env, jobject thiz,
                                                  jobject params);
  static jint AudioTrackGetStreamTypeCatchAll(JNIEnv *env, jobject thiz);
  static jint AudioTrackGetSampleRateCatchAll(JNIEnv *env, jobject thiz);
  static jint AudioTrackGetPlaybackRateCatchAll(JNIEnv *env, jobject thiz);
  static jint AudioTrackSetPlaybackRateCatchAll(JNIEnv *env, jobject thiz,
                                                jint sample_rate_in_hz);
  static void AudioTracksetSpeedCatchAll(JNIEnv *env, jobject thiz,
                                         jfloat speed);
  static jint AudioTrackGetLatencyCatchAll(JNIEnv *env, jobject thiz);
  static int LoadClass(JNIEnv *env);

private:
  static jclass audio_track_class_;
  static jmethodID constructor_audio_track_;
  static jmethodID method_get_min_buffer_size_;
  static jmethodID method_get_max_volume_;
  static jmethodID method_get_min_volume_;
  static jmethodID method_get_native_output_sample_rate_;
  static jmethodID method_play_;
  static jmethodID method_pause_;
  static jmethodID method_stop_;
  static jmethodID method_flush_;
  static jmethodID method_release_;
  static jmethodID method_write_;
  static jmethodID method_set_stereo_volume_;
  static jmethodID method_get_audio_session_id_;
  static jmethodID method_get_playback_params_;
  static jmethodID method_set_playback_params_;
  static jmethodID method_get_stream_type_;
  static jmethodID method_get_sample_rate_;
  static jmethodID method_get_playback_rate_;
  static jmethodID method_set_playback_rate_;

  static jclass media_playback_params_class_;
  static jmethodID method_playback_params_set_speed_;

  static jmethodID method_get_latency_;
};
NS_REDRENDER_AUDIO_END
#endif
