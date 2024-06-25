#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>

#import <atomic>

#import "NSString+REDMedia.h"
#import "RedMediaPlayBack.h"
#import "RedNotificationManager.h"
#import "RedPlayerController.h"
#import "wrapper/reddownload_datasource_wrapper.h"

#import "Interface/RedPlayer.h"
#import "RedCore/module/sourcer/format/redioapplication.h"
#import "RedLog.h"
#import "RedMeta.h"
#import "redrender/video/ios/redrender_view_protocol.h"
#import "redrender/video/metal/redrender_metal_view.h"

#define TAG "RedPlayerController"

static std::atomic<int> g_id(0);
static RedLogCallback kLogcallback = nil;
static REDLogCallScene kLogCallScene = REDLogCallScene_Undefine;

@interface REDWeakHolder : NSObject
@property(nonatomic, weak) id object;
@end

@implementation REDWeakHolder
@end

using namespace redPlayer_ns;

// It means you didn't call shutdown if you found this object leaked.
@interface RedWeakHolder : NSObject
@property(nonatomic, weak) id object;
@end

@implementation RedWeakHolder
@end

@interface RedPlayerController ()

@property(nonatomic, strong) dispatch_queue_t syncQueue;

@end

@implementation RedPlayerController {
  sp<CRedPlayer> _mediaPlayer;
  UIView<RedRenderViewProtocol> *_glView;
  NSString *_urlString;
  NSString *_jsonString;
  BOOL _isInputJson;

  NSInteger _videoWidth;
  NSInteger _videoHeight;
  NSInteger _sampleAspectRatioNumerator;
  NSInteger _sampleAspectRatioDenominator;

  BOOL _seeking;
  NSInteger _bufferingTime;
  NSInteger _bufferingPosition;

  BOOL _keepScreenOnWhilePlaying;
  BOOL _pauseInBackground;
  BOOL _isVideoToolboxOpen;
  BOOL _playingBeforeInterruption;
  BOOL _mute;

  RedRenderType _renderType;

  RedNotificationManager *_notificationManager;

  RedCacheStatisticWrapper _cacheStat;
  NSTimer *_hudTimer;

  int _id;
  int _rotateDegrees;
  std::mutex _lock;
}

@synthesize view = _view;
@synthesize currentPlaybackTime;
@synthesize duration;
@synthesize playableDuration;
@synthesize bufferingProgress = _bufferingProgress;

@synthesize isPreparedToPlay = _isPreparedToPlay;
@synthesize playbackState = _playbackState;
@synthesize loadState = _loadState;

@synthesize naturalSize = _naturalSize;
@synthesize scalingMode = _scalingMode;
@synthesize shouldAutoplay = _shouldAutoplay;

@synthesize isSeekBuffering = _isSeekBuffering;
@synthesize isAudioSync = _isAudioSync;
@synthesize isVideoSync = _isVideoSync;
@synthesize videoFileSize = _videoFileSize;
@synthesize vdps = _vdps;
@synthesize vRenderFps = _vRenderFps;

@synthesize cachedDuration = _cachedDuration;
@synthesize fpsInMeta = _fpsInMeta;

@synthesize notPauseGlviewBackground = _notPauseGlviewBackground;
#define FFP_IO_STAT_STEP (50 * 1024)

- (id)initWithContentURL:(NSURL *)aUrl
          withRenderType:(RedRenderType)renderType {
  if (aUrl == nil)
    return nil;
  // Detect if URL is file path and return proper string for it
  NSString *aUrlString = [aUrl isFileURL] ? [aUrl path] : [aUrl absoluteString];

  return [self initWithContentURLString:aUrlString withRenderType:renderType];
}

- (instancetype)initWithContentURLString:(NSString *)aUrlString
                          withRenderType:(RedRenderType)renderType {
  if (aUrlString == nil)
    return nil;

  self = [super init];
  if (self) {
    globalInit();
    globalSetInjectCallback(red_inject_callback);

    _scalingMode = RedScalingModeAspectFit;
    memset(&_cacheStat, 0, sizeof(_cacheStat));

    _urlString = aUrlString;

    _id = ++g_id;
    _mediaPlayer = CRedPlayer::Create(
        _id, std::bind(&media_player_msg_loop, std::placeholders::_1));
    REDWeakHolder *weakHolder = [REDWeakHolder new];
    weakHolder.object = self;

    _mediaPlayer->setWeakThiz((__bridge_retained void *)self);
    _mediaPlayer->setInjectOpaque((__bridge_retained void *)weakHolder);
    _mediaPlayer->setConfig(cfgTypePlayer, "start-on-prepared",
                            _shouldAutoplay ? 1 : 0);

    _syncQueue = dispatch_queue_create("com.xiaohongshu.player.red.sync.queue",
                                       DISPATCH_QUEUE_SERIAL);

    _renderType = renderType;
    // init video sink
    CGRect cgrect = {{0, 0}, {0, 0}};
    switch (renderType) {
    case RedRenderType::RedRenderTypeOpenGL:
      cgrect = [[UIScreen mainScreen] bounds];
      break;
    case RedRenderType::RedRenderTypeMetal:
#if TARGET_OS_SIMULATOR
      cgrect = [[UIScreen mainScreen] bounds];
      renderType = RedRenderType::RedRenderTypeOpenGL;
#else
      cgrect = CGRectZero;
#endif
      break;
    case RedRenderType::RedRenderTypeSampleBufferDisplayLayer:
      cgrect = CGRectZero;
      break;
    default:
      cgrect = [[UIScreen mainScreen] bounds];
      break;
    }
    _glView = (UIView<RedRenderViewProtocol> *)_mediaPlayer->initWithFrame(
        (int)renderType, cgrect);

    _view = _glView;

    _mediaPlayer->setConfig(cfgTypePlayer, "overlay-format", "fcc-_es2");
    _pauseInBackground = NO;
  }
  return self;
}

