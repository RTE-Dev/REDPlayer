package com.xingin.openredplayer.floating

import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.IBinder
import android.view.LayoutInflater
import android.view.View
import android.widget.ImageView
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.xingin.openredplayer.R
import com.xingin.openredplayer.player.XhsPlayerActivity
import com.xingin.openredplayer.player.XhsVideoPlayerView
import java.io.Serializable

/**
 * Small Window Service
 */
class XhsFloatingService : Service() {
    companion object {
        const val ACTION_SHOW_VIDEO = "action_play_video_show"
        const val ACTION_HIDE_VIDEO = "action_play_video_hide"
        const val INTENT_KEY_VIDEO_URLS = "video_urls"
        const val INTENT_KEY_SHOW_LOADING = "video_show_loading"
        var serviceHasStarted = false
    }

    private lateinit var floatingWindowHelper: XhsFloatingWindowHelper
    private lateinit var videoPlayerLayout: View
    private lateinit var videoCloseView: ImageView
    private lateinit var videoFullScreenView: ImageView
    private lateinit var videoPlayerView: XhsVideoPlayerView

    private var videoUrl: String? = null
    private var showLoading: Boolean? = null

    private var mLocalBroadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            when (intent?.action) {
                ACTION_SHOW_VIDEO -> {
                    showVideo(
                        intent.getStringExtra(INTENT_KEY_VIDEO_URLS),
                        intent.getBooleanExtra(INTENT_KEY_SHOW_LOADING, false),
                    )
                }

                ACTION_HIDE_VIDEO -> {
                    hideVideo()
                }

                else -> {}
            }
        }
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }

    override fun onCreate() {
        super.onCreate()
        serviceHasStarted = true
        LocalBroadcastManager.getInstance(this).apply {
            registerReceiver(mLocalBroadcastReceiver, IntentFilter(ACTION_SHOW_VIDEO))
            registerReceiver(mLocalBroadcastReceiver, IntentFilter(ACTION_HIDE_VIDEO))
        }
        floatingWindowHelper = XhsFloatingWindowHelper(this)
        val layoutInflater = LayoutInflater.from(this)
        videoPlayerLayout = layoutInflater.inflate(R.layout.window_player_layout, null, false)
        videoPlayerView = videoPlayerLayout.findViewById(R.id.window_video_view)
        videoCloseView = videoPlayerLayout.findViewById(R.id.window_close_view)
        videoFullScreenView = videoPlayerLayout.findViewById(R.id.window_full_screen_view)
        videoCloseView.setOnClickListener {
            hideVideo()
        }
        videoFullScreenView.setOnClickListener {
            handleFullScreenEvent()
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        showVideo(
            intent?.getStringExtra(INTENT_KEY_VIDEO_URLS),
            intent?.getBooleanExtra(INTENT_KEY_SHOW_LOADING, false),
        )
        return super.onStartCommand(intent, flags, startId)
    }

    private fun showVideo(url: String?, loading: Boolean?) {
        hideVideo()
        videoUrl = url
        showLoading = loading
        videoPlayerView.setOnCompletionListener {
            handleVideoPlayCompleteEvent()
        }
        videoPlayerView.setVideoPath(url)
        floatingWindowHelper.addView(videoPlayerLayout, true)
    }

    private fun hideVideo() {
        videoPlayerView.release()
        floatingWindowHelper.removeView()
    }

    private fun handleVideoPlayCompleteEvent() {
        videoPlayerView.seekTo(0)
        videoPlayerView.start()
    }

    private fun handleFullScreenEvent() {
        val intent = Intent(this, XhsPlayerActivity::class.java)
        intent.putExtra(XhsPlayerActivity.INTENT_KEY_URLS, listOf(videoUrl) as Serializable)
        intent.putExtra(XhsPlayerActivity.INTENT_KEY_SHOW_LOADING, showLoading)
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
        startActivity(intent)
    }

    override fun onDestroy() {
        super.onDestroy()
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mLocalBroadcastReceiver)
        floatingWindowHelper.clear()
        serviceHasStarted = false
    }
}