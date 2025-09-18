package com.xingin.openredplayer.player

import android.annotation.SuppressLint
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.Color
import android.hardware.Sensor
import android.hardware.SensorManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.View
import android.view.WindowManager
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.RelativeLayout
import android.widget.TextView
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.isVisible
import androidx.drawerlayout.widget.DrawerLayout
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.xingin.openredplayer.R
import com.xingin.openredplayer.floating.XhsFloatingService
import com.xingin.openredplayer.floating.XhsFloatingWindowHelper
import com.xingin.openredplayer.player.progress.XhsProgressBar
import com.xingin.openredplayer.player.section.XhsSectionAdapter
import com.xingin.openredplayer.player.section.XhsSectionSpaceItemDecoration
import com.xingin.openredplayer.player.sensor.XhsOrientationListener
import com.xingin.openredplayer.setting.XhsPlayerSettings
import com.xingin.openredplayer.utils.Utils
import com.xingin.openredplayercore.core.api.IMediaPlayer
import kotlin.math.max
import kotlin.math.min

class XhsPlayerActivity : AppCompatActivity(), XhsSectionAdapter.OnSectionItemClickListener {
    companion object {
        private const val TAG = "XhsPlayerActivity"
        const val INTENT_KEY_URLS = "video_urls"
        const val INTENT_KEY_INDEX = "video_index"
        const val INTENT_KEY_SHOW_LOADING = "show_loading"
        const val INTENT_KEY_IS_LIVE = "is_live"
    }

    /** DrawerLayout UI */
    private lateinit var drawerLayout: DrawerLayout
    private lateinit var drawerContentLayout: LinearLayout
    private lateinit var playerContentRV: RecyclerView
    private lateinit var sectionAdapter: XhsSectionAdapter

    /** loading */
    private lateinit var loadingView: RelativeLayout
    private lateinit var backPressedCallback: OnBackPressedCallback

    /** player controller UI */
    private lateinit var videoPlayerView: XhsVideoPlayerView
    private lateinit var videoFrameLayout: RelativeLayout
    private lateinit var videoTopActionLayout: RelativeLayout
    private lateinit var moreView: ImageView
    private lateinit var backView: ImageView
    private lateinit var shareView: ImageView
    private lateinit var likeView: ImageView
    private lateinit var videoBottomActionLayout: RelativeLayout
    private lateinit var progressSeekBar: XhsProgressBar
    private lateinit var playPauseButton: ImageView
    private lateinit var playNextButton: ImageView
    private lateinit var playSectionButton: TextView
    private lateinit var playHVSwitchBtn: ImageView
    private lateinit var lightLayout: RelativeLayout
    private lateinit var lightSeekBar: XhsProgressBar
    private lateinit var voiceLayout: RelativeLayout
    private lateinit var voiceSeekBar: XhsProgressBar
    private lateinit var speedLayout: RelativeLayout

    /** debug info UI **/
    private lateinit var debugContentLayout: View
    private lateinit var debugInfoLayout: LinearLayout
    private lateinit var debugInfoNameView: TextView
    private lateinit var debugInfoValueView: TextView
    private lateinit var debugInfoHideView: TextView

    /** data **/
    private var isShowLoading = false
    private var isLive = false
    private var videoUrls = arrayListOf<String>()
    private var playIndex = 0

    /** scale **/
    private var scaleFactor = 1.0f
    private var oldScaleFactor = 1.0f
    private var startX = 0f
    private var startY = 0f
    private var maxWidthTranslation = 0f
    private var mIsLandscape = false

    private var downX = 0f
    private var downY = 0f
    private var firstTouch = true
    private var changePosition = false
    private var changeBrightness = false
    private var changeVolume = false

    private lateinit var settingConfig: XhsPlayerSettings

