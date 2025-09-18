package com.xingin.openredplayer.home

import android.annotation.SuppressLint
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.viewpager.widget.ViewPager
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayout.OnTabSelectedListener
import com.xingin.openredplayer.R
import com.xingin.openredplayer.setting.SettingsFragment


class XhsHomeActivity : AppCompatActivity() {
    private lateinit var tabLayout: TabLayout
    private lateinit var viewPager: ViewPager
    private var homeFragment: XhsHomeFragment? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_xhs_home_page_layout)
        initViewPager()
        initTabLayout()
    }

    private fun initViewPager() {
        viewPager = findViewById(R.id.view_pager)
        val fragments: MutableList<Fragment> = ArrayList()
        val homeFragment = XhsHomeFragment.newInstance()
        this.homeFragment = homeFragment
        fragments.add(homeFragment)
        fragments.add(SettingsFragment.newInstance())
        val adapter = XhsHomePagerAdapter(supportFragmentManager, fragments)
        viewPager.adapter = adapter
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        homeFragment?.onActivityRequestPermissionsResult(requestCode)
    }

    @SuppressLint("UseCompatLoadingForDrawables")
    private fun initTabLayout() {
        tabLayout = findViewById(R.id.tab_layout)
        tabLayout.setupWithViewPager(viewPager)
        tabLayout.getTabAt(0)?.run {
            text = getString(R.string.home)
            icon = getDrawable(R.drawable.icon_home_selected)
            view.isLongClickable = false
        }

        tabLayout.getTabAt(1)?.run {
            text = getString(R.string.setting)
            icon = getDrawable(R.drawable.icon_setting)
            view.isLongClickable = false
        }
        tabLayout.addOnTabSelectedListener(object : OnTabSelectedListener {
            override fun onTabSelected(tab: TabLayout.Tab) {
                viewPager.currentItem = tab.position
                when (tab.position) {
                    0 -> {
                        tab.icon = getDrawable(R.drawable.icon_home_selected)
                    }

                    1 -> {
                        tab.icon = getDrawable(R.drawable.icon_setting_selected)
                    }
                }
            }

            override fun onTabUnselected(tab: TabLayout.Tab) {
                when (tab.position) {
                    0 -> {
                        tab.icon = getDrawable(R.drawable.icon_home)
                    }

                    1 -> {
                        tab.icon = getDrawable(R.drawable.icon_setting)
                    }
                }
            }

            override fun onTabReselected(tab: TabLayout.Tab) {

            }
        })
    }
}