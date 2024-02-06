package com.xingin.openredplayercore.core.api;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.IOException;
import java.util.Map;

import com.xingin.openredplayercore.core.impl.RedPlayerEvent;

/**
 * MediaPlayer Core API
 */
public interface IMediaPlayer {
    int MEDIA_INFO_UNKNOWN = 1;
    int MEDIA_INFO_STARTED_AS_NEXT = 2;
    int MEDIA_INFO_VIDEO_RENDERING_START = 3;
    int MEDIA_INFO_VIDEO_TRACK_LAGGING = 700;
    int MEDIA_INFO_BUFFERING_START = 701;
    int MEDIA_INFO_BUFFERING_END = 702;
    int MEDIA_INFO_NETWORK_BANDWIDTH = 703;
    int MEDIA_INFO_BAD_INTERLEAVING = 800;
    int MEDIA_INFO_NOT_SEEKABLE = 801;
    int MEDIA_INFO_METADATA_UPDATE = 802;
    int MEDIA_INFO_TIMED_TEXT_ERROR = 900;
    int MEDIA_INFO_UNSUPPORTED_SUBTITLE = 901;
    int MEDIA_INFO_SUBTITLE_TIMED_OUT = 902;

    int MEDIA_INFO_VIDEO_ROTATION_CHANGED = 10001;
    int MEDIA_INFO_AUDIO_RENDERING_START = 10002;
    int MEDIA_INFO_AUDIO_DECODED_START = 10003;
    int MEDIA_INFO_VIDEO_DECODED_START = 10004;
    int MEDIA_INFO_OPEN_INPUT = 10005;
    int MEDIA_INFO_FIND_STREAM_INFO = 10006;
    int MEDIA_INFO_COMPONENT_OPEN = 10007;
    int MEDIA_INFO_VIDEO_SEEK_RENDERING_START = 10008;
    int MEDIA_INFO_AUDIO_SEEK_RENDERING_START = 10009;
    int MEDIA_INFO_VIDEO_FIRST_PACKET_IN_DECODER = 10010;
    int MEDIA_INFO_VIDEO_START_ON_PLAYING = 10011;
    int MEDIA_INFO_URL_CHANGE = 10014;

    int MEDIA_INFO_MEDIA_ACCURATE_SEEK_COMPLETE = 10100;
    int MEDIA_INFO_MEDIA_SEEK_LOOP_COMPLETE = 10101;
    int MEDIA_INFO_MEDIA_SEEK_REQ_COMPLETE = 10102;

    int MEDIA_ERROR_UNKNOWN = 1;
    int MEDIA_ERROR_SERVER_DIED = 100;
    int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;
    int MEDIA_ERROR_IO = -1004;
    int MEDIA_ERROR_MALFORMED = -1007;
    int MEDIA_ERROR_UNSUPPORTED = -1010;
    int MEDIA_ERROR_TIMED_OUT = -110;

    int LOG_UNKNOWN = 0;
    int LOG_DEFAULT = 1;
    int LOG_VERBOSE = 2;
    int LOG_DEBUG = 3;
    int LOG_INFO = 4;
    int LOG_WARN = 5;
    int LOG_ERROR = 6;
    int LOG_FATAL = 7;
    int LOG_SILENT = 8;

    int DECODER_UNKNOWN = 0;
    int DECODER_AVCODEC = 1;
    int DECODER_MEDIACODEC = 2;

    // core type
    int MEDIA_PLAYER_ANDROID = 0;
    int MEDIA_PLAYER_RED = 1;

    void setOnPreparedListener(OnPreparedListener listener);

    void setOnCompletionListener(OnCompletionListener listener);

    void setOnBufferingUpdateListener(OnBufferingUpdateListener listener);

    void setOnSeekCompleteListener(OnSeekCompleteListener listener);

    void setOnVideoSizeChangedListener(OnVideoSizeChangedListener listener);

    void setOnErrorListener(OnErrorListener listener);

    void setOnInfoListener(OnInfoListener listener);

    void setLooping(boolean looping);

    void setSpeed(float speed);

    void setSurface(Surface surface);

