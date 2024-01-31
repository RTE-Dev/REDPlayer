# How to use the iOS Red Player SDK

This guide provides step-by-step instructions on how to initialize and configure the iOS SDK for our open-source video player. This player supports videos in both regular URL and [JSON formats](../../JSON.md) and offers multiple rendering methods, including Metal, OpenGL, and SampleBufferDisplayLayer.

📌Integrate RedPlayer into your Xcode project [RedPlayerDemo](./RedPlayerDemo)

📌For video preloading feature, please refer to [RedPreload_SDK](RedPreload_SDK.md)


## Key Features

- Supports playing videos in both normal URL and [JSON formats](../../JSON.md)
- Offers multiple rendering methods, including Metal, OpenGL, and SampleBufferDisplayLayer
- Provides options to configure the player, such as HDR decoding, autoplay, loop count, and more
- Supports monitoring player status and callbacks for various events

## Header Files References:

* [RedMediaPlayback.h](RedPlayer/Sources/RedPlayer.framework/Headers/RedMediaPlayBack.h)

* [RedPlayerController.h](RedPlayer/Sources/RedPlayer.framework/Headers/RedPlayerController.h) 



## ▶️ Getting Started

## Step 1: Player Initialization & Datasource Setting

For **regular URL** playback, use:

```objective-c
self.player = [[RedPlayerController alloc] initWithContentURL:[NSURL urlWithString:@"http://xxx.mp4"]; withRenderType:RedRenderTypeMetal/RedRenderTypeOpenGL/RedRenderTypeSampleBufferDisplayLayer];
```

For [JSON formats](../../JSON.md) playback, use:

```objective-c
self.player = [[RedPlayerController alloc] initWithContentURLString:@"" withRenderType:RedRenderTypeMetal/RedRenderTypeOpenGL/RedRenderTypeSampleBufferDisplayLayer];
[self.player setContentString:Json];
```

## Step 2: Configure the player

* Use HDR decoding method if the video is confirmed to be HDR. 
* This will ***force hardware decoding***. 
* **The default is NO**:

```objective-c
[self.player setEnableHDR:YES];
```

* Set the view frame:

```objective-c
self.player.view.frame = self.view.bounds;
```

* Set the view's contentMode. The options are aspectFit, aspectFill, and fill. **The default is aspectFit**:

```objective-c
self.player.scalingMode = RedScalingModeAspectFit;
```

* Set the player to autoplay after prepare. **The default is NO**:

```objective-c
self.player.shouldAutoplay = YES;
```

- Set whether the background rendering continues. **The default is NO**:

```objective-c
self.player.notPauseGlviewBackground = NO;
```

- Set the loop count (0 for infinite loop). **The default is 1**:

```objective-c
[self.player setLoop:self.isLoop ? 0 : 1];
```

- Set whether to mute. **The default is NO**:

```objective-c
[self.player setMute:NO];
```

- Set whether to use hardware decoding (VideoToolbox). **The default is software decoding (NO**):

```objective-c
[self.player setEnableVTB:![RedPlayerSettings sharedInstance].softDecoder];
```

- Set the cache path. **The default is no caching**:

```objective-c
[self.player setVideoCacheDir:[RedMediaUtil cachePath]];
```

- Finally, call **prepareToPlay** to start the connection:

```objective-c
[self.player prepareToPlay];
```

## Step 3: Common player methods

```objective-c
/// Prepares the media for playback.
- (void)prepareToPlay;
/// Starts playback.
- (void)play;
/// Pauses playback.
- (void)pause;
/// Sets whether to pause in the background.
- (void)setPauseInBackground:(BOOL)pause;
/// Seeks to the specified playback time.
- (BOOL)seekCurrentPlaybackTime:(NSTimeInterval)aCurrentPlaybackTime;
/// Sets the loop setting.
- (void)setLoop:(int)loop;
/// Sets the mute status.
- (void)setMute:(BOOL)muted;
/// Shuts down the media playback. This will release the kernal player object
- (void)shutdown;
```

