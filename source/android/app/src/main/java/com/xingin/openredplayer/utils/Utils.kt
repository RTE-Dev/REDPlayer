package com.xingin.openredplayer.utils

import android.app.Activity
import android.content.Context
import android.content.res.Resources
import android.graphics.Point
import android.media.AudioManager
import android.os.Build
import android.os.Vibrator
import android.util.Base64
import android.util.TypedValue
import android.view.WindowManager
import com.google.gson.JsonParser
import com.google.gson.JsonSyntaxException
import com.xingin.openredplayer.XhsMediaApplication
import com.xingin.openredplayer.feed.model.XhsVisibleItemInfo
import com.xingin.openredplayercore.core.api.IMediaPlayer
import com.xingin.openredplayercore.core.api.IMediaPlayer.MEDIA_PLAYER_ANDROID
import com.xingin.openredplayercore.core.api.IMediaPlayer.MEDIA_PLAYER_RED
import java.util.Locale
import java.util.Random
import kotlin.math.abs

object Utils {

    /**
     * 将videoUrl解码出来
     */
    fun decodeBase64(videoURLEncoder: String): String {
        val bytes = Base64.decode(videoURLEncoder, 0)
        return String(bytes)
    }

    @JvmStatic
    fun isValidJsonString(json: String?): Boolean {
        return try {
            JsonParser.parseString(json)
            true
        } catch (e: JsonSyntaxException) {
            false
        }
    }

    fun getElementRandom(list: List<XhsVisibleItemInfo>): XhsVisibleItemInfo? {
        val random = Random()
        val index = random.nextInt(list.size)
        return list[index]
    }

    fun getVideCacheDir(): String {
        val context = XhsMediaApplication.instance
        return context.externalCacheDir?.absolutePath
            ?: context.cacheDir.absolutePath
    }

