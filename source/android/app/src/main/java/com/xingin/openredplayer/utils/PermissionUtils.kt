package com.xingin.openredplayer.utils

import android.app.Activity
import android.content.pm.PackageManager
import android.widget.Toast
import androidx.core.app.ActivityCompat

object PermissionUtils {
    fun request(activity: Activity, permissions: Array<String>, code: Int) {
        ActivityCompat.requestPermissions(activity, permissions, code)
    }

    fun hasPermission(activity: Activity, permissions: Array<String>): Boolean {
        permissions.forEach {
            if (ActivityCompat.checkSelfPermission(activity, it) != PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(activity, "Permission denied: $it", Toast.LENGTH_SHORT).show()
                return false
            }
        }
        return true
    }
}