## Step 4: Player status retrieval

```objective-c
/// The current playback time.
@property (nonatomic, assign) NSTimeInterval currentPlaybackTime;

/// The total duration of the media.
@property (nonatomic, readonly) NSTimeInterval duration;

/// The duration of media that can be played without buffering.
@property (nonatomic, readonly) NSTimeInterval playableDuration;

/// Indicates whether the playback is prepared to play.
@property (nonatomic, readonly) BOOL isPreparedToPlay;

/// The current playback state.
@property (nonatomic, readonly) RedPlaybackState playbackState;

/// The current load state.
@property (nonatomic, readonly) RedLoadState loadState;

/// The buffering progress as a percentage.
@property (nonatomic, readonly) NSInteger bufferingProgress;

/// The duration of cached content.
@property (nonatomic, readonly) int64_t cachedDuration;

/// The natural size of the media.
@property (nonatomic, readonly) CGSize naturalSize;

/// The scaling mode for video playback.
@property (nonatomic, assign) RedScalingMode scalingMode;

/// Returns whether the media is currently playing.
- (BOOL)isPlaying;

/// Retrieves the actual playback URL.
- (NSString *)getPlayUrl;

/// Retrieves debug information for video.
- (NSDictionary *)getVideoDebugInfo;
```

## Step 5: Player status monitoring & callbacks

- **Player Prepared** Check if the player is prepared to start playback.

```objective-c
// RedMediaPlaybackIsPreparedToPlayDidChangeNotification
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(mediaIsPreparedToPlayDidChange:)
                                             name:RedMediaPlaybackIsPreparedToPlayDidChangeNotification
                                           object:_player];
```

- **First Frame Rendered** Notification
  - If you call seturl and then play ***without waiting for prepared***, you will only receive one firstFrame with the reason **RedVideoFirstRenderingReasonStart**.
  - If you call seturl, ***wait for prepared***, and then call play, you will receive two firstFrame. The first one with the reason **RedVideoFirstRenderingReasonWaitStart** and the second one with the reason **RedVideoFirstRenderingReasonStart**.


```objective-c
// RedPlayerFirstVideoFrameRenderedNotification
[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didReceiveFirstFrameRenderedNotification:)
   name:RedPlayerFirstVideoFrameRenderedNotification object:_player];

/// Enum for RedVideoFirstRenderingReason
typedef NS_ENUM(NSInteger,RedVideoFirstRenderingReason){
    RedVideoFirstRenderingReasonStart, ///< Start rendering
    RedVideoFirstRenderingReasonWaitStart ///< Wait start rendering，
};
```

- **Player Playback State (RedPlaybackState):** Keep track of the current playback state, whether it's stopped, playing, paused, interrupted, or seeking.

```objective-c
// RedPlayerPlaybackStateDidChangeNotification
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(moviePlayBackStateDidChange:)
                                             name:RedPlayerPlaybackStateDidChangeNotification
                                           object:_player]; 
/// Enum for RedPlaybackState
typedef NS_ENUM(NSInteger, RedPlaybackState) {
    RedPlaybackStateStopped, ///< Playback is stopped
    RedPlaybackStatePlaying, ///< Playback is playing
    RedPlaybackStatePaused, ///< Playback is paused
    RedPlaybackStateInterrupted, ///< Playback is interrupted
    RedPlaybackStateSeekingForward, ///< Playback is seeking forward
    RedPlaybackStateSeekingBackward ///< Playback is seeking backward
};
```

- **Player Load State (RedLoadState):** Track the player's load state to understand its readiness for playback.