- (id)init {
  self = [super init];

  if (self) {
    globalInit();
    _id = ++g_id;
    _mediaPlayer = CRedPlayer::Create(
        _id, std::bind(&media_player_msg_loop, std::placeholders::_1));
    _urlString = nil;
    _mediaPlayer->setWeakThiz((__bridge_retained void *)self);
  }

  return self;
}

- (void)setShouldAutoplay:(BOOL)shouldAutoplay {
  _shouldAutoplay = shouldAutoplay;

  if (!_mediaPlayer)
    return;
  _mediaPlayer->setConfig(cfgTypePlayer, "start-on-prepared",
                          _shouldAutoplay ? 1 : 0);
}

- (BOOL)shouldAutoplay {
  return _shouldAutoplay;
}

- (void)setContentURL:(NSURL *)aURL {
  _urlString = nil;
  _urlString = [aURL isFileURL] ? [aURL path] : [aURL absoluteString];
  AV_LOGD_ID(TAG, _id, "%s url %s\n", __func__, [_urlString UTF8String]);
}

- (void)setContentURLList:(NSString *)aString {
  _urlString = nil;
  _urlString = aString;
  AV_LOGD_ID(TAG, _id, "%s urllist %s\n", __func__, [_urlString UTF8String]);
}

- (void)setContentString:(NSString *)aString {
  _urlString = nil;
  _jsonString = nil;
  _jsonString = aString;
  _isInputJson = YES;
  if (!_mediaPlayer || !_jsonString)
    return;
  _mediaPlayer->setConfig(cfgTypePlayer, "is-input-json", 2);
  _mediaPlayer->setConfig(cfgTypeFormat, "enable-url-list-fallback", 1);
  _mediaPlayer->setDataSource([_jsonString UTF8String]);
  [self getPlayUrl];
}

- (void)setVideoCacheDir:(NSString *)dir {
  if (!_mediaPlayer || !dir) {
    return;
  }
  _mediaPlayer->setConfig(cfgTypeFormat, "cache_file_dir", [dir UTF8String]);
}

- (void)setEnableHDR:(BOOL)enable {
  if (!_mediaPlayer) {
    return;
  }
  _mediaPlayer->setConfig(cfgTypePlayer, "enable-video-hdr", (int64_t)enable);
  if (enable)
    _mediaPlayer->setConfig(cfgTypePlayer, "videotoolbox", (int64_t)enable);
}

- (void)setEnableVTB:(BOOL)enable {
  if (!_mediaPlayer) {
    return;
  }
  _mediaPlayer->setConfig(cfgTypePlayer, "videotoolbox", (int64_t)enable);
}

- (void)setLiveMode:(BOOL)enable {
  if (!_mediaPlayer) {
    return;
  }
  if (enable == YES) {
    _mediaPlayer->setConfig(cfgTypeFormat, "load_file", 0);
    _mediaPlayer->setConfig(cfgTypeFormat, "connect_timeout", 0);
  }
}

- (NSString *)getPlayUrl {
  NSString *ret = nil;
  std::string play_url;
  if (!_mediaPlayer)
    return ret;
  if (!_mediaPlayer->getPlayUrl(play_url)) {
    ret = [NSString stringWithUTF8String:play_url.c_str()];
  }
  _urlString = ret;
  return ret;
}

- (void)prepareToPlay {
  if (!_mediaPlayer || !_urlString) {
    AV_LOGW_ID(TAG, _id, "%s failed, _mediaPlayer %p, _urlString %p\n",
               __func__, _mediaPlayer.get(), _urlString);
    return;
  }
  if (!_isInputJson) {
    std::map<std::string, std::string> headers;
    std::string url = [_urlString UTF8String];
    _mediaPlayer->setDataSource(url);
  }
  _mediaPlayer->prepareAsync();
}

- (void)play {
  if (!_mediaPlayer)
    return;

  _mediaPlayer->start();
  _glView.renderPaused = NO;
}

- (void)pause {
  if (!_mediaPlayer)
    return;

  _mediaPlayer->pause();
  _glView.renderPaused = YES;
}

- (void)stop {
  if (!_mediaPlayer)
    return;

  _mediaPlayer->stop();
}

- (BOOL)isPlaying {
  if (!_mediaPlayer)
    return NO;

  return _mediaPlayer->isPlaying();
}

- (void)setPauseInBackground:(BOOL)pause {
  _pauseInBackground = pause;
}

- (BOOL)isVideoToolboxOpen {
  if (!_mediaPlayer)
    return NO;

  return _isVideoToolboxOpen;
}

+ (void)setLogCallbackLevel:(RedLogLevel)logLevel {
  setLogCallbackLevel((int)logLevel);
}

+ (void)setLogCallback:(RedLogCallback)logCallback {
  [RedPlayerController setLogCallback:logCallback
                         logCallScene:REDLogCallScene_Default];
}

+ (void)setLogCallback:(RedLogCallback)logCallback
          logCallScene:(REDLogCallScene)logCallScene {
  if (kLogCallScene != logCallScene || !logCallback) {
    kLogCallScene = logCallScene;
    kLogcallback = [logCallback copy];
    if (!logCallback) {
      setLogCallback(nullptr);
      return;
    }
    setLogCallback(customLogCallback);
    AV_LOGD(TAG, "RED_SET_LOG_CALLBACK: %d\n", (int)logCallScene);
  }
}

+ (void)initreddownload:(NSString *)cachePath
                maxSize:(uint32_t)maxSize
         threadpoolSize:(int)poolSize {
  if (cachePath == nil) {
    AV_LOGE(TAG, "initreddownload cachePath is nil\n");
    return;
  }
  AV_LOGI(TAG, "initreddownload %s maxSize: %u threadpoolSize: %d\n",
          cachePath.UTF8String, maxSize, poolSize);
  DownLoadOptWrapper opt;
  reddownload_datasource_wrapper_opt_reset(&opt);
  opt.cache_file_dir = cachePath.UTF8String;
  opt.cache_max_entries = maxSize;
  opt.threadpool_size = poolSize;
  reddownload_datasource_wrapper_init(&opt);
}

