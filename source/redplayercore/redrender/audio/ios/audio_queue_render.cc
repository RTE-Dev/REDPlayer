#if defined(__APPLE__)
#include "./audio_queue_render.h"

#include "RedLog.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#include <iostream>

#include "../audio_common.h"
USING_NS_REDRENDER_AUDIO

AudioQueueRender::AudioQueueRender() : AudioQueueRender(0) {}
AudioQueueRender::AudioQueueRender(const int &session_id)
    : IAudioRender(session_id) {}
AudioQueueRender::~AudioQueueRender() {}

void AudioQueueRender::AdjustAudioInfo(const AudioInfo &desired,
                                       AudioInfo &obtained) {
  obtained = desired;
  if (desired.channels > 2) {
    obtained.channels = 2;
  }
  if (obtained.format != AudioFormat::kAudioS16Sys) {
    obtained.format = AudioFormat::kAudioS16Sys;
  }
  obtained.silence = 0x00;
}

void AudioQueueRender::AudioQueueOuptutCallback(void *user_data,
                                                AudioQueueRef aq,
                                                AudioQueueBufferRef buffer) {
  auto *aqr = reinterpret_cast<AudioQueueRender *>(user_data);
  if (!aqr) {
    AV_LOGE(AUDIO_LOG_TAG, "[%s:%d] aqr is null!\n", __FUNCTION__, __LINE__);
    return;
  }
  if (aqr->is_stopped_) {
    return;
  }
  if (aqr->is_paused_) {
    memset(buffer->mAudioData, aqr->obtained_.silence,
           buffer->mAudioDataByteSize);
  } else if (aqr->audio_callback_) {
    aqr->audio_callback_->GetBuffer(static_cast<uint8_t *>(buffer->mAudioData),
                                    buffer->mAudioDataByteSize);
  }
  AudioQueueEnqueueBuffer(aq, buffer, 0, nullptr);
}

int AudioQueueRender::OpenAudio(
    const AudioInfo &desired, AudioInfo &obtained,
    std::unique_ptr<AudioCallback> &audio_callback) {
  AdjustAudioInfo(desired, obtained);
  uint32_t size = 1;
  uint32_t bit_per_channel = 16;
  size *= bit_per_channel / 8;
  size *= obtained.channels;
  size *= obtained.samples;
  obtained.size = size;
  if (size == 0) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] unexcepted size!\n",
               __FUNCTION__, __LINE__);
    return -1;
  }
  obtained_ = obtained;
  AudioStreamBasicDescription streamDescription;
  streamDescription.mSampleRate = static_cast<Float64>(obtained_.sample_rate);
  streamDescription.mFormatID = kAudioFormatLinearPCM;
  streamDescription.mFormatFlags = kLinearPCMFormatFlagIsPacked;
  streamDescription.mChannelsPerFrame = obtained_.channels;
  streamDescription.mFramesPerPacket = 1;
  streamDescription.mBitsPerChannel = bit_per_channel;
  // kAudioS16Sys
  streamDescription.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
  streamDescription.mFormatFlags |= kAudioFormatFlagsNativeEndian;

  streamDescription.mBytesPerFrame = streamDescription.mBitsPerChannel *
                                     streamDescription.mChannelsPerFrame / 8;
  streamDescription.mBytesPerPacket =
      streamDescription.mBytesPerFrame * streamDescription.mFramesPerPacket;
  std::lock_guard<std::mutex> mutex(mutex_);
  OSStatus status =
      AudioQueueNewOutput(&streamDescription, AudioQueueOuptutCallback, this,
                          NULL, kCFRunLoopCommonModes, 0, &audio_queue_ref_);
  if (status != noErr) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] status:%d!\n", __FUNCTION__,
               __LINE__, static_cast<int>(status));
    return -1;
  }
  UInt32 propValue = 1;
  AudioQueueSetProperty(audio_queue_ref_, kAudioQueueProperty_EnableTimePitch,
                        &propValue, sizeof(propValue));
  propValue = 1;
  AudioQueueSetProperty(audio_queue_ref_, kAudioQueueProperty_TimePitchBypass,
                        &propValue, sizeof(propValue));
  propValue = kAudioQueueTimePitchAlgorithm_Spectral;
  AudioQueueSetProperty(audio_queue_ref_,
                        kAudioQueueProperty_TimePitchAlgorithm, &propValue,
                        sizeof(propValue));

  for (int i = 0; i < KAudioQueueNumberBuffers; i++) {
    status = AudioQueueAllocateBuffer(audio_queue_ref_, size,
                                      &audio_queue_buffer_ref_array_[i]);
    if (status != noErr) {
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] status:%d!\n",
                 __FUNCTION__, __LINE__, static_cast<int>(status));
      break;
    }
    audio_queue_buffer_ref_array_[i]->mAudioDataByteSize = size;
    memset(audio_queue_buffer_ref_array_[i]->mAudioData, 0, size);
    status = AudioQueueEnqueueBuffer(audio_queue_ref_,
                                     audio_queue_buffer_ref_array_[i], 0, NULL);
    if (status != noErr) {
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] status:%d!\n",
                 __FUNCTION__, __LINE__, static_cast<int>(status));
      break;
    }
  }
  audio_callback_ = std::move(audio_callback);
  return 0;
}

