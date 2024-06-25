#pragma once

#include "soundTouch/SoundTouch.h"
#include <stdint.h>

soundtouch::SoundTouch *soundtouchCreate();
int soundtouchTranslate(soundtouch::SoundTouch *handle, short *data,
                        float speed, float pitch, int len, int bytes_per_sample,
                        int n_channel, int n_sampleRate);
void soundtouchDestroy(soundtouch::SoundTouch **handle);
