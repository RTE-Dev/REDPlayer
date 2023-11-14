package com.xingin.openredplayer.player.sensor

import android.content.pm.ActivityInfo
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener

class XhsOrientationListener(private val mListener: OnOrientationChangeListener?) :
    SensorEventListener {
    private var mOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT

    override fun onSensorChanged(event: SensorEvent) {
        if (Sensor.TYPE_ACCELEROMETER != event.sensor.type) {
            return
        }
        val values = event.values
        val x = values[0]
        val y = values[1]
        val newOrientation: Int = if (x >= 4.5 && y < 4.5 && y >= -4.5) {
            ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        } else {
            ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE
        }
        if (mOrientation != newOrientation) {
            mListener?.orientationChanged(newOrientation)
            mOrientation = newOrientation
        }
    }

    override fun onAccuracyChanged(sensor: Sensor, accuracy: Int) {}

    interface OnOrientationChangeListener {
        fun orientationChanged(newOrientation: Int)
    }
}