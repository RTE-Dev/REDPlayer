package com.xingin.openredplayercore.core.loader;

import android.content.Context;

public interface RedPlayerLibLoader {
    void loadLibrary(Context context, String libName) throws UnsatisfiedLinkError,
            SecurityException;
}