+ (void)initreddownload:(NSString *)cachePath
                maxSize:(uint32_t)maxSize
         threadpoolSize:(int)poolSize
         maxdircapacity:(int64_t)maxdircapacity {
  if (cachePath == nil) {
    AV_LOGE(TAG, "initreddownload cachePath is nil\n");
    return;
  }
  AV_LOGI(TAG, "initreddownload %s maxSize: %u threadpoolSize: %d\n",
          cachePath.UTF8String, maxSize, poolSize);
  DownLoadOptWrapper opt;
  reddownload_datasource_wrapper_opt_reset(&opt);
  opt.cache_file_dir = cachePath.UTF8String;
  opt.cache_max_entries = maxSize;
  opt.threadpool_size = poolSize;
  if (maxdircapacity > 0) {
    opt.cache_max_dir_capacity = maxdircapacity;
  }
  reddownload_datasource_wrapper_init(&opt);
}

+ (void)setreddownloadGlobalConfig:
    (NSDictionary<NSString *, NSNumber *> *)configDic {
  static dispatch_once_t onceToken;
  for (NSString *key in configDic) {
    reddownload_global_config_set(key.UTF8String, configDic[key].intValue);
  }
}

+ (void)getNetworkQuality:(int *)level speed:(int *)speed {
  reddownload_get_network_quality(level, speed);
}

- (void)shutdown {
  AV_LOGD_ID(TAG, _id, "%s start\n", __func__);
  if (!_mediaPlayer)
    return;
  void (^releaseBlock)(void) = ^{
    if (!self->_mediaPlayer)
      return;
    self->_mediaPlayer->stop();
    __unused id weakHolder =
        (__bridge_transfer REDWeakHolder *)self->_mediaPlayer->setInjectOpaque(
            nullptr);
    self->_mediaPlayer->release();

    void (^block)(void) = ^{
      if (!self->_mediaPlayer)
        return;
      self->_isPreparedToPlay = NO;

      __unused id weakPlayer = (__bridge_transfer RedPlayerController *)
                                   self->_mediaPlayer->setWeakThiz(nullptr);

      [self->_glView removeFromSuperview];
      [self->_glView shutdown];
      self->_view = nil;
      self->_glView = nil;

      [self didShutdown];
    };

    if (0 != pthread_main_np()) {
      block();
    } else {
      dispatch_sync(dispatch_get_main_queue(), ^{
        block();
      });
    }
  };
  dispatch_async(_syncQueue, ^{
    releaseBlock();
  });
  AV_LOGD_ID(TAG, _id, "%s end\n", __func__);
}

- (void)didShutdown {
}

- (RedPlaybackState)playbackState {
  if (!_mediaPlayer)
    return RedPlaybackStateStopped;

  RedPlaybackState mpState = RedPlaybackStateStopped;
  int state = _mediaPlayer->getPlayerState();
  switch (state) {
  case MP_STATE_STOPPED:
  case MP_STATE_COMPLETED:
  case MP_STATE_ERROR:
  case MP_STATE_END:
    mpState = RedPlaybackStateStopped;
    break;
  case MP_STATE_IDLE:
  case MP_STATE_INITIALIZED:
  case MP_STATE_ASYNC_PREPARING:
  case MP_STATE_PAUSED:
    mpState = RedPlaybackStatePaused;
    break;
  case MP_STATE_PREPARED:
  case MP_STATE_STARTED: {
    if (_seeking)
      mpState = RedPlaybackStateSeekingForward;
    else
      mpState = RedPlaybackStatePlaying;
    break;
  }
  }
  return mpState;
}

- (void)setCurrentPlaybackTime:(NSTimeInterval)aCurrentPlaybackTime {
  if (!_mediaPlayer)
    return;

  _seeking = YES;
  [[NSNotificationCenter defaultCenter]
      postNotificationName:RedPlayerPlaybackStateDidChangeNotification
                    object:self];

  _bufferingPosition = 0;
  _mediaPlayer->seekTo(aCurrentPlaybackTime * 1000);
}

- (BOOL)seekCurrentPlaybackTime:(NSTimeInterval)aCurrentPlaybackTime {
  if (!_mediaPlayer)
    return 1;

  _seeking = YES;
  [[NSNotificationCenter defaultCenter]
      postNotificationName:RedPlayerPlaybackStateDidChangeNotification
                    object:self];

  _bufferingPosition = 0;
  BOOL bRet = _mediaPlayer->seekTo(aCurrentPlaybackTime * 1000);
  if (bRet != 0) {
    _seeking = NO;
  }
  return bRet;
}

- (NSTimeInterval)currentPlaybackTime {
  if (!_mediaPlayer)
    return 0.0f;

  int64_t pos = 0;
  _mediaPlayer->getCurrentPosition(pos);
  NSTimeInterval ret = pos;
  if (isnan(ret) || isinf(ret))
    return -1;

  return ret / 1000;
}

- (NSTimeInterval)duration {
  if (!_mediaPlayer)
    return 0.0f;

  int64_t duration = 0;
  _mediaPlayer->getDuration(duration);
  NSTimeInterval ret = duration;
  if (isnan(ret) || isinf(ret))
    return -1;

  return ret / 1000;
}

- (NSTimeInterval)playableDuration {
  if (!_mediaPlayer)
    return 0.0f;

  int64_t pos = 0;
  int64_t playable_duration = 0;
  int64_t video_cached_ms = 0, audio_cached_ms = 0;

  video_cached_ms =
      _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_CACHED_DURATION, (int64_t)0);
  audio_cached_ms =
      _mediaPlayer->getProp(RED_PROP_INT64_AUDIO_CACHED_DURATION, (int64_t)0);
  _mediaPlayer->getCurrentPosition(pos);
  playable_duration = pos + std::min(video_cached_ms, audio_cached_ms);

  NSTimeInterval demux_cache = ((NSTimeInterval)playable_duration) / 1000;

  return demux_cache;
}