```objective-c
// RedPlayerLoadStateDidChangeNotification
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(loadStateDidChange:)
                                             name:RedPlayerLoadStateDidChangeNotification
                                           object:_player];
/// Enum for RedLoadState
typedef NS_OPTIONS(NSUInteger, RedLoadState) {
    RedLoadStateUnknown        = 0, ///< Unknown load state
    RedLoadStatePlayable       = 1 << 0, ///< State when playable
    RedLoadStatePlaythroughOK  = 1 << 1, ///< Playback will be automatically started in this state when shouldAutoplay is YES
    RedLoadStateStalled        = 1 << 2, ///< Playback will be automatically paused in this state, if started
};
```

- **Player Finish State (RedFinishReason):** Monitor the reason for playback termination - whether it ended naturally, due to an error, or user exit.

```objective-c
// RedPlayerPlaybackDidFinishNotification
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(moviePlayBackDidFinish:)
                                             name:RedPlayerPlaybackDidFinishNotification
                                           object:_player]; 
/// Enum for RedFinishReason
typedef NS_ENUM(NSInteger, RedFinishReason) {
    RedFinishReasonPlaybackEnded, ///< Playback ended
    RedFinishReasonPlaybackError, ///< Playback error
    RedFinishReasonUserExited ///< User exited
};                                           
```

- **Stage-by-stage** status callback monitoring - **NotificationName**
  
  - ***RedPlayerOpenInputNotification*** - Triggered when the player opens an input
  - ***RedPlayerVideoDecoderOpenNotification*** - Triggered when the video decoder is opened
  - ***RedPlayerFirstVideoFrameDecodedNotification*** - Triggered when the first video frame is decoded
  - ***RedPlayerFindStreamInfoNotification*** - Triggered when the stream info is found
  - ***RedPlayerComponentOpenNotification*** - Triggered when a component is opened
  - ***RedPlayerVideoStartOnPlayingNotification*** - Triggered when the **video starts playing**
  - ***RedPlayerVideoFirstPacketInDecoderNotification*** - Triggered when the first packet is in the decoder
  - ***RedPlayerNetworkWillHTTPOpenNotification*** - Triggered before the HTTP connection is opened
  - ***RedPlayerNetworkDidHTTPOpenNotification*** - Triggered after the HTTP connection is opened
  
  
  
  ```objective-c
  // add observer for event notifacation
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(notificationEvent:)
                                               name:RedPlayerXXXXXXXXXNotification
                                             object:_player];
  ```
  
  - **URL change notification**


- **Cache Finish Callback:** Receive a callback when caching is complete.

```objective-c
// RedPlayerCacheDidFinishNotification
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(moviePlayCacheDidFinish:)
                                             name:RedPlayerCacheDidFinishNotification
                                           object:_player];
```

- **URL change notification**

```objective-c
// RedPlayerUrlChangeMsgNotification
[[NSNotificationCenter defaultCenter] addObserver:self
                                         selector:@selector(moviePlayUrlChangeMsg:)
                                             name:RedPlayerUrlChangeMsgNotification
                                           object:_player];
```

- **Callback when the playback display area size changes**

```objective-c
// RedPlayerNaturalSizeAvailableNotification
[[NSNotificationCenter defaultCenter] addObserver:self 
                                         selector:@selector(movieNaturalSizeChange:)
                                             name:RedPlayerNaturalSizeAvailableNotification object:self.player];
```



## Step 6: Set Log Callback

```objective-c
/// Callback block for RedLogCallback
typedef void(^RedLogCallback)(RedLogLevel logLevel, NSString *tagInfo, NSString *logContent);

/**
 Sets the log callback level for the RedPlayerController.

 @param logLevel The log level for the log callback.
 */
+ (void)setLogCallbackLevel:(RedLogLevel)logLevel;

/**
 Sets the log callback for the RedPlayerController.

 @param logCallback The log callback block.
 */
+ (void)setLogCallback:(RedLogCallback)logCallback;

/**
 Sets the log callback for the RedPlayerController with a specific log call scene.

 @param logCallback The log callback block.
 @param logCallScene The log call scene.
 */
+ (void)setLogCallback:(RedLogCallback)logCallback logCallScene:(REDLogCallScene)logCallScene;

```

