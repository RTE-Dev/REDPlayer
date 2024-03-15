package com.xingin.openredplayercore.core.impl.redplayer;

import android.content.ContentResolver;
import android.content.Context;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import com.xingin.openredplayercore.core.impl.RedPlayerEvent;
import com.xingin.openredplayercore.core.loader.RedPlayerLibLoader;
import com.xingin.openredplayercore.core.loader.RedPlayerSoLoader;
import com.xingin.openredplayercore.core.impl.AbstractMediaPlayer;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * RedPlayer Core
 */
public class RedMediaPlayer extends AbstractMediaPlayer {
    private final static String TAG = RedMediaPlayer.class.getName();
    private static final int MEDIA_NOP = 0;
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_BPREPARED = 7;

    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;

    protected static final int MEDIA_SET_VIDEO_SAR = 10001;

    public static final int RED_LOG_UNKNOWN = 0;
    public static final int RED_LOG_DEFAULT = 1;
    public static final int RED_LOG_VERBOSE = 2;
    public static final int RED_LOG_DEBUG = 3;
    public static final int RED_LOG_INFO = 4;
    public static final int RED_LOG_WARN = 5;
    public static final int RED_LOG_ERROR = 6;
    public static final int RED_LOG_FATAL = 7;
    public static final int RED_LOG_SILENT = 8;

    public static final int DECODER_UNKNOWN = 0;
    public static final int DECODER_AVCODEC = 1;
    public static final int DECODER_MEDIACODEC = 2;
    public static final int DECODER_VIDEOTOOLBOX = 3;

    private static int gLogCallBackLevel = RED_LOG_VERBOSE;
    private static NativeLogCallback mNativeLogListener;

    private static AtomicInteger mAtomicInteger = new AtomicInteger(0);
    private int mID;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mVideoSarNum;
    private int mVideoSarDen;
    private String mDataSource;
    private String mUrlList;
    private int mUrlListHttpCode;

    private SurfaceHolder mSurfaceHolder;
    private EventHandler mEventHandler;
    private PowerManager.WakeLock mWakeLock = null;
    private boolean mScreenOnWhilePlaying;
    private boolean mStayAwake;
    private long mDuration;

    /**
     * Default library loader
     * Load them by yourself, if your libraries are not installed at default place.
     */
    private static final RedPlayerLibLoader sLocalLibLoader = RedPlayerSoLoader.sLocalLibLoader;

    /**
     * @param context   Android Context
     * @param libLoader RedLibLoader
     */
    public static void loadLibrariesOnce(Context context, RedPlayerLibLoader libLoader) {
        RedPlayerSoLoader.loadRedLibsOnce(context, libLoader, "red_player");
    }

    public static boolean setHaveLoadLibraries(boolean load) {
        return RedPlayerSoLoader.setHaveLoadLibraries(load);
    }

    private static volatile boolean mIsNativeInitialized = false;

    private static void initNativeOnce() {
        synchronized (RedMediaPlayer.class) {
            if (!mIsNativeInitialized) {
                native_init();
                mIsNativeInitialized = true;
            }
        }
    }

    private static native final void native_init();

    /**
     * Default constructor. Consider using one of the create() methods for
     * synchronously instantiating a RedPlayerCore from a Uri or resource.
     * <p>
     * When done with the RedPlayerCore, you should call {@link #release()}, to
     * free the resources. If not released, too many RedPlayerCore instances
     * may result in an exception.
     * </p>
     */
    public RedMediaPlayer() {
        this(null, sLocalLibLoader);
    }

    public RedMediaPlayer(Context context) {
        this(context, sLocalLibLoader);
    }


    /**
     * do not loadLibaray
     *
     * @param libLoader custom library loader, can be null.
     */
    public RedMediaPlayer(Context context, RedPlayerLibLoader libLoader) {
        initPlayer(context, libLoader);
    }