- (CGSize)naturalSize {
  return _naturalSize;
}

- (void)changeNaturalSize {
  [self willChangeValueForKey:@"naturalSize"];
  if (_sampleAspectRatioNumerator > 0 && _sampleAspectRatioDenominator > 0) {
    self->_naturalSize =
        CGSizeMake(1.0f * _videoWidth * _sampleAspectRatioNumerator /
                       _sampleAspectRatioDenominator,
                   _videoHeight);
  } else {
    self->_naturalSize = CGSizeMake(_videoWidth, _videoHeight);
  }
  [self didChangeValueForKey:@"naturalSize"];

  if (self->_naturalSize.width > 0 && self->_naturalSize.height > 0) {
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerNaturalSizeAvailableNotification
                      object:self];
  }
}

- (void)setScalingMode:(RedScalingMode)aScalingMode {
  RedScalingMode newScalingMode = aScalingMode;
  switch (aScalingMode) {
  case RedScalingModeNone:
    [_view setContentMode:UIViewContentModeCenter];
    break;
  case RedScalingModeAspectFit:
    [_view setContentMode:UIViewContentModeScaleAspectFit];
    break;
  case RedScalingModeAspectFill:
    [_view setContentMode:UIViewContentModeScaleAspectFill];
    break;
  case RedScalingModeFill:
    [_view setContentMode:UIViewContentModeScaleToFill];
    break;
  default:
    newScalingMode = _scalingMode;
  }

  _scalingMode = newScalingMode;
}

- (UIImage *)thumbnailImageAtCurrentTime {
  if ([_view conformsToProtocol:@protocol(RedRenderViewProtocol)]) {
    id<RedRenderViewProtocol> glView = (id<RedRenderViewProtocol>)_view;
    return [glView snapshot];
  }

  return nil;
}

inline static NSString *formatedDurationMilli(int64_t duration) {
  if (duration >= 1000) {
    return [NSString stringWithFormat:@"%.2f sec", ((float)duration) / 1000];
  } else {
    return [NSString stringWithFormat:@"%ld msec", (long)duration];
  }
}

inline static NSString *formatedDurationBytesAndBitrate(int64_t bytes,
                                                        int64_t bitRate) {
  if (bitRate <= 0) {
    return @"inf";
  }
  return formatedDurationMilli(((float)bytes) * 8 * 1000 / bitRate);
}

inline static NSString *formatedSize(int64_t bytes) {
  if (bytes >= 100 * 1024) {
    return [NSString stringWithFormat:@"%.2f MB", ((float)bytes) / 1000 / 1000];
  } else if (bytes >= 100) {
    return [NSString stringWithFormat:@"%.1f KB", ((float)bytes) / 1000];
  } else {
    return [NSString stringWithFormat:@"%ld B", (long)bytes];
  }
}

inline static NSString *formatedSpeed(int64_t bytes, int64_t elapsed_milli) {
  if (elapsed_milli <= 0) {
    return @"N/A";
  }

  if (bytes <= 0) {
    return @"0";
  }

  float bytes_per_sec = ((float)bytes) * 1000.f / elapsed_milli;
  if (bytes_per_sec >= 1000 * 1000) {
    return [NSString
        stringWithFormat:@"%.2f MB/s", ((float)bytes_per_sec) / 1000 / 1000];
  } else if (bytes_per_sec >= 1000) {
    return
        [NSString stringWithFormat:@"%.1f KB/s", ((float)bytes_per_sec) / 1000];
  } else {
    return [NSString stringWithFormat:@"%ld B/s", (long)bytes_per_sec];
  }
}

- (NSDictionary *)getVideoDebugInfo {
  if (_mediaPlayer == nil)
    return nil;

  NSMutableDictionary *debugInfo = @{}.mutableCopy;

  int64_t vdec = _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_DECODER,
                                       (int64_t)RED_PROPV_DECODER_UNKNOWN);
  float vdps =
      _mediaPlayer->getProp(RED_PROP_FLOAT_VIDEO_DECODE_FRAMES_PER_SECOND, .0f);
  float vfps =
      _mediaPlayer->getProp(RED_PROP_FLOAT_VIDEO_OUTPUT_FRAMES_PER_SECOND, .0f);
  int64_t pix_fmt = _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_PIXEL_FORMAT,
                                          (int64_t)AV_PIX_FMT_NONE);
  int64_t bit_rate = _mediaPlayer->getProp(RED_PROP_INT64_BIT_RATE, (int64_t)0);
  int64_t seek_cost = _mediaPlayer->getProp(
      RED_PROP_INT64_LATEST_SEEK_LOAD_DURATION, (int64_t)0);

  switch (vdec) {
  case RED_PROPV_DECODER_VIDEOTOOLBOX:
    [debugInfo setValue:@"VideoToolbox" forKey:@"vdec"];
    break;
  case RED_PROPV_DECODER_AVCODEC:
    [debugInfo setValue:[NSString stringWithFormat:@"avcodec %d.%d.%d",
                                                   LIBAVCODEC_VERSION_MAJOR,
                                                   LIBAVCODEC_VERSION_MINOR,
                                                   LIBAVCODEC_VERSION_MICRO]
                 forKey:@"vdec"];
    break;
  default:
    [debugInfo setValue:@"N/A" forKey:@"vdec"];
    break;
  }

  switch (_renderType) {
  case RedRenderTypeOpenGL:
    [debugInfo setValue:@"OpenGL" forKey:@"render"];
    break;
  case RedRenderTypeMetal:
    [debugInfo setValue:@"Metal" forKey:@"render"];
    break;
  case RedRenderTypeSampleBufferDisplayLayer:
    [debugInfo setValue:@"SampleBufferDisplayLayer" forKey:@"render"];
    break;

  default:
    break;
  }

  switch (pix_fmt) {
  case AV_PIX_FMT_YUV420P10LE:
    [debugInfo setValue:@"420p10le" forKey:@"pix_fmt"];
    break;

  default:
    [debugInfo setValue:@"420sp/420p" forKey:@"pix_fmt"];
    break;
  }

  [debugInfo setValue:[NSString stringWithFormat:@"%.2f / %.2f", vdps, vfps]
               forKey:@"fps"];
  [debugInfo
      setValue:[NSString stringWithFormat:@"%.3d /%.3d", (int)_videoWidth,
                                          (int)_videoHeight]
        forKey:@"resolution"];
  [debugInfo
      setValue:[NSString stringWithFormat:@"%@/s", formatedSize(bit_rate / 8)]
        forKey:@"bitrate"];
  int64_t vcacheb =
      _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_CACHED_BYTES, (int64_t)0);
  int64_t acacheb =
      _mediaPlayer->getProp(RED_PROP_INT64_AUDIO_CACHED_BYTES, (int64_t)0);
  int64_t vcached =
      _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_CACHED_DURATION, (int64_t)0);
  int64_t acached =
      _mediaPlayer->getProp(RED_PROP_INT64_AUDIO_CACHED_DURATION, (int64_t)0);
  int64_t vcachep =
      _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_CACHED_PACKETS, (int64_t)0);
  int64_t acachep =
      _mediaPlayer->getProp(RED_PROP_INT64_AUDIO_CACHED_PACKETS, (int64_t)0);
  [debugInfo setValue:[NSString stringWithFormat:@"%@, %@, %" PRId64 " packets",
                                                 formatedDurationMilli(vcached),
                                                 formatedSize(vcacheb), vcachep]
               forKey:@"v-cache"];
  [debugInfo setValue:[NSString stringWithFormat:@"%@, %@, %" PRId64 " packets",
                                                 formatedDurationMilli(acached),
                                                 formatedSize(acacheb), acachep]
               forKey:@"a-cache"];

  float avdelay = _mediaPlayer->getProp(RED_PROP_FLOAT_AVDELAY, .0f);
  float avdiff = _mediaPlayer->getProp(RED_PROP_FLOAT_AVDIFF, .0f);
  [debugInfo setValue:[NSString stringWithFormat:@"%.3f %.3f", avdelay, -avdiff]
               forKey:@"delay"];

  [debugInfo setValue:@(self.playbackRate) forKey:@"speed"];
  [debugInfo setValue:@(self.playbackVolume) forKey:@"volume"];
  [debugInfo setValue:formatedDurationMilli(seek_cost) forKey:@"seek_cost"];
  return [debugInfo copy];
}

