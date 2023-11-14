package com.xingin.openredplayercore.core.impl.android;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaDataSource;
import android.media.MediaPlayer;
import android.media.TimedText;
import android.net.Uri;
import android.text.TextUtils;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.xingin.openredplayercore.core.impl.RedPlayerEvent;
import com.xingin.openredplayercore.core.impl.AbstractMediaPlayer;
import com.xingin.openredplayercore.core.impl.android.util.DebugLog;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.Map;

public class AndroidMediaPlayer extends AbstractMediaPlayer {
    private final MediaPlayer mInternalMediaPlayer;
    private final AndroidMediaPlayerListenerHolder mInternalListenerAdapter;
    private String mDataSource;
    private MediaDataSource mMediaDataSource;

    private final Object mInitLock = new Object();
    private boolean mIsReleased;

    public AndroidMediaPlayer() {
        synchronized (mInitLock) {
            mInternalMediaPlayer = new MediaPlayer();
        }
        mInternalMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        mInternalListenerAdapter = new AndroidMediaPlayerListenerHolder(this);
        attachInternalListeners();
    }

    public MediaPlayer getInternalMediaPlayer() {
        return mInternalMediaPlayer;
    }

    @Override
    public void setDisplay(SurfaceHolder sh) {
        synchronized (mInitLock) {
            if (!mIsReleased) {
                mInternalMediaPlayer.setDisplay(sh);
            }
        }
    }

    @Override
    public void setSurface(Surface surface) {
        mInternalMediaPlayer.setSurface(surface);
    }

