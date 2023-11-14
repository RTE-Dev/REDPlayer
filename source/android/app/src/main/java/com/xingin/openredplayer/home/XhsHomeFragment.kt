package com.xingin.openredplayer.home

import android.Manifest
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.kelin.photoselector.PhotoSelector
import com.tbruyelle.rxpermissions3.RxPermissions
import com.xingin.openredplayer.R
import com.xingin.openredplayer.feed.XhsFeedActivity
import com.xingin.openredplayer.feed.model.VIDEO_ONE
import com.xingin.openredplayer.feed.model.covertToVideoUrls
import com.xingin.openredplayer.floating.XhsFloatingService
import com.xingin.openredplayer.floating.XhsFloatingWindowHelper
import com.xingin.openredplayer.input.XhsInputActivity
import com.xingin.openredplayer.player.XhsPlayerActivity
import io.reactivex.rxjava3.disposables.Disposable
import java.io.Serializable

class XhsHomeFragment : Fragment() {
    private lateinit var rootView: View
    private lateinit var itemUrlView: View
    private lateinit var itemAlbumView: View
    private lateinit var itemFeedView: View
    private lateinit var itemWindowView: View

    private lateinit var rxPermissions: RxPermissions
    private var permissionDisposable: Disposable? = null

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        initView()
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        rootView = inflater.inflate(R.layout.fragment_xhs_home_layout, null, false)
        return rootView
    }

    override fun onDestroy() {
        super.onDestroy()
        permissionDisposable?.dispose()
    }

    private fun initView() {
        itemUrlView = rootView.findViewById(R.id.layout_play_url)
        itemUrlView.setOnClickListener { openInputPage() }
        itemAlbumView = rootView.findViewById(R.id.layout_local_album)
        itemAlbumView.setOnClickListener { openAlbumPage() }
        itemFeedView = rootView.findViewById(R.id.layout_double_column)
        itemFeedView.setOnClickListener { openFeedPage() }
        itemWindowView = rootView.findViewById(R.id.layout_window)
        itemWindowView.setOnClickListener { openWindow() }
        // init permission
        rxPermissions = RxPermissions(this)
    }

    private fun openInputPage() {
        val intent = Intent(this.activity, XhsInputActivity::class.java)
        startActivity(intent)
    }

    private fun openAlbumPage() {
        permissionDisposable = rxPermissions.request(
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.ACCESS_NETWORK_STATE,
        ).subscribe { granted ->
            if (!granted) {
                return@subscribe
            }
            // open the album
            PhotoSelector.openVideoSelector(
                this,
                100,
                PhotoSelector.ID_REPEATABLE
            ) { photos ->
                if (photos?.isNotEmpty() == true) {
                    val intent = Intent(this.activity, XhsPlayerActivity::class.java)
                    intent.putExtra(
                        XhsPlayerActivity.INTENT_KEY_URLS,
                        covertToVideoUrls(photos) as Serializable
                    )
                    startActivity(intent)
                }
            }
        }
    }

    private fun openFeedPage() {
        val intent = Intent(this.activity, XhsFeedActivity::class.java)
        startActivity(intent)
    }

    private fun openWindow() {
        val activity = this.activity ?: return
        // service has stared: play the video url
        if (XhsFloatingService.serviceHasStarted) {
            val intent = Intent(XhsFloatingService.ACTION_SHOW_VIDEO).apply {
                putExtra(XhsFloatingService.INTENT_KEY_VIDEO_URLS, VIDEO_ONE)
                putExtra(XhsFloatingService.INTENT_KEY_SHOW_LOADING, false)
            }
            LocalBroadcastManager.getInstance(activity).sendBroadcast(intent)
            return
        }
        // service is not start:has permission，start the service and then play the video url
        if (XhsFloatingWindowHelper.checkHasFloatingPermission(this.activity)) {
            val intent = Intent(activity, XhsFloatingService::class.java).apply {
                putExtra(XhsFloatingService.INTENT_KEY_VIDEO_URLS, VIDEO_ONE)
                putExtra(XhsFloatingService.INTENT_KEY_SHOW_LOADING, false)
            }
            activity.startService(intent)
            return
        }
        // open the setting page and request the alert permission
        XhsFloatingWindowHelper.startSettingPage(activity)
    }

    companion object {
        fun newInstance(): XhsHomeFragment {
            return XhsHomeFragment()
        }
    }
}