- (void)setPlaybackRate:(float)playbackRate {
  if (!_mediaPlayer)
    return;

  _mediaPlayer->setProp(RED_PROP_FLOAT_PLAYBACK_RATE, playbackRate);
}

- (float)playbackRate {
  if (!_mediaPlayer)
    return 0.0f;

  return _mediaPlayer->getProp(RED_PROP_FLOAT_PLAYBACK_RATE, 0.0f);
}

- (void)setPlaybackVolume:(float)volume {
  if (!_mediaPlayer)
    return;
  return _mediaPlayer->setVolume(volume, volume);
}

- (float)playbackVolume {
  if (!_mediaPlayer)
    return 0.0f;
  return _mediaPlayer->getProp(RED_PROP_FLOAT_PLAYBACK_VOLUME, 1.0f);
}

- (int64_t)getFileSize {
  if (!_mediaPlayer)
    return 0;
  return _mediaPlayer->getProp(RED_PROP_INT64_LOGICAL_FILE_SIZE, (int64_t)0);
}

- (float)dropPacketRateBeforeDecode {
  if (!_mediaPlayer)
    return 0.0f;
  return _mediaPlayer->getProp(RED_PROP_FLOAT_DROP_PACKET_RATE_BEFORE_DECODE,
                               0.0f);
}

- (float)dropFrameRate {
  if (!_mediaPlayer)
    return 0.0f;
  return _mediaPlayer->getProp(RED_PROP_FLOAT_DROP_FRAME_RATE, 0.0f);
}

- (float)vdps {
  if (!_mediaPlayer)
    return 0.0f;
  return _mediaPlayer->getProp(RED_PROP_FLOAT_VIDEO_DECODE_FRAMES_PER_SECOND,
                               0.0f);
}

- (float)vRenderFps {
  if (!_mediaPlayer)
    return 0.0f;
  return _mediaPlayer->getProp(RED_PROP_FLOAT_VIDEO_OUTPUT_FRAMES_PER_SECOND,
                               0.0f);
}

- (int64_t)cachedDuration {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_CACHED_DURATION,
                               (int64_t)0);
}

- (float)getPropertyFloat:(int)key {
  if (!_mediaPlayer)
    return 0.0f;
  return _mediaPlayer->getProp(key, 0.0f);
}

- (int64_t)getPropertyInt64:(int)key {
  if (!_mediaPlayer)
    return 0;
  return _mediaPlayer->getProp(key, (int64_t)0);
}

- (int)getLoop {
  if (!_mediaPlayer)
    return 0;
  return _mediaPlayer->getLoop();
}

- (void)setLoop:(int)loop {
  if (!_mediaPlayer)
    return;
  _mediaPlayer->setLoop(loop);
}

- (BOOL)getMute {
  return _mute;
}

- (void)setMute:(BOOL)muted {
  if (!_mediaPlayer)
    return;
  _mute = muted;
  _mediaPlayer->setMute(muted);
}

- (BOOL)isSoftDecoding {
  if (!_mediaPlayer) {
    return NO;
  }
  int64_t vdec = _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_DECODER,
                                       (int64_t)RED_PROPV_DECODER_UNKNOWN);
  return (vdec == RED_PROPV_DECODER_AVCODEC);
}

- (int64_t)videoCachedBytes {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_CACHED_STATISTIC_BYTE_COUNT,
                               (int64_t)0);
}

- (int64_t)videoRealCachedBytes {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_CACHED_STATISTIC_REAL_BYTE_COUNT,
                               (int64_t)0);
}

- (int64_t)tcpFirstTime {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_FIRST_TCP_READ_TIME, (int64_t)0);
}

