package com.xingin.openredplayer

import android.app.Application
import com.kelin.photoselector.PhotoSelector

class XhsMediaApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        instance = this
        // 初始化相册选择
        PhotoSelector.init(
            this,
            this.packageName + ".fileProvider",
            true,
            100,
            false
        )
    }

    companion object {
        lateinit var instance: XhsMediaApplication
            private set
    }
}