void AudioQueueRender::PauseAudio(int pause_on) {
  std::lock_guard<std::mutex> mutex(mutex_);
  if (!audio_queue_ref_) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] audio_queue_ref_ is null!\n", __FUNCTION__, __LINE__);
    return;
  }
  if (pause_on) {
    is_paused_ = true;
    OSStatus status = AudioQueuePause(audio_queue_ref_);
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] AudioQueuePause status:%d!\n", __FUNCTION__, __LINE__,
               static_cast<int>(status));
  } else {
    is_paused_ = false;
    OSStatus status = AudioQueueStart(audio_queue_ref_, nullptr);
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] AudioQueueStart status:%d!\n", __FUNCTION__, __LINE__,
               static_cast<int>(status));
  }
}

void AudioQueueRender::FlushAudio() {
  std::lock_guard<std::mutex> mutex(mutex_);
  if (!audio_queue_ref_) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] audio_queue_ref_ is null!\n", __FUNCTION__, __LINE__);
    return;
  }
  if (is_paused_) {
    for (int i = 0; i < KAudioQueueNumberBuffers; i++) {
      if (audio_queue_buffer_ref_array_[i] &&
          audio_queue_buffer_ref_array_[i]->mAudioData) {
        audio_queue_buffer_ref_array_[i]->mAudioDataByteSize = obtained_.size;
        memset(audio_queue_buffer_ref_array_[i]->mAudioData, 0, obtained_.size);
      }
    }
  } else {
    OSStatus status = AudioQueueFlush(audio_queue_ref_);
    if (status != noErr) {
      AV_LOGE_ID(AUDIO_LOG_TAG, session_id_, "[%s:%d] status:%d!\n",
                 __FUNCTION__, __LINE__, static_cast<int>(status));
    }
  }
}
void AudioQueueRender::SetStereoVolume(float left_volume, float right_volume) {
  return;
}
void AudioQueueRender::CloseAudio() {
  std::lock_guard<std::mutex> mutex(mutex_);
  if (!audio_queue_ref_) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] audio_queue_ref_ is null!\n", __FUNCTION__, __LINE__);
    return;
  }
  is_stopped_ = true;
  AudioQueueStop(audio_queue_ref_, true);
  AudioQueueDispose(audio_queue_ref_, true);
  audio_queue_ref_ = nullptr;
}

void AudioQueueRender::WaitClose() { return; }

double AudioQueueRender::GetLatencySeconds() {
  return (static_cast<double>(KAudioQueueNumberBuffers)) * obtained_.samples /
         obtained_.sample_rate;
}

double AudioQueueRender::GetAudiotrackLatencySeconds() { return 0.0; }

void AudioQueueRender::SetDefaultLatencySeconds(double latency) { return; }

int AudioQueueRender::GetAudioPerSecondCallBacks() {
  return KAudioMaxCallbacksPerSec;
}

// optional
void AudioQueueRender::SetPlaybackRate(float playbackRate) {
  std::lock_guard<std::mutex> mutex(mutex_);
  if (!audio_queue_ref_) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] audio_queue_ref_ is null!\n", __FUNCTION__, __LINE__);
    return;
  }
  UInt32 propValue = 0;
  if (fabsf(playbackRate - 1.0f) <= 0.000001) {
    propValue = 1;
    playbackRate = 1.0f;
  }
  AudioQueueSetProperty(audio_queue_ref_, kAudioQueueProperty_TimePitchBypass,
                        &propValue, sizeof(propValue));
  AudioQueueSetParameter(audio_queue_ref_, kAudioQueueParam_PlayRate,
                         playbackRate);
}

void AudioQueueRender::SetPlaybackVolume(float volume) {
  std::lock_guard<std::mutex> mutex(mutex_);
  if (!audio_queue_ref_) {
    AV_LOGE_ID(AUDIO_LOG_TAG, session_id_,
               "[%s:%d] audio_queue_ref_ is null!\n", __FUNCTION__, __LINE__);
    return;
  }
  float aq_volume = volume;
  if (fabsf(aq_volume - 1.0f) <= 0.000001) {
    AudioQueueSetParameter(audio_queue_ref_, kAudioQueueParam_Volume, 1.f);
  } else {
    AudioQueueSetParameter(audio_queue_ref_, kAudioQueueParam_Volume,
                           aq_volume);
  }
}

// android only
int AudioQueueRender::GetAudioSessionId() { return -1; }

#endif
