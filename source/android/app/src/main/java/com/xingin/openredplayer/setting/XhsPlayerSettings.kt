package com.xingin.openredplayer.setting

import android.content.Context
import android.content.SharedPreferences
import android.preference.PreferenceManager
import com.xingin.openredplayer.R

class XhsPlayerSettings(context: Context) {
    private val mAppContext: Context
    private val mSharedPreferences: SharedPreferences

    init {
        mAppContext = context.applicationContext
        mSharedPreferences = PreferenceManager.getDefaultSharedPreferences(mAppContext)
    }

    val enableSurfaceView: Boolean
        get() {
            val key = mAppContext.getString(R.string.pref_key_enable_surface_view)
            return mSharedPreferences.getBoolean(key, false)
        }

    val enableDebugInfoView: Boolean
        get() {
            val key = mAppContext.getString(R.string.pref_key_enable_debug_info_view)
            return mSharedPreferences.getBoolean(key, false)
        }

    val usingMediaCodec: Boolean
        get() {
            val key = mAppContext.getString(R.string.pref_key_using_media_codec)
            return mSharedPreferences.getBoolean(key, false)
        }

    val enableAndroidMediaPlayer: Boolean
        get() {
            val key = mAppContext.getString(R.string.pref_key_using_android_media_player)
            return mSharedPreferences.getBoolean(key, false)
        }

    val enableVideoAutoLoop: Boolean
        get() {
            val key = mAppContext.getString(R.string.pref_key_video_play_loop)
            return mSharedPreferences.getBoolean(key, false)
        }

    val enableVideoAutoPlayerNext: Boolean
        get() {
            val key = mAppContext.getString(R.string.pref_key_video_play_auto_next)
            return mSharedPreferences.getBoolean(key, false)
        }

    val enableCoverVideoPlayer: Boolean
        get() {
            val key = mAppContext.getString(R.string.pref_key_video_play_cover)
            return mSharedPreferences.getBoolean(key, true)
        }

    val getVideoPlayerSpeed: Float
        get() {
            val key = mAppContext.getString(R.string.pref_key_player_speed)
            return mSharedPreferences.getString(key, "1.0")?.toFloat() ?: 1.0f
        }
}