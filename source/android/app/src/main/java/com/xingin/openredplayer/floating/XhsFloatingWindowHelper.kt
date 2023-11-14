package com.xingin.openredplayer.floating

import android.animation.ValueAnimator
import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.graphics.PixelFormat
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.View.OnTouchListener
import android.view.WindowManager
import android.view.animation.DecelerateInterpolator
import android.widget.LinearLayout
import com.xingin.openredplayer.XhsMediaApplication
import com.xingin.openredplayer.utils.Utils.dp2px
import com.xingin.openredplayer.utils.Utils.getScreenRealHeight
import com.xingin.openredplayer.utils.Utils.getScreenRealWidth
import com.xingin.openredplayer.utils.Utils.vibrate

/** the small window controller: show, hide, drag, auto stick slide... */
class XhsFloatingWindowHelper(context: Context) {
    private var windowManager: WindowManager?
    private var currentView: View? = null
    private var currentLayoutParams: WindowManager.LayoutParams? = null
    private var stickAnimator: ValueAnimator? = null

    init {
        windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
    }

    /** create the small window layout params */
    private fun createLayoutParams(): WindowManager.LayoutParams {
        return WindowManager.LayoutParams().apply {
            type = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
            } else {
                WindowManager.LayoutParams.TYPE_PHONE
            }
            format = PixelFormat.RGBA_8888
            gravity = Gravity.END or Gravity.BOTTOM
            flags =
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL or WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
            width = LinearLayout.LayoutParams.WRAP_CONTENT
            height = LinearLayout.LayoutParams.WRAP_CONTENT
        }
    }

    /** add and show the small window */
    @SuppressLint("ClickableViewAccessibility")
    fun addView(view: View, canMove: Boolean) {
        currentView = view
        currentLayoutParams = createLayoutParams()
        windowManager?.addView(currentView, currentLayoutParams)
        if (canMove) {
            currentView?.setOnTouchListener(OnFloatingTouchListener())
        }
    }

    /** remove and hide the small window */
    fun removeView() {
        windowManager?.let { windowManager ->
            currentView?.let { windowManager.removeView(it) }
        }
        currentView = null
        currentLayoutParams = null
        stickAnimator?.removeAllListeners()
        stickAnimator?.cancel()
        stickAnimator = null
    }

    /** release all small window resource */
    fun clear() {
        windowManager?.let { windowManager ->
            currentView?.let { windowManager.removeView(it) }
        }
        currentView = null
        currentLayoutParams = null
        windowManager = null
        // release anim
        stickAnimator?.removeAllListeners()
        stickAnimator?.cancel()
        stickAnimator = null
    }

    /** handle the small window drag event */
    inner class OnFloatingTouchListener : OnTouchListener {
        @SuppressLint("ClickableViewAccessibility")
        override fun onTouch(view: View, event: MotionEvent): Boolean {
            when (event.action) {
                MotionEvent.ACTION_DOWN -> vibrate(XhsMediaApplication.instance)
                MotionEvent.ACTION_MOVE -> {
                    val movedX = event.rawX.toInt()
                    val movedY = event.rawY.toInt()
                    currentLayoutParams?.x = calculateLayoutParamsX(movedX)
                    currentLayoutParams?.y = calculateLayoutParamsY(movedY)
                    // drag the window with the move event
                    windowManager?.updateViewLayout(view, currentLayoutParams)
                }

                MotionEvent.ACTION_UP -> {
                    val start = (currentLayoutParams?.x ?: 0)
                    val end = calculateFinalEndPosition(start = start)
                    // stick the small window to screen slide
                    stickToSide(view, start = start, end = end)
                }

                else -> {}
            }
            return view.onTouchEvent(event)
        }

        /** calculate the x-coordinate of the small window based on the sliding position: which is opposite to the common understanding. */
        private fun calculateLayoutParamsX(startX: Int): Int {
            return getScreenRealWidth() - startX - getWindowWidth() / 2
        }

        /** calculate the y-coordinate of the small window based on the sliding position: which is opposite to the common understanding. */
        private fun calculateLayoutParamsY(startY: Int): Int {
            return getScreenRealHeight() - startY - getWindowHeight() / 2
        }

        /** calculate whether to stick to the left or right after releasing */
        private fun calculateFinalEndPosition(start: Int): Int {
            return if (beyondHalfScreenWidth(start)) {
                // stick to the right
                getLayoutParamsMaxX()
            } else {
                // stick to the left
                getLayoutParamsMinX()
            }
        }

        /** animation of automatically sticking to the edge after releasing */
        private fun stickToSide(view: View, start: Int, end: Int) {
            stickAnimator = ValueAnimator.ofInt(start, end)
                .setDuration(WINDOW_ANIM_DURATION)
                .apply {
                    interpolator = DecelerateInterpolator()
                    addUpdateListener { animation: ValueAnimator ->
                        currentLayoutParams?.x = animation.animatedValue as Int
                        windowManager?.updateViewLayout(view, currentLayoutParams)
                    }
                    start()
                }
        }

        /** get the coordinate of the leftmost point of the small window: opposite to the common understanding, it is the screen width minus the view width. */
        private fun getLayoutParamsMinX(): Int {
            return getScreenRealWidth() - getWindowWidth()
        }

        /** get the coordinate of the rightmost point of the small window: opposite to the common understanding, 0 is the rightmost point. */
        private fun getLayoutParamsMaxX(): Int {
            return 0
        }

        /** whether the current position exceeds half of the screen width: start is the current starting coordinate of the small window. */
        private fun beyondHalfScreenWidth(start: Int): Boolean {
            return getScreenRealWidth() - start - getWindowWidth() / 2 >= getScreenRealWidth() / 2
        }

        /** get the width of the small window: used for various position calculations, and the width can be dynamically set based on the video aspect ratio. */
        private fun getWindowWidth(): Int {
            return dp2px(120)
        }

        /** get the height of the small window: used for various position calculations, and the height can be dynamically set based on the video aspect ratio. */
        private fun getWindowHeight(): Int {
            return dp2px(215)
        }
    }

    companion object {
        const val WINDOW_ANIM_DURATION = 300L
        private const val PACKAGE_NAME = "package:"

        fun checkHasFloatingPermission(context: Context?): Boolean {
            return if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
                true
            } else Settings.canDrawOverlays(context)
        }

        fun startSettingPage(context: Context) {
            val intent = Intent(
                Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse(PACKAGE_NAME + context.packageName)
            ).apply {
                flags = Intent.FLAG_ACTIVITY_NEW_TASK
            }
            context.startActivity(intent)
        }
    }
}