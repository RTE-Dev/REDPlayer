package com.xingin.openredpreload;

import android.os.Bundle;

import androidx.annotation.Nullable;

import java.lang.ref.WeakReference;

public class OpenRedPreload {

    private final static String TAG = OpenRedPreload.class.getName();

    /**
     * preload traffic info(Bundle)
     * "bytes": int -- traffic
     */
    public static final int PRELOAD_EVENT_IO_TRAFFIC = 0x30001;

    public static final int PRELOAD_EVENT_SPEED = 0x30002;

    /**
     * http事件(Bundle)
     * "error": int -- =0：no error
     * "wan_ip": String -- ip
     * "http_rtt": int -- tcp to fist data pacage time
     */
    public static final int PRELOAD_EVENT_DID_HTTP_OPEN = 0x30005;
    /**
     * tcp event(Bundle)
     * "error": int -- =0：no error
     * "dst_ip": String -- ip
     * "tcp_rtt": int -- tcp cost
     */
    public static final int PRELOAD_EVENT_DID_TCP_OPEN = 0x30006;
    /**
     * error event(Bundle)
     * "error": int -- =0：no error
     */
    public static final int PRELOAD_EVENT_ERROR = 0x30008;

    public interface DownloadEventListener {
        void onEvent(int event, @Nullable Bundle param, @Nullable String url, @Nullable Object userData);
    }

    private String mDataSource;
    private String mDataSourceJson;

    static {
        System.loadLibrary("redbase");
        System.loadLibrary("reddownload");
        System.loadLibrary("redstrategycenter");
        System.loadLibrary("redpreload");
    }

    private static void onEvent(Object obj, int event, String url, Bundle param) {
        if (!(obj instanceof PreloadObject)) {
            return;
        }
        WeakReference<DownloadEventListener> weakListener = ((PreloadObject) obj).listener;
        if (weakListener == null) {
            return;
        }
        @SuppressWarnings("unchecked")
        DownloadEventListener listener = weakListener.get();
        if (listener != null) {
            listener.onEvent(event, param, url, ((PreloadObject) obj).userData);
        }
    }

    /**
     * start preload
     *
     * @param param: "downloadtype": int
     *               "capacity": long
     *               "use_https": int
     *               "is_json": int
     *               "referer": String
     *               "user_agent": String
     *               "header": String
     */
    public void open(String url, String cachePath, long downloadSize, @Nullable WeakReference<DownloadEventListener> weakListener, @Nullable Bundle param, @Nullable Object userData) {
        Object obj = new PreloadObject(weakListener, userData);
        mDataSource = _open(url, cachePath, downloadSize, obj, param);
    }

    public void release() {
        if (mDataSource != null && mDataSource != "")
            _release(mDataSource);
    }

    public void stop() {
        _stop();
    }

    public int deleteCache(String cachePath, String uri, boolean is_full_url) {
        return _deleteCache(cachePath, uri, is_full_url ? 1 : 0);
    }

    public String[] getAllCachedFile(String cachePath) {
        return _getAllCachedFile(cachePath);
    }

    public String getCacheFilePath(String cachePath, String url) {
        return _getCacheFilePath(cachePath, url);
    }

    public long getCachedSize(String path, String uri, boolean is_full_url) {
        return _getCacheSize(path, uri, is_full_url ? 1 : 0);
    }

    private native String _open(String url, String cachePath, long downloadSize, Object preloadObj, Bundle param);


    private native void _release(String url);

    private native void _stop();

    private native int _deleteCache(String path, String uri, int is_full_url);

    private native String[] _getAllCachedFile(String path);

    private native long _getCacheSize(String path, String uri, int is_full_url);

    private native String _getCacheFilePath(String path, String url);
}

class PreloadObject {
    public WeakReference<OpenRedPreload.DownloadEventListener> listener;
    public Object userData;

    public PreloadObject(WeakReference<OpenRedPreload.DownloadEventListener> _weakListener, Object _userData) {
        this.listener = _weakListener;
        this.userData = _userData;
    }
}
