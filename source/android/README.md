# How to use the Android RedPlayer SDK
This guide provides step-by-step instructions on how to initialize and configure the Android SDK for our open-source video player, This player supports videos in both regular URL and [JSON formats](../../JSON.md). In addition to basic usage, it also supports advanced features such as [data preload](Preload_README.md) and playing while downloading and so on.

📌Integrate RedPlayer into your android studio project [RedPlayerDemo](../android/app/README.md)

📌For video preloading feature, please refer to [RedPreload SDK](Preload_README.md)


## Key Features
- Supports playing videos in both normal URL and [JSON formats](../../JSON.md) 
- Offers multiple rendering methods, including SurfaceView, TextureView
- Provides options to configure the player, such as HDR decoding, autoplay, loop count, and more
- Supports monitoring player status and callbacks for various events

## API References:
* [IMediaPlayer.java](OpenRedPlayerCore/src/main/java/com/xingin/openredplayercore/core/api/IMediaPlayer.java)

## ▶️ Getting Started

## Step 1: Create the player
```java
IMediaPlayer mMediaPlayer =  new RedMediaPlayer();
```

## Step 2: Call SetDataSource
If you are playing a **regular URL**, use the following code: 
```java
mMediaPlayer.setDataSource(mAppContext, url);
```
If you are playing a **JSON format**, use the following code:
```java
mMediaPlayer.setDataSourceJson(mAppContext, jsonUrl);
```

## Step 3: Configure the player
Use MediaCodec decoding if your android system support: **The default is false and use ffmpeg**
```java
mMediaPlayer.setEnableMediaCodec(true);
```
Set playing download cache dir: **The default is null, highly recommend specify a path**
```java
mMediaPlayer.setVideoCacheDir(cacheDir);
```
Set is play in looping mode: **The default is false**
```java
mMediaPlayer.setLoop(true);
```
Set whether to mute: **The default is 1.0**
```java
mMediaPlayer.setVolume(0.0f, 0.0f);
```
Set the Surface or SurfaceHolder to player, which decide you use SurfaceView or TextureView</br>
If you are render use **TextureView**, use the following code: 
```java
mMediaPlayer.setSurface(surface);
```
If you are render use **SurfaceView**, use the following code:
```java
mMediaPlayer.setDisplay(surfaceHolder);
```
Finally, call prepareAsync to start the connection and decoding:

```java
mMediaPlayer.prepareAsync();
```

## Step 4: Common player methods
```java
void start(); // start player
        
void pause(); // pause player
        
void stop(); // stop player: can't start again, need prepareAsync
        
void reset(); // reset player: the player state to IDLE
        
void release(); // release player: the player is null, can't call any method 
```

## Step 5: Player status retrieval
```java
long getCurrentPosition(); // the player current play time
        
long getDuration(); // the total duration of the media resource
        
float getSpeed(); // the player current play speed
        
long getBitRate(); // the media resource current bit rate
        
int getVideoWidth(); // the media resource width
        
int getVideoHeight(); // the media resource height
        
String getPlayUrl(); // the player final play url
        
boolean isPlaying(); // the player is in playing state
        
boolean isLooping(); // the player is in play looping
...
```

## Step 6: Player status monitoring & callbacks
- **OnPreparedListener** When the player is prepared, this listener will be called, you can start playback after this listener.
```java
interface OnPreparedListener {
    void onPrepared(IMediaPlayer mp, RedPlayerEvent event);
}

mMediaPlayer.setOnPreparedListener(preparedListener); // monitor the player prepared callback
```
- **OnVideoSizeChangedListener** When video size changed, this listener will be called, you can update you render view with the new width and height. 
```java
interface OnVideoSizeChangedListener {
    // the width & height params is the new width and height
    void onVideoSizeChanged(IMediaPlayer mp, int width, int height, int sar_num,int sar_den);
}

mMediaPlayer.setOnVideoSizeChangedListener(sizeChangedListener); // monitor the player video size change callback
```

- **OnCompletionListener** When the player is play complete, this listener will be called, you can play next video in this callback.
```java
interface OnCompletionListener {
     void onCompletion(IMediaPlayer mp);
}

mMediaPlayer.setOnCompletionListener(completionListener); // monitor the player play complete callback
```

- **OnSeekCompleteListener** When you seek the player, this listener will be called.
```java
interface OnSeekCompleteListener {
    void onSeekComplete(IMediaPlayer mp);
}

mMediaPlayer.setOnSeekCompleteListener(seekCompleteListener); // monitor the seek complete callback
```

- **OnBufferingUpdateListener** When the player buffering the video stream, this listener willl be called, you can get buffering percent.
```java
interface OnBufferingUpdateListener {
    void onBufferingUpdate(IMediaPlayer mp, int percent);
}

mMediaPlayer.setOnBufferingUpdateListener(bufferingUpdateListener); // monitor the player buffering callback
```

- **OnErrorListener** When the player play error, this listener will be called, you can get error msg and retry the player.
```java
interface OnErrorListener {
    boolean onError(IMediaPlayer mp, int what, int extra);
}

mMediaPlayer.setOnErrorListener(errorListener); // monitor the play error callback
```

- **OnInfoListener** This interface runs through the entire lifecycle of the player, it will be called with many different event.
```java
interface OnInfoListener {
    int MEDIA_INFO_OPEN_INPUT = 10005; // the stream connection established

    int MEDIA_INFO_FIND_STREAM_INFO = 10006; // the audio and video format detection completed
    
    int MEDIA_INFO_COMPONENT_OPEN = 10007; // the decoder initialization completed

    int MEDIA_INFO_VIDEO_FIRST_PACKET_IN_DECODER = 10010; // The first video packet enters the decoder

    int MEDIA_INFO_VIDEO_DECODED_START = 10004; // The first video frame decoding is completed

    int MEDIA_INFO_VIDEO_RENDERING_START = 3; // The first video frame rendering is completed.

    int MEDIA_INFO_BUFFERING_START = 701; // the buffering start

    int MEDIA_INFO_BUFFERING_END = 702; // the buffering end
    
    int MEDIA_INFO_VIDEO_ROTATION_CHANGED = 10001;// the video rotation changed

    int MEDIA_INFO_VIDEO_START_ON_PLAYING = 10011; // the second frame rendering start
    
    boolean onInfo(IMediaPlayer mp, int what, int extra, RedPlayerEvent event);
}

mMediaPlayer.setOnInfoListener(infoListener);  // monitor the player info callback
```

## Step 7: Set Log Callback
```java
interface NativeLogCallback {
    void onLogOutput(int logLevel, String tag, String content);
}

/**
 * Sets the log callback level for the player.
 * @param logLevel The log level for the log callback.
 */
void setLogCallbackLevel(int logLevel);

/**
 * Sets the log callback for the player
 * @param nativeLogCallback The log callback block.
 */
static void setNativeLogCallback(NativeLogCallback nativeLogCallback);
```
