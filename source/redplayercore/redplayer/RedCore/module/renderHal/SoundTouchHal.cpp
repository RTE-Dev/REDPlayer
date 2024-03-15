#include "SoundTouchHal.h"

soundtouch::SoundTouch *soundtouchCreate() {
  soundtouch::SoundTouch *handle_ptr = new soundtouch::SoundTouch();
  return handle_ptr;
}

int soundtouchTranslate(soundtouch::SoundTouch *handle, short *data,
                        float speed, float pitch, int len, int bytes_per_sample,
                        int n_channel, int n_sampleRate) {
  int put_n_sample = len / n_channel;
  int nb = 0;
  int pcm_data_size = 0;

  if (handle == nullptr) {
    return 0;
  }

  handle->setPitch(pitch);
  handle->setRate(speed);
  handle->setSampleRate(n_sampleRate);
  handle->setChannels(n_channel);
  handle->putSamples(reinterpret_cast<soundtouch::SAMPLETYPE *>(data),
                     put_n_sample);

  do {
    nb =
        handle->receiveSamples(reinterpret_cast<soundtouch::SAMPLETYPE *>(data),
                               n_sampleRate / n_channel);
    pcm_data_size += nb * n_channel * bytes_per_sample;
  } while (nb != 0);

  return pcm_data_size;
}

void soundtouchDestroy(soundtouch::SoundTouch **handle) {
  if (!handle || !(*handle)) {
    return;
  }
  (*handle)->clear();
  delete (*handle);
  (*handle) = nullptr;
}