    private lateinit var sensorManager: SensorManager
    private var xhsOrientationListener: XhsOrientationListener? = null

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_USER
        setContentView(R.layout.activity_xhs_player_layout)
        initData()
        initView()
        tryHideSmallWindow()
    }

    private fun initData() {
        playIndex = intent.getIntExtra(INTENT_KEY_INDEX, 0)
        isShowLoading = intent.getBooleanExtra(INTENT_KEY_SHOW_LOADING, false)
        isLive = intent.getBooleanExtra(INTENT_KEY_IS_LIVE, false)
        val list = intent.getSerializableExtra(INTENT_KEY_URLS) as List<String>
        videoUrls.addAll(list)
        // init config
        settingConfig = XhsPlayerSettings(this@XhsPlayerActivity)
    }

    private fun initView() {
        initDrawerLayout()
        initVideoViewScale()
        initVideoProgressView()
        initVideoControllerView()
        initDebugInfoView()
        backPressedCallback = object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                tryShowSmallWindow()
                finish()
            }
        }
        onBackPressedDispatcher.addCallback(this, backPressedCallback)
    }

    /** init the gravity sensor  */
    private fun initSensor() {
        sensorManager = getSystemService(SENSOR_SERVICE) as SensorManager
        xhsOrientationListener =
            XhsOrientationListener(object : XhsOrientationListener.OnOrientationChangeListener {
                override fun orientationChanged(newOrientation: Int) {
                    // update the screen orientation
                    requestedOrientation = newOrientation
                }
            })
        sensorManager.registerListener(
            xhsOrientationListener,
            sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER),
            SensorManager.SENSOR_DELAY_NORMAL
        )
    }

    private fun removeSensor() {
        sensorManager.unregisterListener(xhsOrientationListener)
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            mIsLandscape = true
            // 1.0 set the activity full screen: hide the status bar
            window.setFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
            )
            // 1.1 video view fill the screen width and height
            videoPlayerView.requestLayout()
            shareView.visibility = View.VISIBLE
            likeView.visibility = View.VISIBLE
            // 1.2 register the gravity sensor
            initSensor()
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
            mIsLandscape = false
            // 1.0 restore the status bar
            val attrs = window.attributes
            attrs.flags = attrs.flags and WindowManager.LayoutParams.FLAG_FULLSCREEN.inv()
            window.attributes = attrs
            window.clearFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS)
            // 1.1 restore the video view origin width height
            videoPlayerView.requestLayout()
            shareView.visibility = View.GONE
            likeView.visibility = View.GONE
            // 1.2 remove the gravity sensor
            removeSensor()
        }
    }

    override fun onResume() {
        super.onResume()
        videoPlayerView.start()
    }

    override fun onPause() {
        super.onPause()
        videoPlayerView.pause()
    }

    override fun onDestroy() {
        super.onDestroy()
        videoPlayerView.release()
        backPressedCallback.isEnabled = false
    }

    /** pause or resume media player */
    private fun pauseOrResume() {
        if (videoPlayerView.isPlaying) {
            videoPlayerView.pause()
            playPauseButton.setBackgroundResource(R.drawable.icon_play)
        } else {
            videoPlayerView.start()
            playPauseButton.setBackgroundResource(R.drawable.icon_pause)
        }
    }

    /** update activity orientation */
    private fun changeScreenOrientation() {
        requestedOrientation = if (!mIsLandscape) {
            ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        } else {
            ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
        }
    }

    /** update the video play progress */
    private fun initVideoProgressView() {
        progressSeekBar = findViewById(R.id.video_view_seekbar)
        if (!isLive) {
            progressSeekBar.setOnProgressChangedListener(object :
                XhsProgressBar.OnProgressChangedListener {
                override fun onProgressChanged(
                    signSeekBar: XhsProgressBar,
                    progress: Int,
                    progressFloat: Float,
                    fromUser: Boolean
                ) {
                }

                override fun getProgressOnActionUp(
                    signSeekBar: XhsProgressBar, progress: Int, progressFloat: Float
                ) {
                    val percentage = progress / 1000.0
                    videoPlayerView.seekTo((percentage * videoPlayerView.duration).toInt())
                    progressSeekBar.setProgress(progressFloat)
                }

                override fun getProgressOnFinally(
                    signSeekBar: XhsProgressBar,
                    progress: Int,
                    progressFloat: Float,
                    fromUser: Boolean
                ) {
                }
            })
        }
        // update the seekbar
        val handler = Handler(Looper.getMainLooper())
        var position: Int
        var duration: Int
        val updateSeekBar = object : Runnable {
            override fun run() {
                position = videoPlayerView.currentPosition
                duration = videoPlayerView.duration
                if (duration > 0) {
                    progressSeekBar.setProgress((1000L * position / duration).toFloat())
                }
                videoPlayerView.mediaPlayer?.let { updateDebugInfo(it) }
                handler.postDelayed(this, 200)
            }
        }
        handler.post(updateSeekBar)
    }

    /** init the media player controller and logic */
    private fun initVideoControllerView() {
        loadingView = findViewById(R.id.loading_layout)
        loadingView.visibility = if (isShowLoading) View.VISIBLE else View.GONE
        videoTopActionLayout = findViewById(R.id.player_top_action_layout)
        backView = findViewById(R.id.top_action_back_view)
        backView.setOnClickListener {
            if (mIsLandscape) {
                changeScreenOrientation()
            } else {
                // try show pop window and play
                tryShowSmallWindow()
                finish()
            }
        }
        moreView = findViewById(R.id.top_action_more_view)
        moreView.setOnClickListener {
            showOrHideActionBar()
            drawerLayout.openDrawer(drawerContentLayout)
        }
        shareView = findViewById(R.id.top_action_share_view)
        likeView = findViewById(R.id.top_action_like_view)
        likeView.setOnClickListener {
            likeView.setBackgroundResource(R.drawable.icon_liked)
        }
        videoBottomActionLayout = findViewById(R.id.player_bottom_action_layout)
        lightLayout = findViewById(R.id.light_layout)
        lightSeekBar = findViewById(R.id.battery_seek_bar)
        voiceLayout = findViewById(R.id.voice_layout)
        voiceSeekBar = findViewById(R.id.voice_seek_bar)
        speedLayout = findViewById(R.id.seep_layout)
        playPauseButton = findViewById(R.id.play_pause_button)
        playNextButton = findViewById(R.id.play_next_button)
        playSectionButton = findViewById(R.id.video_section_view)
        playHVSwitchBtn = findViewById(R.id.video_screen_switch_btn)
        playHVSwitchBtn.setOnClickListener {
            changeScreenOrientation()
        }
        playPauseButton.setOnClickListener {
            pauseOrResume()
        }
        playNextButton.setOnClickListener {
            playNext()
        }
        playSectionButton.setOnClickListener {
            showOrHideActionBar()
            drawerLayout.openDrawer(drawerContentLayout)
        }
        videoPlayerView = findViewById(R.id.single_video_view)
        // 1.0 set the media player necessary listener
        videoPlayerView.setOnPreparedListener { mp, event -> handleVideoPreparedEvent(mp) }
        videoPlayerView.setOnInfoListener { mp, what, extra, event ->
            handleVideoInfoEvent(what, extra)
        }
        videoPlayerView.setOnSeekCompleteListenerListener { handleVideoSeekCompleteEvent() }
        videoPlayerView.setOnStartPlayListener { _, isPlaying, url ->
            handleVideoStartPlayEvent(isPlaying, url)
        }
        videoPlayerView.setOnBufferingUpdateListener { _, _ -> handleVideoBufferingUpdateEvent() }
        videoPlayerView.setOnVideoSizeChangedListener { _, width, height, sar_num, sar_den ->
            handleVideoSizeChangedEvent(width, height)
        }
        videoPlayerView.setOnCompletionListener {
            playPauseButton.setBackgroundResource(R.drawable.icon_play)
            handleVideoPlayCompleteEvent()
        }
        videoPlayerView.setOnErrorListener { _, what, extra -> handleVideoErrorEvent(what, extra) }
        // 1.1 set the video url and wait play
        videoPlayerView.setVideoPath(getCurrentPlayUrl(), isLive)
        if (isLive) {
            playPauseButton.visibility = View.GONE
        }
    }

    /** init the video debug info view */
    private fun initDebugInfoView() {
        debugContentLayout = findViewById(R.id.debug_info_layout)
        debugInfoLayout = findViewById(R.id.debug_content_view)
        debugInfoHideView = findViewById(R.id.debug_show_or_hide_btn)
        debugInfoHideView.setOnClickListener {
            debugInfoLayout.visibility = if (debugInfoLayout.isVisible) View.GONE else View.VISIBLE
        }
        debugInfoNameView = findViewById(R.id.player_debug_info_name_view)
        debugInfoValueView = findViewById(R.id.player_debug_info_value_view)
        debugContentLayout.visibility =
            if (settingConfig.enableDebugInfoView) View.VISIBLE else View.GONE
    }

    private fun getCurrentPlayUrl(): String {
        val index = playIndex % videoUrls.size
        return videoUrls[index]
    }

    /** init the side layout */
    private fun initDrawerLayout() {
        drawerLayout = findViewById(R.id.player_drawer_layout)
        drawerLayout.setScrimColor(Color.TRANSPARENT)
        drawerContentLayout = findViewById(R.id.player_drawer_content_layout)
        playerContentRV = findViewById(R.id.player_content_recyclerview)
        playerContentRV.layoutManager = LinearLayoutManager(this)
        playerContentRV.addItemDecoration(XhsSectionSpaceItemDecoration(Utils.dp2px(5)))
        sectionAdapter = XhsSectionAdapter(this, videoUrls, getCurrentPlayUrl())
        playerContentRV.adapter = sectionAdapter
        playerContentRV.itemAnimator = null
        playerContentRV.setHasFixedSize(true)
        playerContentRV.addOnItemTouchListener(object : RecyclerView.SimpleOnItemTouchListener() {})
        sectionAdapter.setOnSectionItemClickListener(this)
    }

    override fun onClick(holder: RecyclerView.ViewHolder?, position: Int, model: String?) {
        drawerLayout.closeDrawer(drawerContentLayout)
        playPosition(position)
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun initVideoViewScale() {
        videoFrameLayout = findViewById(R.id.video_frame_layout)
        val scaleGestureDetector = ScaleGestureDetector(
            this,
            object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
                override fun onScale(detector: ScaleGestureDetector): Boolean {
                    oldScaleFactor = scaleFactor
                    scaleFactor *= detector.scaleFactor
                    scaleFactor = max(1.0f, min(scaleFactor, 5.0f))
                    videoPlayerView.setViewScale(scaleFactor)
                    return true
                }
            })
        val gestureDetector =
            GestureDetector(this, object : GestureDetector.SimpleOnGestureListener() {
                override fun onDown(e: MotionEvent): Boolean {
                    downX = e.x
                    downY = e.y
                    firstTouch = true
                    changePosition = false
                    changeBrightness = false
                    changeVolume = false
                    return true
                }

                override fun onScroll(
                    e1: MotionEvent?, e2: MotionEvent, distanceX: Float, distanceY: Float
                ): Boolean {
                    if (scaleFactor != 1.0f) {
                        startX -= distanceX
                        startY -= distanceY
                        maxWidthTranslation = videoPlayerView.width * (scaleFactor - 1.0F) / 2
                        startX = max(-maxWidthTranslation, min(maxWidthTranslation, startX))
                        startY =
                            max(-maxWidthTranslation * 2, min(maxWidthTranslation * 2, startY))
                        videoPlayerView.setViewTranslation(startX, startY)
                        return true
                    }
                    if (e1?.pointerCount == 1 && e2.pointerCount == 1) {
                        handleChangeVolumeOrBrightness(downX - e2.x, downY - e2.y)
                    }
                    return true
                }

                override fun onLongPress(e: MotionEvent) {
                    handleVideoSpeedPlay(settingConfig.getVideoPlayerSpeed, true)
                }

                override fun onSingleTapConfirmed(e: MotionEvent): Boolean {
                    showOrHideActionBar()
                    return true
                }
            })
        videoFrameLayout.setOnTouchListener { _, event ->
            scaleGestureDetector.onTouchEvent(event)
            gestureDetector.onTouchEvent(event)
            if (event.action == MotionEvent.ACTION_UP) {
                handleVideoSpeedPlay(1.0f, false)
                voiceLayout.visibility = View.GONE
                lightLayout.visibility = View.GONE
                downX = 0f
                downY = 0f
                firstTouch = false
                changePosition = false
                changeBrightness = false
                changeVolume = false
            }
            true
        }
    }

    private fun handleChangeVolumeOrBrightness(deltaX: Float, deltaY: Float) {
        if (firstTouch) {
            when {
                Utils.beyondHalfScreenWidth(this@XhsPlayerActivity, downX) -> {
                    changeVolume = true
                }

                else -> {
                    changeBrightness = true
                }
            }
            firstTouch = false
        }
        when {
            changeBrightness -> {
                Utils.slideToChangeBrightness(this@XhsPlayerActivity, deltaY) { progress, max ->
                    showLightLayout(progress!!, max!!)
                }
            }

            changeVolume -> {
                Utils.slideToChangeVolume(this@XhsPlayerActivity, deltaY) { progress, max ->
                    showVoiceLayout(progress!!, max!!)
                }
            }
        }
    }

    /** play next video */
    private fun playNext() {
        playIndex += 1
        if (playIndex > (videoUrls.size - 1)) {
            playIndex = 0
        }
        videoPlayerView.switchVideo(getCurrentPlayUrl())
    }

    /** play the special index video */
    private fun playPosition(index: Int) {
        playIndex = index
        videoPlayerView.switchVideo(getCurrentPlayUrl())
    }

    private fun showOrHideActionBar() {
        if (videoBottomActionLayout.isVisible) {
            videoBottomActionLayout.visibility = View.GONE
            videoTopActionLayout.visibility = View.GONE
            debugContentLayout.visibility = View.GONE
        } else {
            videoBottomActionLayout.visibility = View.VISIBLE
            videoTopActionLayout.visibility = View.VISIBLE
            // 由配置控制是否可见
            debugContentLayout.visibility =
                if (settingConfig.enableDebugInfoView) View.VISIBLE else View.GONE
        }
    }

    private fun showVoiceLayout(progress: Int, max: Int) {
        voiceLayout.visibility = View.VISIBLE
        voiceSeekBar.setProgress(progress.toFloat())
    }

    private fun showLightLayout(progress: Int, max: Int) {
        lightLayout.visibility = View.VISIBLE
        lightSeekBar.setProgress(progress.toFloat())
    }

    /** update the debug info */
    private fun updateDebugInfo(mediaPlayer: IMediaPlayer) {
        val debugInfo =
            Utils.getVideoDebugInfo(this, mediaPlayer, settingConfig.enableSurfaceView)
        debugInfoNameView.text = debugInfo.first
        debugInfoValueView.text = debugInfo.second
    }

    /**  handle the media player prepared event */
    private fun handleVideoPreparedEvent(mediaPlayer: IMediaPlayer) {
        Log.d(TAG, "the media player is ready, can start.")
    }

    /** handle video speed play: support 0.75，1.0，1.25，1.5，2.0 */
    private fun handleVideoSpeedPlay(speed: Float, isShow: Boolean) {
        if (!videoPlayerView.isPlaying || isLive) {
            return
        }
        videoPlayerView.setSpeed(speed)
        if (isShow) {
            speedLayout.visibility = View.VISIBLE
            Utils.vibrate(this@XhsPlayerActivity)
        } else {
            speedLayout.visibility = View.GONE
        }
    }

    /** handle video play complete event */
    private fun handleVideoPlayCompleteEvent() {
        videoPlayerView.seekTo(0)
        handleVideoSpeedPlay(1.0f, false)
        voiceLayout.visibility = View.GONE
        lightLayout.visibility = View.GONE
        speedLayout.visibility = View.GONE
        val autoPlayNext =
            settingConfig.enableVideoAutoPlayerNext && settingConfig.enableVideoAutoLoop
        val autoLoop = settingConfig.enableVideoAutoLoop
        when {
            autoPlayNext -> {
                playNext()
            }

            autoLoop -> {
                videoPlayerView.start()
            }
        }
    }

    /** handle video size change event */
    private fun handleVideoSizeChangedEvent(width: Int, height: Int) {
        Log.d(TAG, "the new video width is:$width, height is: $height")
    }


    private fun handleVideoErrorEvent(what: Int, extra: Int): Boolean {
        Log.d(TAG, "the video errorCode is:$what, errorMsg is: $extra")
        return false
    }

    /** handle video seek complete event */
    private fun handleVideoSeekCompleteEvent() {
        Log.d(TAG, "the current video progress is: ${videoPlayerView.currentPosition}")
    }

    /** handle media player start event */
    private fun handleVideoStartPlayEvent(isPlaying: Boolean, url: String) {
        if (isPlaying) {
            playPauseButton.setBackgroundResource(R.drawable.icon_pause)
            sectionAdapter.updateCurrentPlayUrl(url)
        } else {
            playPauseButton.setBackgroundResource(R.drawable.icon_play)
        }
        // 开播后，隐藏loading布局
        loadingView.visibility = View.GONE
    }

    private fun handleVideoBufferingUpdateEvent() {}

    /** handle media player from prepare to play all event */
    private fun handleVideoInfoEvent(what: Int, extra: Int): Boolean {
        when (what) {
            // video stream connection success event
            IMediaPlayer.MEDIA_INFO_OPEN_INPUT -> {
                // videoTrackManager.onDidConnect(event?.tcpCount ?: 0L, currTime)
            }
            // video format check finished event
            IMediaPlayer.MEDIA_INFO_FIND_STREAM_INFO -> {
                // videoTrackManager.onStreamInfoFound(currTime)
            }
            // media codec init event
            IMediaPlayer.MEDIA_INFO_COMPONENT_OPEN -> {
                // videoTrackManager.onComponentOpen(currTime)
            }

            // first video package input the codec event
            IMediaPlayer.MEDIA_INFO_VIDEO_FIRST_PACKET_IN_DECODER -> {
                // videoTrackManager.onPlayerFirstPacketInDecoder(currTime)
            }
            // video first frame decode finished event
            IMediaPlayer.MEDIA_INFO_VIDEO_DECODED_START -> {
                // videoTrackManager.onPlayerDecodedStart(currTime)
            }
            // video first frame render success event
            IMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START -> {
                // videoTrackManager.onPlayerRenderStart(extra == 0, currTime)
            }
            // video start playing event
            IMediaPlayer.MEDIA_INFO_VIDEO_START_ON_PLAYING -> {
                // videoTrackManager.onVideoStartOnPlaying(currTime)
            }
        }
        return true
    }

    /** try show small window */
    private fun tryShowSmallWindow() {
        if (XhsFloatingService.serviceHasStarted) {
            val intent = Intent(XhsFloatingService.ACTION_SHOW_VIDEO).apply {
                putExtra(XhsFloatingService.INTENT_KEY_VIDEO_URLS, getCurrentPlayUrl())
                putExtra(XhsFloatingService.INTENT_KEY_SHOW_LOADING, isShowLoading)
            }
            LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
            return
        }
        if (XhsFloatingWindowHelper.checkHasFloatingPermission(this)) {
            val intent = Intent(this, XhsFloatingService::class.java).apply {
                putExtra(XhsFloatingService.INTENT_KEY_VIDEO_URLS, getCurrentPlayUrl())
                putExtra(XhsFloatingService.INTENT_KEY_SHOW_LOADING, isShowLoading)
            }
            startService(intent)
            return
        }
    }

    private fun tryHideSmallWindow() {
        if (XhsFloatingService.serviceHasStarted) {
            LocalBroadcastManager.getInstance(this)
                .sendBroadcast(Intent(XhsFloatingService.ACTION_HIDE_VIDEO))
        }
    }
}