- (int64_t)videoFileSize {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_LOGICAL_FILE_SIZE, (int64_t)0);
}

- (int64_t)maxBufferSize {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_MAX_BUFFER_SIZE, (int64_t)0);
}

- (int64_t)tcpSpeed {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_TCP_SPEED, (int64_t)0);
}

- (int64_t)lastTcpSpeed {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_LAST_TCP_SPEED, (int64_t)0);
}

- (int64_t)videoBufferBytes {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_VIDEO_CACHED_BYTES, (int64_t)0);
}

- (int64_t)audioBufferBytes {
  if (!_mediaPlayer) {
    return 0;
  }
  return _mediaPlayer->getProp(RED_PROP_INT64_AUDIO_CACHED_BYTES, (int64_t)0);
}

static void customLogCallback(int level, const char *tagstr,
                              const char *logstr) {
  NSString *tag = [NSString stringWithUTF8String:tagstr];
  NSString *log = [NSString stringWithUTF8String:logstr];
  if (kLogcallback) {
    kLogcallback((RedLogLevel)level, tag, log);
  }
}

- (void)postEvent:(sp<Message>)msg {
  if (!msg)
    return;

  if (!_mediaPlayer) {
    return;
  }

  int64_t time = msg->mTime;
  int32_t arg1 = msg->mArg1, arg2 = msg->mArg2;

  switch (msg->mWhat) {
  case RED_MSG_FLUSH:
    break;
  case RED_MSG_ERROR: {
    AV_LOGD_ID(TAG, _id, "RED_MSG_ERROR: %d detailError: %d\n", arg1, arg2);

    // [self setScreenOn:NO];

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerPlaybackStateDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerPlaybackDidFinishNotification
                      object:self
                    userInfo:@{
                      RedPlayerPlaybackDidFinishReasonUserInfoKey :
                          @(RedFinishReasonPlaybackError),
                      RedPlayerPlaybackDidFinishErrorUserInfoKey : @(arg1),
                      RedPlayerPlaybackDidFinishDetailedErrorUserInfoKey :
                          @(arg2),
                      RedPlayerMessageTimeUserInfoKey : @(time)
                    }];
    break;
  }
  case RED_MSG_BPREPARED: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_BPREPARED:\n");
    break;
  }
  case RED_MSG_PREPARED: {
    _isPreparedToPlay = YES;

    [[NSNotificationCenter defaultCenter]
        postNotificationName:
            RedMediaPlaybackIsPreparedToPlayDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    _loadState = RedLoadStatePlayable | RedLoadStatePlaythroughOK;

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerLoadStateDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];

    break;
  }
  case RED_MSG_COMPLETED: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_COMPLETED:\n");

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerPlaybackStateDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerPlaybackDidFinishNotification
                      object:self
                    userInfo:@{
                      RedPlayerPlaybackDidFinishReasonUserInfoKey :
                          @(RedFinishReasonPlaybackEnded),
                      RedPlayerMessageTimeUserInfoKey : @(time)
                    }];
    break;
  }
  case RED_MSG_VIDEO_SIZE_CHANGED:
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_SIZE_CHANGED: %d, %d\n", arg1, arg2);
    if (arg1 > 0)
      _videoWidth = arg1;
    if (arg2 > 0)
      _videoHeight = arg2;
    [self changeNaturalSize];
    break;
  case RED_MSG_SAR_CHANGED:
    // AV_LOGD_ID(TAG, _id, "RED_MSG_SAR_CHANGED: %d, %d\n", arg1, arg2);
    if (arg1 > 0)
      _sampleAspectRatioNumerator = arg1;
    if (arg2 > 0)
      _sampleAspectRatioDenominator = arg2;
    [self changeNaturalSize];
    break;
  case RED_MSG_BUFFERING_START: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_BUFFERING_START: %d\n", arg1);

    _loadState = RedLoadStateStalled;
    _isSeekBuffering = arg1;

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerLoadStateDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    _isSeekBuffering = 0;
    break;
  }
  case RED_MSG_BUFFERING_END: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_BUFFERING_END:\n");
    _loadState = RedLoadStatePlayable | RedLoadStatePlaythroughOK;
    _isSeekBuffering = arg1;

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerLoadStateDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];

    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerPlaybackStateDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    _isSeekBuffering = 0;
    break;
  }
  case RED_MSG_BUFFERING_UPDATE:
    _bufferingPosition = arg1;
    _bufferingProgress = arg2;
    break;
  case RED_MSG_BUFFERING_BYTES_UPDATE:
    break;
  case RED_MSG_BUFFERING_TIME_UPDATE:
    _bufferingTime = arg1;
    break;
  case RED_MSG_PLAYBACK_STATE_CHANGED:
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerPlaybackStateDidChangeNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  case RED_MSG_SEEK_COMPLETE: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_SEEK_COMPLETE:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerDidSeekCompleteNotification
                      object:self
                    userInfo:@{
                      RedPlayerDidSeekCompleteTargetKey : @(arg1),
                      RedPlayerDidSeekCompleteErrorKey : @(arg2),
                      RedPlayerMessageTimeUserInfoKey : @(time)
                    }];
    _seeking = NO;
    break;
  }
  case RED_MSG_VIDEO_DECODER_OPEN: {
    _isVideoToolboxOpen = arg1;
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_DECODER_OPEN: %s\n",
    // _isVideoToolboxOpen ? "true" : "false");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerVideoDecoderOpenNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_VIDEO_RENDERING_START: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_RENDERING_START=%lld, arg1=%d\n",
    // CurrentTimeMs(), arg1);
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerFirstVideoFrameRenderedNotification
                      object:self
                    userInfo:@{
                      RedPlayerFirstVideoFrameRenderedNotificationUserInfo :
                          @(arg1),
                      RedPlayerMessageTimeUserInfoKey : @(time)
                    }];
    break;
  }
  case RED_MSG_AUDIO_RENDERING_START: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_AUDIO_RENDERING_START:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerFirstAudioFrameRenderedNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_AUDIO_DECODED_START: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_AUDIO_DECODED_START:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerFirstAudioFrameDecodedNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_VIDEO_DECODED_START: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_DECODED_START:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerFirstVideoFrameDecodedNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_OPEN_INPUT: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_OPEN_INPUT:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerOpenInputNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_FIND_STREAM_INFO: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_FIND_STREAM_INFO:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerFindStreamInfoNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_COMPONENT_OPEN: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_COMPONENT_OPEN:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerComponentOpenNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_ACCURATE_SEEK_COMPLETE: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_ACCURATE_SEEK_COMPLETE:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerAccurateSeekCompleteNotification
                      object:self
                    userInfo:@{
                      RedPlayerDidAccurateSeekCompleteCurPos : @(arg1),
                      RedPlayerMessageTimeUserInfoKey : @(time)
                    }];
    break;
  }
  case RED_MSG_VIDEO_SEEK_RENDERING_START: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_SEEK_RENDERING_START:\n");
    _isVideoSync = arg1;
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerSeekVideoStartNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    _isVideoSync = 0;
    break;
  }

  case RED_MSG_URL_CHANGE: {
    NSString *current_url = nil;
    NSString *next_url = nil;
    void *obj1 = msg->mObj1;
    void *obj2 = msg->mObj2;
    if (obj1) {
      current_url = [NSString stringWithUTF8String:(char *)obj1];
      AV_LOGD_ID(TAG, _id, "%s:current url = %s\n", __func__, (char *)obj1);
    }
    if (obj2) {
      next_url = [NSString stringWithUTF8String:(char *)obj2];
      AV_LOGD_ID(TAG, _id, "%s:next url = %s\n", __func__, (char *)obj2);
    }
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerUrlChangeMsgNotification
                      object:self
                    userInfo:@{
                      RedPlayerUrlChangeCurUrlKey : (current_url ?: @""),
                      RedPlayerUrlChangeCurUrlHttpCodeKey : @(arg1),
                      RedPlayerUrlChangeNextUrlKey : (next_url ?: @"")
                    }];
    break;
  }
  case RED_MSG_AUDIO_SEEK_RENDERING_START: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_AUDIO_SEEK_RENDERING_START:\n");
    _isAudioSync = arg1;
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerSeekAudioStartNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    _isAudioSync = 0;
    break;
  }
  case RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_FIRST_PACKET_IN_DECODER:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerVideoFirstPacketInDecoderNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_VIDEO_START_ON_PLAYING: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_START_ON_PLAYING:\n");
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerVideoStartOnPlayingNotification
                      object:self
                    userInfo:@{RedPlayerMessageTimeUserInfoKey : @(time)}];
    break;
  }
  case RED_MSG_VIDEO_ROTATION_CHANGED: {
    // AV_LOGD_ID(TAG, _id, "RED_MSG_VIDEO_ROTATION_CHANGED:\n");
    _rotateDegrees = arg1;
    if (_rotateDegrees > 0) {
      CGRect frameBefore = _glView.frame;
      _glView.transform =
          CGAffineTransformMakeRotation(M_PI * _rotateDegrees / 180.0);
      _glView.frame = frameBefore;
    }
    break;
  }
  case RED_MSG_VTB_COLOR_PRIMARIES_SMPTE432: {
    // AV_LOGD_ID(TAG, _id, "FFP_MSG_VTB_COLOR_PRIMARIES_SMPTE432:\n");
    [_glView setSmpte432ColorPrimaries];
    break;
  }
  default:
    AV_LOGD_ID(TAG, _id, "unknown RED_MSG_xxx(%d)\n", msg->mWhat);
    break;
  }

  _mediaPlayer->recycleMessage(msg);
}

