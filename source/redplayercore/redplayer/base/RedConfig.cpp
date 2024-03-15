#include "base/RedConfig.h"

REDPLAYER_NS_BEGIN;

RedPlayerConfig::RedPlayerConfig() { mConfig = new PlayerConfig(); }

RedPlayerConfig::~RedPlayerConfig() {
  reset();
  if (mConfig) {
    delete mConfig;
    mConfig = nullptr;
  }
}

void RedPlayerConfig::reset() {
  if (!mConfig) {
    return;
  }

  memset(mConfig, 0, sizeof(PlayerConfig));

  mConfig->loop = 1;
  mConfig->max_fps = 61;
  mConfig->start_on_prepared = 1;
  mConfig->pictq_size = VIDEO_PICTURE_QUEUE_SIZE_DEFAULT;
  mConfig->packet_buffering = 1;
  mConfig->accurate_seek_timeout = MAX_ACCURATE_SEEK_TIMEOUT;

  mConfig->dcc.max_buffer_size = MAX_QUEUE_SIZE;
  mConfig->dcc.high_water_mark_in_bytes = DEFAULT_HIGH_WATER_MARK_IN_BYTES;
  mConfig->dcc.first_high_water_mark_in_ms =
      DEFAULT_FIRST_HIGH_WATER_MARK_IN_MS;
  mConfig->dcc.next_high_water_mark_in_ms = DEFAULT_NEXT_HIGH_WATER_MARK_IN_MS;
  mConfig->dcc.last_high_water_mark_in_ms = DEFAULT_LAST_HIGH_WATER_MARK_IN_MS;
  mConfig->dcc.current_high_water_mark_in_ms =
      DEFAULT_FIRST_HIGH_WATER_MARK_IN_MS;
}

PlayerConfig *RedPlayerConfig::operator->() { return mConfig; }

PlayerConfig *RedPlayerConfig::get() { return mConfig; }

REDPLAYER_NS_END;