    void setDisplay(SurfaceHolder sh);

    /**
     * enable or disable hardware decoders
     */
    void setEnableMediaCodec(boolean enable);

    /**
     * Set video cache path: storage location for streaming and downloading videos
     */
    void setVideoCacheDir(String dir);

    void setLiveMode(boolean enable);

    void setDataSource(Context context, Uri uri) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    void setDataSource(Context context, Uri uri, Map<String, String> headers) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    void setDataSource(String path) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    void setDataSource(String path, Map<String, String> headers) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    /**
     * set up JSON multi-bitrate data source
     */
    void setDataSourceJson(Context context, String jsonStr) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    void setVolume(float leftVolume, float rightVolume);

    void setScreenOnWhilePlaying(boolean screenOn);

    void setCallbackLogLevel(int level);

    void prepareAsync() throws IllegalStateException;

    void start() throws IllegalStateException;

    void pause() throws IllegalStateException;

    void seekTo(long msec) throws IllegalStateException;

    void stop() throws IllegalStateException;

    void release();

    void reset();

    /**
     * read dns: This method can be used for DNS preheating to speed up the playback start time.
     */
    int readDns(String path, String referer);

    /**
     * get audio codec information
     */
    String getAudioCodecInfo();

    /**
     * get video codec information
     */
    String getVideoCodecInfo();

    int getVideoWidth();

    int getVideoHeight();

    /**
     * get current playback position
     */
    long getCurrentPosition();

    /**
     * get total duration of media file
     */
    long getDuration();

    float getSpeed(float speed);

    /**
     * get the final playback URL
     */
    String getPlayUrl() throws IllegalStateException;

    int getVideoSarNum();

    int getVideoSarDen();

    int getVideoDecoder();

    /**
     * get video file frame rate
     */
    float getVideoFileFps();

    /**
     * get video output frame rate
     */
    float getVideoOutputFramesPerSecond();

    /**
     * get video decoding frame rate
     */
    float getVideoDecodeFramesPerSecond();

    /**
     * et video cached duration
     */
    long getVideoCachedSizeMs();

    /**
     * get audio cached duration
     */
    long getAudioCachedSizeMs();

    /**
     * get video cached byte count
     */
    long getVideoCachedSizeBytes();

    /**
     * get audio cached byte count
     */
    long getAudioCachedSizeBytes();

    /**
     * get video cached packet count
     */
    long getVideoCachedSizePackets();

    /**
     * get audio cached packet count
     */
    long getAudioCachedSizePackets();

    long getTrafficStatisticByteCount();

    long getRealCacheBytes();

    /**
     * get video file size
     */
    long getFileSize();

    /**
     * get video bit rate
     */
    long getBitRate();

    /**
     * get seek time cost
     */
    long getSeekCostTimeMs();

    /**
     * get frame drop rate
     */
    float getDropFrameRate();

    /**
     * get frame drop rate before decoding
     */
    float getDropPacketRateBeforeDecode();

    int getPlayerState();

    String getDataSource();

    int getPlayerCoreType();

    boolean isPlaying();

    boolean isLooping();

    interface OnPreparedListener {
        void onPrepared(IMediaPlayer mp, RedPlayerEvent event);
    }

    interface OnCompletionListener {
        void onCompletion(IMediaPlayer mp);
    }

    interface OnBufferingUpdateListener {
        void onBufferingUpdate(IMediaPlayer mp, int percent);
    }

    interface OnSeekCompleteListener {
        void onSeekComplete(IMediaPlayer mp);
    }

    interface OnStartOrPauseListener {
        void onVideoStartOrPause(IMediaPlayer mp, boolean isPlaying, String url);
    }

    interface OnVideoSizeChangedListener {
        void onVideoSizeChanged(IMediaPlayer mp, int width, int height, int sar_num, int sar_den);
    }

    interface OnErrorListener {
        boolean onError(IMediaPlayer mp, int what, int extra);
    }

    interface OnInfoListener {
        boolean onInfo(IMediaPlayer mp, int what, int extra, RedPlayerEvent event);
    }
}