inline static RedPlayerController *redPlayerRetain(void *arg) {
  return (__bridge_transfer RedPlayerController *)arg;
}

int media_player_msg_loop(CRedPlayer *mp) {
  @autoreleasepool {
    __weak RedPlayerController *playerController =
        redPlayerRetain(mp->setWeakThiz(nullptr));
    while (playerController) {
      @autoreleasepool {
        sp<Message> msg = mp->getMessage(true);
        if (!msg)
          break;

        int32_t arg1 = msg->mArg1, arg2 = msg->mArg2;

        switch (msg->mWhat) {
        case RED_MSG_FLUSH:
          break;
        case RED_MSG_ERROR: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_ERROR: %d\n", __func__, arg1);
          break;
        }
        case RED_MSG_PREPARED: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_PREPARED:\n", __func__);
          break;
        }
        case RED_MSG_COMPLETED: {
          break;
        }
        case RED_MSG_VIDEO_SIZE_CHANGED:
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_VIDEO_SIZE_CHANGED: %d, %d\n",
                     __func__, arg1, arg2);
          break;
        case RED_MSG_SAR_CHANGED:
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_SAR_CHANGED: %d, %d\n",
                     __func__, arg1, arg2);
          break;
        case RED_MSG_BUFFERING_START: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_BUFFERING_START:%d\n", __func__,
                     arg1);
          break;
        }
        case RED_MSG_BUFFERING_END: {
          AV_LOGD_ID(TAG, mp->id(), "RED_MSG_BUFFERING_END:\n");
          break;
        }
        case RED_MSG_BUFFERING_UPDATE:
          break;
        case RED_MSG_BUFFERING_BYTES_UPDATE:
          break;
        case RED_MSG_BUFFERING_TIME_UPDATE:
          break;
        case RED_MSG_PLAYBACK_STATE_CHANGED:
          break;
        case RED_MSG_SEEK_COMPLETE: {
          break;
        }
        case RED_MSG_SEEK_LOOP_START: {
          break;
        }
        case RED_MSG_VIDEO_DECODER_OPEN: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_VIDEO_DECODER_OPEN\n",
                     __func__);
          break;
        }
        case RED_MSG_VIDEO_RENDERING_START: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_VIDEO_RENDERING_START=%lld\n",
                     __func__, CurrentTimeMs());
          break;
        }
        case RED_MSG_AUDIO_RENDERING_START: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_AUDIO_RENDERING_START:\n",
                     __func__);
          break;
        }
        case RED_MSG_AUDIO_DECODED_START: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_AUDIO_DECODED_START:\n",
                     __func__);
          break;
        }
        case RED_MSG_VIDEO_DECODED_START: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_VIDEO_DECODED_START:\n",
                     __func__);
          break;
        }
        case RED_MSG_OPEN_INPUT: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_OPEN_INPUT:\n", __func__);
          break;
        }
        case RED_MSG_FIND_STREAM_INFO: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_FIND_STREAM_INFO:\n", __func__);
          break;
        }
        case RED_MSG_COMPONENT_OPEN: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_COMPONENT_OPEN:\n", __func__);
          break;
        }
        case RED_MSG_ACCURATE_SEEK_COMPLETE: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_ACCURATE_SEEK_COMPLETE:\n",
                     __func__);
          break;
        }
        case RED_MSG_VIDEO_SEEK_RENDERING_START: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_VIDEO_SEEK_RENDERING_START:\n",
                     __func__);
          break;
        }
        case RED_MSG_AUDIO_SEEK_RENDERING_START: {
          AV_LOGD_ID(TAG, mp->id(), "%s:RED_MSG_AUDIO_SEEK_RENDERING_START:\n",
                     __func__);
          break;
        }
        default:
          break;
        }

        dispatch_async(dispatch_get_main_queue(), ^{
          [playerController postEvent:msg];
        });
      }
    }

    return 0;
  }
}