    @Override
    public void setDataSource(Context context, Uri uri)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        mInternalMediaPlayer.setDataSource(context, uri);
    }

    @Override
    public void setDataSource(Context context, Uri uri, Map<String, String> headers)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        mInternalMediaPlayer.setDataSource(context, uri, headers);
    }

    @Override
    public void setDataSource(String path) throws IOException,
            IllegalArgumentException, SecurityException, IllegalStateException {
        mDataSource = path;

        Uri uri = Uri.parse(path);
        String scheme = uri.getScheme();
        if (!TextUtils.isEmpty(scheme) && scheme.equalsIgnoreCase("file")) {
            mInternalMediaPlayer.setDataSource(uri.getPath());
        } else {
            mInternalMediaPlayer.setDataSource(path);
        }
    }

    @Override
    public void setDataSource(String path, Map<String, String> headers) throws IllegalArgumentException, SecurityException, IllegalStateException {
    }

    @Override
    public void setDataSourceJson(Context context, String jsonStr)
            throws IllegalArgumentException, SecurityException, IllegalStateException {
    }

    @Override
    public String getDataSource() {
        return mDataSource;
    }

    private void releaseMediaDataSource() {
        if (mMediaDataSource != null) {
            try {
                mMediaDataSource.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
            mMediaDataSource = null;
        }
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        mInternalMediaPlayer.prepareAsync();
    }

    @Override
    public void start() throws IllegalStateException {
        mInternalMediaPlayer.start();
    }

    @Override
    public void stop() throws IllegalStateException {
        mInternalMediaPlayer.stop();
    }

    @Override
    public void pause() throws IllegalStateException {
        mInternalMediaPlayer.pause();
    }

    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {
        mInternalMediaPlayer.setScreenOnWhilePlaying(screenOn);
    }

    @Override
    public void setEnableMediaCodec(boolean enable) {
    }

    @Override
    public void setVideoCacheDir(String dir) {
    }

    @Override
    public void setLiveMode(boolean enable) {
    }

    @Override
    public String getAudioCodecInfo() {
        return "unknown";
    }

    @Override
    public String getVideoCodecInfo() {
        return "unknown";
    }

    @Override
    public float getVideoFileFps() {
        return 0.0f;
    }

    @Override
    public int getVideoWidth() {
        return mInternalMediaPlayer.getVideoWidth();
    }

    @Override
    public int getVideoHeight() {
        return mInternalMediaPlayer.getVideoHeight();
    }

    @Override
    public int getVideoSarNum() {
        return 1;
    }

    @Override
    public int getVideoSarDen() {
        return 1;
    }

    @Override
    public boolean isPlaying() {
        try {
            return mInternalMediaPlayer.isPlaying();
        } catch (IllegalStateException e) {
            DebugLog.printStackTrace(e);
            return false;
        }
    }

    @Override
    public void seekTo(long msec) throws IllegalStateException {
        mInternalMediaPlayer.seekTo((int) msec);
    }

    @Override
    public long getCurrentPosition() {
        try {
            return mInternalMediaPlayer.getCurrentPosition();
        } catch (IllegalStateException e) {
            DebugLog.printStackTrace(e);
            return 0;
        }
    }

    @Override
    public long getDuration() {
        try {
            return mInternalMediaPlayer.getDuration();
        } catch (IllegalStateException e) {
            DebugLog.printStackTrace(e);
            return 0;
        }
    }

    @Override
    public void release() {
        mIsReleased = true;
        mInternalMediaPlayer.release();
        releaseMediaDataSource();
        resetListeners();
        attachInternalListeners();
    }

    @Override
    public void reset() {
        try {
            mInternalMediaPlayer.reset();
        } catch (IllegalStateException e) {
            DebugLog.printStackTrace(e);
        }
        releaseMediaDataSource();
        resetListeners();
        attachInternalListeners();
    }

    @Override
    public void setLooping(boolean looping) {
        mInternalMediaPlayer.setLooping(looping);
    }

    @Override
    public boolean isLooping() {
        return mInternalMediaPlayer.isLooping();
    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        mInternalMediaPlayer.setVolume(leftVolume, rightVolume);
    }

    @Override
    public String getPlayUrl() throws IllegalStateException {
        return null;
    }

    private void attachInternalListeners() {
        mInternalMediaPlayer.setOnPreparedListener(mInternalListenerAdapter);
        mInternalMediaPlayer
                .setOnBufferingUpdateListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnCompletionListener(mInternalListenerAdapter);
        mInternalMediaPlayer
                .setOnSeekCompleteListener(mInternalListenerAdapter);
        mInternalMediaPlayer
                .setOnVideoSizeChangedListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnErrorListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnInfoListener(mInternalListenerAdapter);
        mInternalMediaPlayer.setOnTimedTextListener(mInternalListenerAdapter);
    }

    private class AndroidMediaPlayerListenerHolder implements
            MediaPlayer.OnPreparedListener, MediaPlayer.OnCompletionListener,
            MediaPlayer.OnBufferingUpdateListener,
            MediaPlayer.OnSeekCompleteListener,
            MediaPlayer.OnVideoSizeChangedListener,
            MediaPlayer.OnErrorListener, MediaPlayer.OnInfoListener,
            MediaPlayer.OnTimedTextListener {
        public final WeakReference<AndroidMediaPlayer> mWeakMediaPlayer;

        public AndroidMediaPlayerListenerHolder(AndroidMediaPlayer mp) {
            mWeakMediaPlayer = new WeakReference<AndroidMediaPlayer>(mp);
        }

        @Override
        public boolean onInfo(MediaPlayer mp, int what, int extra) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            return self != null && notifyOnInfo(what, extra, null);

        }

        @Override
        public boolean onError(MediaPlayer mp, int what, int extra) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            return self != null && notifyOnError(what, extra);

        }

        @Override
        public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnVideoSizeChanged(width, height, 1, 1);
        }

        @Override
        public void onSeekComplete(MediaPlayer mp) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnSeekComplete();
        }

        @Override
        public void onBufferingUpdate(MediaPlayer mp, int percent) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnBufferingUpdate(percent);
        }

        @Override
        public void onCompletion(MediaPlayer mp) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;

            notifyOnCompletion();
        }

        @Override
        public void onPrepared(MediaPlayer mp) {
            AndroidMediaPlayer self = mWeakMediaPlayer.get();
            if (self == null)
                return;
            RedPlayerEvent event = new RedPlayerEvent();
            event.obj = self;
            event.time = System.currentTimeMillis();
            notifyOnPrepared(event);
        }

        @Override
        public void onTimedText(MediaPlayer mp, TimedText text) {
        }
    }

    @Override
    public void setSpeed(float speed) {
    }

    @Override
    public float getSpeed(float speed) {
        return 0.0f;
    }

    @Override
    public int getVideoDecoder() {
        return 0;
    }

    @Override
    public float getVideoOutputFramesPerSecond() {
        return 0.0f;
    }

    @Override
    public float getVideoDecodeFramesPerSecond() {
        return 0.0f;
    }

    @Override
    public long getVideoCachedSizeMs() {
        return 0;
    }

    @Override
    public long getAudioCachedSizeMs() {
        return 0;
    }

    @Override
    public long getVideoCachedSizeBytes() {
        return 0;
    }

    @Override
    public long getAudioCachedSizeBytes() {
        return 0;
    }

    @Override
    public long getVideoCachedSizePackets() {
        return 0;
    }

    @Override
    public long getAudioCachedSizePackets() {
        return 0;
    }

    @Override
    public long getTrafficStatisticByteCount() {
        return 0;
    }

    @Override
    public long getRealCacheBytes() {
        return 0;
    }

    @Override
    public long getFileSize() {
        return 0;
    }

    @Override

    public long getBitRate() {
        return 0;
    }

    @Override
    public long getSeekCostTimeMs() {
        return 0;
    }

    @Override
    public float getDropFrameRate() {
        return 0;
    }

    @Override
    public int getPlayerState() {
        return 0;
    }

    @Override
    public float getDropPacketRateBeforeDecode() {
        return 0.0f;
    }

    @Override
    public int readDns(String path, String referer) {
        return 0;
    }

    @Override
    public void setCallbackLogLevel(int level) {
    }

    @Override
    public int getPlayerCoreType() {
        return MEDIA_PLAYER_ANDROID;
    }
}