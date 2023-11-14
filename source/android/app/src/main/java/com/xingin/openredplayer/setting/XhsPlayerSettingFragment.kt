package com.xingin.openredplayer.setting

import android.os.Bundle
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import com.xingin.openredplayer.R
import com.xingin.openredplayer.floating.XhsFloatingWindowHelper

class SettingsFragment : PreferenceFragmentCompat() {
    override fun onCreatePreferences(bundle: Bundle?, s: String?) {
        setPreferencesFromResource(R.xml.settings, s)
        val windowPreference: Preference? =
            findPreference(getString(R.string.pref_key_video_window))
        windowPreference?.onPreferenceClickListener =
            Preference.OnPreferenceClickListener {
                XhsFloatingWindowHelper.startSettingPage(this@SettingsFragment.requireContext())
                return@OnPreferenceClickListener true
            }
    }

    companion object {
        fun newInstance(): SettingsFragment {
            return SettingsFragment()
        }
    }
}