#pragma mark av_format_control_message

static int onInectREDIOStatistic(RedPlayerController *mpc, int type,
                                 void *data) {
  RedCacheStatisticWrapper *realData = (RedCacheStatisticWrapper *)data;
  assert(realData);

  mpc->_cacheStat = *realData;
  NSRange realUrlStartRange = [mpc->_urlString rangeOfString:@"http"];
  if (realUrlStartRange.location == NSNotFound) {
    return 0;
  }

  NSString *realUrl =
      [mpc->_urlString substringFromIndex:realUrlStartRange.location];
  NSURL *URL = [NSURL URLWithString:realUrl];
  if (URL && realData->cached_size == realData->logical_file_size) {
    [[NSNotificationCenter defaultCenter]
        postNotificationName:RedPlayerCacheDidFinishNotification
                      object:mpc
                    userInfo:@{RedPlayerCacheURLUserInfoKey : URL}];
  }

  return 0;
}

static int64_t calculateElapsed(int64_t begin, int64_t end) {
  if (begin <= 0)
    return -1;

  if (end < begin)
    return -1;

  return end - begin;
}

static int onInjectOnDnsEvent(RedPlayerController *mpc, int type, void *data) {
  return 0;
}

static int red_inject_callback(void *opaque, int message, void *data) {
  REDWeakHolder *weakHolder = (__bridge REDWeakHolder *)opaque;
  RedPlayerController *mpc = weakHolder.object;
  if (!mpc)
    return 0;

  switch (message) {
  case REDIOAPP_EVENT_CACHE_STATISTIC:
    return onInectREDIOStatistic(mpc, message, data);
  case RED_EVENT_WILL_DNS_PARSE_W:
  case RED_EVENT_DID_DNS_PARSE_W:
    return onInjectOnDnsEvent(mpc, message, data);
  default: {
    return 0;
  }
  }
}

static bool red_selecturl_callback(void *opaque, const char *oldUrl,
                                   int reconnectType, char *outUrl,
                                   int outLen) {
  return false;
}

#pragma mark Option Conventionce

- (void)setMaxBufferSize:(int)maxBufferSize {
  if (_mediaPlayer) {
    _mediaPlayer->setConfig(cfgTypePlayer, "max-buffer-size",
                            (int64_t)maxBufferSize);
  }
}

#pragma mark app state changed

- (void)registerApplicationObservers {
  [_notificationManager addObserver:self
                           selector:@selector(applicationWillEnterForeground)
                               name:UIApplicationWillEnterForegroundNotification
                             object:nil];

  [_notificationManager addObserver:self
                           selector:@selector(applicationDidBecomeActive)
                               name:UIApplicationDidBecomeActiveNotification
                             object:nil];

  [_notificationManager addObserver:self
                           selector:@selector(applicationWillResignActive)
                               name:UIApplicationWillResignActiveNotification
                             object:nil];

  [_notificationManager addObserver:self
                           selector:@selector(applicationDidEnterBackground)
                               name:UIApplicationDidEnterBackgroundNotification
                             object:nil];

  [_notificationManager addObserver:self
                           selector:@selector(applicationWillTerminate)
                               name:UIApplicationWillTerminateNotification
                             object:nil];
}

- (void)unregisterApplicationObservers {
  [_notificationManager removeAllObservers:self];
}

- (void)applicationWillEnterForeground {
  NSLog(@"RedPlayerController:applicationWillEnterForeground: %d",
        (int)[UIApplication sharedApplication].applicationState);
}

- (void)applicationDidBecomeActive {
  NSLog(@"RedPlayerController:applicationDidBecomeActive: %d",
        (int)[UIApplication sharedApplication].applicationState);
}

- (void)applicationWillResignActive {
  NSLog(@"RedPlayerController:applicationWillResignActive: %d",
        (int)[UIApplication sharedApplication].applicationState);
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self->_pauseInBackground) {
      [self pause];
    }
  });
}

- (void)applicationDidEnterBackground {
  NSLog(@"RedPlayerController:applicationDidEnterBackground: %d",
        (int)[UIApplication sharedApplication].applicationState);
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self->_pauseInBackground) {
      [self pause];
    }
  });
}

- (void)applicationWillTerminate {
  NSLog(@"RedPlayerController:applicationWillTerminate: %d",
        (int)[UIApplication sharedApplication].applicationState);
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self->_pauseInBackground) {
      [self pause];
    }
  });
}

@end