    private void initPlayer(Context context, RedPlayerLibLoader libLoader) {
        loadLibrariesOnce(context, libLoader);
        initNativeOnce();

        mID = mAtomicInteger.addAndGet(1);
        mDuration = 0L;
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }
        native_setup(new WeakReference<RedMediaPlayer>(this));
    }

    private native void native_setup(Object redplayerThis);

    @Override
    public void setDisplay(SurfaceHolder sh) throws IllegalStateException {
        log(Log.INFO, id() + "setDisplay: " + sh);
        mSurfaceHolder = sh;
        Surface surface;
        if (sh != null) {
            surface = sh.getSurface();
        } else {
            surface = null;
        }
        _setVideoSurface(surface);
    }

    private native void _setVideoSurface(Surface surface) throws IllegalStateException;

    @Override
    public void setDataSource(Context context, Uri uri) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        setDataSource(context, uri, null);
    }

    @Override
    public void setDataSource(Context context, Uri uri, Map<String, String> headers) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        final String scheme = uri.getScheme();
        if (ContentResolver.SCHEME_FILE.equals(scheme)) {
            setDataSource(uri.getPath());
            return;
        } else if (ContentResolver.SCHEME_CONTENT.equals(scheme)
                && Settings.AUTHORITY.equals(uri.getAuthority())) {
            // Redirect ringtones to go directly to underlying provider
            uri = RingtoneManager.getActualDefaultRingtoneUri(context,
                    RingtoneManager.getDefaultType(uri));
            if (uri == null) {
                throw new FileNotFoundException("Failed to resolve default ringtone");
            }
        }
        setDataSource(uri.toString(), headers);
    }

    @Override
    public void setDataSource(String path, Map<String, String> headers)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        if (headers != null && !headers.isEmpty()) {
            StringBuilder sb = new StringBuilder();
            for (Map.Entry<String, String> entry : headers.entrySet()) {
                sb.append(entry.getKey());
                sb.append(":");
                String value = entry.getValue();
                if (!TextUtils.isEmpty(value))
                    sb.append(entry.getValue());
                sb.append("\r\n");
                _setHeaders(sb.toString());
            }
        }
        setDataSource(path);
    }

    private native void _setHeaders(String headers);

    @Override
    public void setDataSourceJson(Context context, String jsonStr) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        _setDataSourceJson(jsonStr);
    }

    @Override
    public void setDataSource(String path) throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
        mDataSource = path;
        _setDataSource(path);
    }

    private native void _setDataSource(String path)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    private native void _setDataSourceJson(String path)
            throws IOException, IllegalArgumentException, SecurityException, IllegalStateException;

    @Override
    public String getDataSource() {
        return mDataSource;
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        log(Log.INFO, id() + "prepareAsync");
        _prepareAsync();
    }

    private native void _prepareAsync() throws IllegalStateException;

    @Override
    public int readDns(String path, String referer) {
        return _read_dns(path, referer);
    }

    private native int _read_dns(String path, String referer);

    @Override
    public void start() throws IllegalStateException {
        log(Log.INFO, id() + "start");
        stayAwake(true);
        _start();
    }

    private native void _start() throws IllegalStateException;

    @Override
    public void stop() throws IllegalStateException {
        log(Log.INFO, id() + "stop");
        stayAwake(false);
        _stop();
    }

    private native void _stop() throws IllegalStateException;

    @Override
    public void pause() throws IllegalStateException {
        log(Log.INFO, id() + "pause");
        stayAwake(false);
        _pause();
    }

    private native void _pause() throws IllegalStateException;

    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {
        if (mScreenOnWhilePlaying != screenOn) {
            if (screenOn && mSurfaceHolder == null) {
                Log.w(TAG,
                        "setScreenOnWhilePlaying(true) is ineffective without a SurfaceHolder");
            }
            mScreenOnWhilePlaying = screenOn;
            updateSurfaceScreenOn();
        }
    }

    private void stayAwake(boolean awake) {
        if (mWakeLock != null) {
            if (awake && !mWakeLock.isHeld()) {
                mWakeLock.acquire();
            } else if (!awake && mWakeLock.isHeld()) {
                mWakeLock.release();
            }
        }
        mStayAwake = awake;
        updateSurfaceScreenOn();
    }

    private void updateSurfaceScreenOn() {
        if (mSurfaceHolder != null) {
            mSurfaceHolder.setKeepScreenOn(mScreenOnWhilePlaying && mStayAwake);
        }
    }

    @Override
    public void setEnableMediaCodec(boolean enable) {
        _setEnableMediaCodec(enable);
    }

    private native void _setEnableMediaCodec(boolean enable);

    @Override
    public void setVideoCacheDir(String dir) {
        _setVideoCacheDir(dir);
    }

    private native void _setVideoCacheDir(String dir);

    @Override
    public void setLiveMode(boolean enable) {
        _setLiveMode(enable);
    }

    private native void _setLiveMode(boolean enable);


    @Override
    public String getAudioCodecInfo() {
        return _getAudioCodecInfo();
    }

    private native String _getAudioCodecInfo();

    @Override
    public String getVideoCodecInfo() {
        return _getVideoCodecInfo();
    }

    private native String _getVideoCodecInfo();

    @Override
    public float getVideoFileFps() {
        return _getVideoFileFps();
    }

    private native float _getVideoFileFps();

    @Override
    public int getVideoWidth() {
        return mVideoWidth;
    }

    @Override
    public int getVideoHeight() {
        return mVideoHeight;
    }

    @Override
    public native boolean isPlaying();

    @Override
    public void seekTo(long msec) throws IllegalStateException {
        log(Log.INFO, id() + "seekTo " + msec + "ms");
        _seekTo(msec);
    }

    private native void _seekTo(long msec) throws IllegalStateException;

    @Override
    public long getCurrentPosition() {
        return _getCurrentPosition();
    }

    private native long _getCurrentPosition();

    @Override
    public long getDuration() {
        if (mDuration <= 0L) {
            mDuration = _getDuration();
        }
        return mDuration;
    }

    private native long _getDuration();

    @Override
    public void release() {
        log(Log.INFO, id() + "release");
        stayAwake(false);
        updateSurfaceScreenOn();
        resetListeners();
        _release();
    }

    private native void _release();

    @Override
    public void reset() {
        log(Log.INFO, id() + "reset");
        stayAwake(false);
        _reset();
        mEventHandler.removeCallbacksAndMessages(null);
        mVideoWidth = 0;
        mVideoHeight = 0;
    }

    private native void _reset();

    @Override
    public void setVolume(float leftVolume, float rightVolume) {
        _setVolume(leftVolume, rightVolume);
    }

    private native void _setVolume(float leftVolume, float rightVolume);

    @Override
    public String getPlayUrl() throws IllegalStateException {
        return _getPlayUrl();
    }

    private native String _getPlayUrl();

    @Override
    public int getVideoSarNum() {
        return mVideoSarNum;
    }

    @Override
    public int getVideoSarDen() {
        return mVideoSarDen;
    }

    @Override
    public void setLooping(boolean looping) {
        int loopCount = looping ? 0 : 1;
        _setLoopCount(loopCount);
    }

    private native void _setLoopCount(int loopCount);

    @Override
    public boolean isLooping() {
        int loopCount = _getLoopCount();
        return loopCount != 1;
    }

    private native int _getLoopCount();

    @Override
    public void setSurface(Surface surface) {
        log(Log.INFO, id() + "setSurface " + surface);
        mSurfaceHolder = null;
        _setVideoSurface(surface);
    }

    @Override
    public void setSpeed(float speed) {
        _setSpeed(speed);
    }

    private native void _setSpeed(float speed);

    @Override
    public float getSpeed(float speed) {
        return _getSpeed(0.0f);
    }

    private native float _getSpeed(float default_value);

    @Override
    public int getVideoDecoder() {
        return _getVideoDecoder(DECODER_UNKNOWN);
    }

    private native int _getVideoDecoder(int default_value);

    @Override
    public float getVideoOutputFramesPerSecond() {
        return _getVideoOutputFramesPerSecond(0.0f);
    }

    private native float _getVideoOutputFramesPerSecond(float default_value);

    @Override
    public float getVideoDecodeFramesPerSecond() {
        return _getVideoDecodeFramesPerSecond(0.0f);
    }

    private native float _getVideoDecodeFramesPerSecond(float default_value);

    @Override
    public long getVideoCachedSizeMs() {
        return _getVideoCachedSizeMs(0);
    }

    private native long _getVideoCachedSizeMs(long default_value);

    @Override
    public long getAudioCachedSizeMs() {
        return _getAudioCachedSizeMs(0);
    }

    private native long _getAudioCachedSizeMs(long default_value);

    @Override
    public long getVideoCachedSizeBytes() {
        return _getVideoCachedSizeBytes(0);
    }

    private native long _getVideoCachedSizeBytes(long default_value);

    @Override
    public long getAudioCachedSizeBytes() {
        return _getAudioCachedSizeBytes(0);
    }

    private native long _getAudioCachedSizeBytes(long default_value);

    @Override
    public long getVideoCachedSizePackets() {
        return _getVideoCachedSizePackets(0);
    }

    private native long _getVideoCachedSizePackets(long default_value);

    @Override
    public long getAudioCachedSizePackets() {
        return _getAudioCachedSizePackets(0);
    }

    private native long _getAudioCachedSizePackets(long default_value);

    @Override
    public long getTrafficStatisticByteCount() {
        return _getTrafficStatisticByteCount(0);
    }

    private native long _getTrafficStatisticByteCount(long default_value);

    @Override
    public long getRealCacheBytes() {
        return _getRealCacheBytes(0);
    }

    private native long _getRealCacheBytes(long default_value);

    @Override
    public long getFileSize() {
        return _getFileSize(0);
    }

    private native long _getFileSize(long default_value);

    @Override
    public long getBitRate() {
        return _getBitRate(0);
    }

    private native long _getBitRate(long default_value);

    @Override
    public long getSeekCostTimeMs() {
        return _getSeekCostTimeMs(0);
    }

    private native long _getSeekCostTimeMs(long default_value);

    @Override
    public float getDropFrameRate() {
        return _getDropFrameRate(.0f);
    }

    private native float _getDropFrameRate(float default_value);

    @Override
    public float getDropPacketRateBeforeDecode() {
        return _getDropPacketRateBeforeDecode(0.0f);
    }

    private native float _getDropPacketRateBeforeDecode(float default_value);

    @Override
    public int getPlayerState() {
        return _getPlayerState();
    }

    private native int _getPlayerState();

    private String id() {
        return String.format("[id @ %04d] ", mID);
    }

    private static class EventHandler extends Handler {
        private final WeakReference<RedMediaPlayer> mWeakPlayer;

        public EventHandler(RedMediaPlayer mp, Looper looper) {
            super(looper);
            mWeakPlayer = new WeakReference<RedMediaPlayer>(mp);
        }

        @Override
        public void handleMessage(Message msg) {
            RedMediaPlayer player = mWeakPlayer.get();
            if (player == null || player.mID == 0) {
                log(Log.WARN, "RedPlayerCore went away with unhandled events");
                return;
            }

            switch (msg.what) {
                case MEDIA_PREPARED:
                    log(Log.INFO, player.id() + "Info: MEDIA_PREPARED");
                    player.notifyOnPrepared((RedPlayerEvent) msg.obj);
                    return;

                case MEDIA_PLAYBACK_COMPLETE:
                    player.stayAwake(false);
                    player.notifyOnCompletion();
                    return;

                case MEDIA_BUFFERING_UPDATE:
                    return;

                case MEDIA_SEEK_COMPLETE: {
                    player.notifyOnSeekComplete();
                    RedPlayerEvent event = (RedPlayerEvent) msg.obj;
                    event.mseekPos = msg.arg1;
                    player.notifyOnInfo(MEDIA_INFO_MEDIA_SEEK_REQ_COMPLETE, msg.arg2, event);
                    return;
                }

                case MEDIA_SET_VIDEO_SIZE:
                    player.mVideoWidth = msg.arg1;
                    player.mVideoHeight = msg.arg2;
                    player.notifyOnVideoSizeChanged(player.mVideoWidth, player.mVideoHeight,
                            player.mVideoSarNum, player.mVideoSarDen);
                    return;

                case MEDIA_ERROR:
                    log(Log.ERROR, player.id() + "Error (" + msg.arg1 + "," + msg.arg2 + ")");
                    player.notifyOnError(msg.arg1, msg.arg2);
                    if (msg.arg2 >= 0) {
                        player.notifyOnCompletion();
                        player.stayAwake(false);
                    }
                    return;

                case MEDIA_INFO:
                    switch (msg.arg1) {
                        case MEDIA_INFO_VIDEO_RENDERING_START:
                            log(Log.INFO, player.id() + "Info: MEDIA_INFO_VIDEO_RENDERING_START, msg-arg2=" + msg.arg2);
                            break;
                        case MEDIA_INFO_URL_CHANGE:
                            log(Log.INFO, player.id() + "Info: MEDIA_INFO_URL_CHANGE, msg-obj=" + msg.obj + "\n");
                            RedPlayerEvent playEvent = (RedPlayerEvent) msg.obj;
                            if (msg.arg2 != -1) { // 区分current_url与next_url
                                player.mUrlList = (String) playEvent.obj;
                                player.mUrlListHttpCode = msg.arg2;
                                return;
                            } else {
                                player.mUrlList = player.mUrlList + "," + (String) playEvent.obj;
                            }
                            playEvent.obj = player.mUrlList;
                            player.notifyOnInfo(msg.arg1, player.mUrlListHttpCode, playEvent); // url1,url2
                            return;
                    }
                    player.notifyOnInfo(msg.arg1, msg.arg2, (RedPlayerEvent) msg.obj);
                    // No real default action so far.
                    return;

                case MEDIA_NOP: // interface test message - ignore
                    break;

                case MEDIA_SET_VIDEO_SAR:
                    player.mVideoSarNum = msg.arg1;
                    player.mVideoSarDen = msg.arg2;
                    player.notifyOnVideoSizeChanged(player.mVideoWidth, player.mVideoHeight,
                            player.mVideoSarNum, player.mVideoSarDen);
                    break;

                default:
                    // log(Log.ERROR, player.id() + "Unknown message type " + msg.what);
            }
        }
    }

    private static void postEventFromNative(Object weakThiz, long ctime, int what,
                                            int arg1, int arg2, Object obj) {
        if (weakThiz == null)
            return;

        RedMediaPlayer mp = (RedMediaPlayer) ((WeakReference) weakThiz).get();
        if (mp == null) {
            return;
        }

        if (mp.mEventHandler != null) {
            RedPlayerEvent event = new RedPlayerEvent();
            event.obj = obj;
            if (ctime > 0)
                event.time = ctime;
            else
                event.time = System.currentTimeMillis();
            event.tcpCount = mp.getTrafficStatisticByteCount();
            Message m = mp.mEventHandler.obtainMessage(what, arg1, arg2, event);
            mp.mEventHandler.sendMessage(m);
        }
    }

    private static void onNativeLog(int level, String tag, byte[] logContent) {
        if (level < gLogCallBackLevel) {
            return;
        }
        String logContentStr = "";
        try {
            logContentStr = new String(logContent, "utf-8");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        if (mNativeLogListener != null) {
            mNativeLogListener.onLogOutput(level, tag, logContentStr);
        } else {
            Log.println(level, tag, logContentStr);
        }
    }

    private static void log(int level, String log) {
        if (level < gLogCallBackLevel) {
            return;
        }
        if (mNativeLogListener != null) {
            mNativeLogListener.onLogOutput(level, TAG, log);
        } else {
            Log.println(level, TAG, log);
        }
    }

    public static void setNativeLogCallback(NativeLogCallback nativeLogCallback) {
        mNativeLogListener = nativeLogCallback;
    }

    public interface NativeLogCallback {
        void onLogOutput(int logLevel, String tag, String log);
    }

    @Override
    public void setCallbackLogLevel(int level) {
        gLogCallBackLevel = level;
        synchronized (RedMediaPlayer.class) {
            if (mIsNativeInitialized) {
                native_setCallbackLogLevel(gLogCallBackLevel);
            }
        }
    }

    public static native void native_setCallbackLogLevel(int level);

    @Override
    public int getPlayerCoreType() {
        return MEDIA_PLAYER_RED;
    }
}