    fun slideToChangeVolume(
        context: Context, deltaY: Float, callback: Function2<Int?, Int?, Unit>
    ) {
        val audioManager = context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
        val screenHeight = getScreenRealHeight(context)
        val nowVolume = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC)
        val streamMaxVolume = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC)
        val deltaV = deltaY / (screenHeight * 2) * streamMaxVolume
        var index = nowVolume + deltaV
        if (index > streamMaxVolume) index = streamMaxVolume.toFloat()
        if (index < 0) index = 0f
        audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, index.toInt(), 0)
        callback.invoke((index * 100).toInt(), streamMaxVolume * 100)
    }

    fun slideToChangeBrightness(
        context: Activity, deltaY: Float, callback: Function2<Int?, Int?, Unit>
    ) {
        val screenHeight = getScreenRealHeight(context)
        val window = context.window
        val attributes = window.attributes
        var brightness = context.window.attributes.screenBrightness
        if (brightness == -1.0f) brightness = 0.5f
        brightness += deltaY / (screenHeight * 4)
        if (brightness < 0) {
            brightness = 0f
        }
        if (brightness > 1.0f) brightness = 1.0f
        attributes.screenBrightness = brightness
        window.attributes = attributes
        callback.invoke((brightness * 1000).toInt(), 1000)
    }

    fun getScreenRealWidth(context: Context = XhsMediaApplication.instance): Int {
        val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            windowManager.currentWindowMetrics.bounds.width()
        } else {
            val point = Point()
            windowManager.defaultDisplay.getSize(point)
            point.x
        }
    }

    fun getScreenRealHeight(context: Context = XhsMediaApplication.instance): Int {
        val windowManager = context.getSystemService(Context.WINDOW_SERVICE) as WindowManager
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            windowManager.currentWindowMetrics.bounds.height()
        } else {
            val point = Point()
            windowManager.defaultDisplay.getSize(point)
            point.y
        }
    }

    fun beyondScreenOneThirdHeight(context: Context, distance: Int): Boolean {
        val oneThirdScreen = getScreenRealHeight(context) / 4
        return abs(distance) >= oneThirdScreen
    }

    fun beyondHalfScreenWidth(context: Context, downX: Float): Boolean {
        val halfScreen = getScreenRealWidth(context) / 2
        return downX > halfScreen
    }

    fun vibrate(context: Context) {
        val vibrator = context.getSystemService(Context.VIBRATOR_SERVICE) as Vibrator
        vibrator.vibrate(100)
    }

    private fun isSoftEncode(mediaPlayer: IMediaPlayer): Int {
        return when (mediaPlayer.videoDecoder) {
            IMediaPlayer.DECODER_AVCODEC -> 1
            IMediaPlayer.DECODER_MEDIACODEC -> 0
            else -> -1
        }
    }

    private fun getVideoCodecName(mediaPlayer: IMediaPlayer): String {
        return mediaPlayer.videoCodecInfo
    }

    private fun getVideoFileFps(mediaPlayer: IMediaPlayer): Int {
        var result = 0.0f
        return result.toInt()
    }

    private fun formatSpeed(bytes: Long, elapsedMilli: Long): String {
        if (elapsedMilli <= 0) {
            return "0 B/s"
        }
        if (bytes <= 0) {
            return "0 B/s"
        }
        val bytesPerSec: Float = bytes.toFloat() * 1000f / elapsedMilli
        return when {
            bytesPerSec >= 1000 * 1000 -> {
                String.format(Locale.US, "%.2f MB/s", bytesPerSec / 1000 / 1000)
            }

            bytesPerSec >= 1000 -> {
                String.format(Locale.US, "%.1f KB/s", bytesPerSec / 1000)
            }

            else -> {
                String.format(Locale.US, "%d B/s", bytesPerSec.toLong())
            }
        }
    }

    private fun formatDurationMilli(duration: Long): String {
        return when {
            duration >= 1000 -> {
                String.format(Locale.US, "%.2f sec", duration.toFloat() / 1000)
            }

            else -> {
                String.format(Locale.US, "%d msec", duration)
            }
        }
    }

    private fun formatSize(bytes: Long): String {
        return when {
            bytes >= 100 * 1000 -> {
                String.format(Locale.US, "%.2f MB", bytes.toFloat() / 1000 / 1000)
            }

            bytes >= 100 -> {
                String.format(Locale.US, "%.1f KB", bytes.toFloat() / 1000)
            }

            else -> {
                String.format(Locale.US, "%d B", bytes)
            }
        }
    }

    private fun getPlayerCoreType(mediaPlayer: IMediaPlayer): String {
        return when (mediaPlayer.playerCoreType) {
            MEDIA_PLAYER_RED -> "RedMediaPlayer"
            MEDIA_PLAYER_ANDROID -> "AndroidMediaPlayer"
            else -> "NONE"
        }
    }

    fun getVideoDebugInfo(
        activity: Activity, mediaPlayer: IMediaPlayer, enableSurfaceView: Boolean
    ): Pair<String, String> {
        val vedc = when (isSoftEncode(mediaPlayer)) {
            0 -> "MediaCodec"
            1 -> "avcodec"
            else -> "unknown"
        }
        val videoCodecName = mediaPlayer.videoCodecInfo
        val audioCodecName = mediaPlayer.audioCodecInfo
        val widthHeight =
            String.format(Locale.US, "%d * %d", mediaPlayer.videoWidth, mediaPlayer.videoHeight)
        val fps = String.format(
            Locale.US,
            "%d / %d / %d",
            mediaPlayer.videoDecodeFramesPerSecond.toInt(),
            mediaPlayer.videoOutputFramesPerSecond.toInt(),
            mediaPlayer.videoFileFps.toInt()
        )
        val bitRate = String.format(Locale.US, "%.2f kbs", mediaPlayer.bitRate / 1000f)
        val vCache = String.format(
            Locale.US,
            "%s / %s",
            formatDurationMilli(mediaPlayer.videoCachedSizeMs),
            formatSize(mediaPlayer.videoCachedSizeBytes)
        )
        val aCache = String.format(
            Locale.US,
            "%s / %s",
            formatDurationMilli(mediaPlayer.audioCachedSizeMs),
            formatSize(mediaPlayer.audioCachedSizeBytes)
        )
        val seekLoadDuration = mediaPlayer.seekCostTimeMs
        val surfaceType = if (enableSurfaceView) {
            "SurfaceView"
        } else {
            "TextureView"
        }
        val playerCoreType = getPlayerCoreType(mediaPlayer)
        val finalUrl = mediaPlayer.playUrl
        val debugInfoName =
            "screen" + "\ndpi" + "\nvdec" + "\nvideoCodec" + "\naudioCodec" + "\nwidthHeight" + "\nfps" + "\nbitRate" + "\nv_cache" + "\na_cache" + "\nseek_cost" + "\nsurfaceType" + "\ncoreType" + "\nurl"
        val debugInfoValue =
            "${getScreenRealWidth(activity)} * ${getScreenRealHeight(activity)}" + "\n${activity.resources.displayMetrics.xdpi} * ${activity.resources.displayMetrics.ydpi}" + "\n${vedc}" + "\n${videoCodecName}" + "\n${audioCodecName}" + "\n${widthHeight}" + "\n${fps}" + "\n${bitRate}" + "\n${vCache}" + "\n${aCache}" + "\n${seekLoadDuration}ms" + "\n${surfaceType}" + "\n${playerCoreType}" + "\n${finalUrl}"
        return Pair(debugInfoName, debugInfoValue)
    }

    fun dp2px(dp: Int): Int {
        return TypedValue.applyDimension(
            TypedValue.COMPLEX_UNIT_DIP, dp.toFloat(),
            Resources.getSystem().displayMetrics
        ).toInt()
    }

    fun sp2px(sp: Int): Int {
        return TypedValue.applyDimension(
            TypedValue.COMPLEX_UNIT_SP, sp.toFloat(),
            Resources.getSystem().displayMetrics
        ).toInt()
    }
}