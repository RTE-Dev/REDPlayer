package com.xingin.openredplayercore.core.loader;

import android.content.Context;

public class RedPlayerSoLoader {
    private final static String TAG = RedPlayerSoLoader.class.getName();

    /**
     * Default library loader
     * Load them by yourself, if your libraries are not installed at default place.
     */
    public static final RedPlayerLibLoader sLocalLibLoader = (context, libName) -> System.loadLibrary(libName);

    private static volatile boolean sIsNativeLibsLoaded = false;

    /**
     * @return soLib has loaded
     */
    public static boolean isNativeLibsLoaded() {
        synchronized (sLoadSoLibsLock) {
            return sIsNativeLibsLoaded;
        }
    }

    public static boolean setHaveLoadLibraries(boolean load) {
        synchronized (sLoadSoLibsLock) {
            boolean prevLibLoaded = sIsNativeLibsLoaded;
            sIsNativeLibsLoaded = load;
            return prevLibLoaded;
        }
    }

    private static final Object sLoadSoLibsLock = new Object();

    /**
     * @param context   Android Context
     * @param libLoader RedPlayerLibLoader
     */
    public static void loadRedLibsOnce(Context context, RedPlayerLibLoader libLoader, String tag) {
        synchronized (sLoadSoLibsLock) {
            if (!sIsNativeLibsLoaded) {
                long curTime = System.currentTimeMillis();
                if (libLoader == null)
                    libLoader = sLocalLibLoader;
                libLoader.loadLibrary(context, "c++_shared");
                libLoader.loadLibrary(context, "ffmpeg");
                libLoader.loadLibrary(context, "redbase");
                libLoader.loadLibrary(context, "reddownload");
                libLoader.loadLibrary(context, "redstrategycenter");
                libLoader.loadLibrary(context, "redsource");
                libLoader.loadLibrary(context, "redrender");
                libLoader.loadLibrary(context, "reddecoder");
                libLoader.loadLibrary(context, "soundtouch");
                libLoader.loadLibrary(context, "redplayer");
                sIsNativeLibsLoaded = true;
                if (sSoLoadCallback != null) {
                    sSoLoadCallback.onFinishSoLoad(tag, System.currentTimeMillis() - curTime);
                }
            }
        }
    }

    private static SoLoadCallback sSoLoadCallback;

    public static void setSoLoadCallback(SoLoadCallback soLoadCallback) {
        sSoLoadCallback = soLoadCallback;
    }

    public interface SoLoadCallback {
        /**
         * so load success
         */
        void onFinishSoLoad(String tag, long cost);
    }
}
