package com.xingin.openredplayer.feed

import android.Manifest
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.ImageView
import androidx.annotation.MainThread
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.DefaultItemAnimator
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.StaggeredGridLayoutManager
import com.xingin.openredplayer.R
import com.xingin.openredplayer.feed.model.XhsFeedModel
import com.xingin.openredplayer.feed.model.XhsVisibleItemInfo
import com.xingin.openredplayer.feed.model.generateCacheRequest
import com.xingin.openredplayer.feed.model.getHomeFeedData
import com.xingin.openredplayer.feed.model.getVisibleItemsInfo
import com.xingin.openredplayer.player.XhsPlayerActivity
import com.xingin.openredplayer.player.XhsPlayerActivity.Companion.INTENT_KEY_SHOW_LOADING
import com.xingin.openredplayer.player.XhsPlayerActivity.Companion.INTENT_KEY_URLS
import com.xingin.openredplayer.setting.XhsPlayerSettings
import com.xingin.openredplayer.utils.PermissionUtils
import com.xingin.openredplayer.utils.Utils
import com.xingin.openredpreload.api.IMediaPreload
import com.xingin.openredpreload.api.PreloadCacheListener
import com.xingin.openredpreload.impl.RedMediaPreload
import com.xingin.openredpreload.impl.model.VideoCacheRequest
import io.reactivex.rxjava3.android.schedulers.AndroidSchedulers
import io.reactivex.rxjava3.disposables.Disposable
import java.io.Serializable

class XhsFeedActivity : AppCompatActivity(), XhsFeedAdapter.OnItemClickListener {
    companion object {
        private const val TAG = "XhsFeedActivity"
    }

    /** UI */
    private lateinit var backView: ImageView
    private lateinit var feedAdapter: XhsFeedAdapter
    private lateinit var feedRecyclerView: RecyclerView
    private lateinit var feedLayoutManager: StaggeredGridLayoutManager
    private val currentPlayerMap = hashMapOf<Int, XhsFeedViewHolder>()
    private var startScrollY = 0
    private var endScrollY = 0
    private var rvScrollDistance = 0

    /** Config */
    private lateinit var settingConfig: XhsPlayerSettings

    /** preload */
    private lateinit var mediaPreload: IMediaPreload
    private var preloadDisposable: Disposable? = null

    private var permissionDisposable: Disposable? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_xhs_home_feed_layout)
        settingConfig = XhsPlayerSettings(this)
        initView()
        initPreload()
        initPermission()
    }

    override fun onDestroy() {
        super.onDestroy()
        mediaPreload.release()
        preloadDisposable?.dispose()
        permissionDisposable?.dispose()
        clearAndStopOtherPlayer()
    }

    /** request storage permission：when grant the permission，start preload the video stream data */
    private val PERMISSION_CODE = 1001
    private val PERMISSIONS = arrayOf(
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE,
        Manifest.permission.ACCESS_NETWORK_STATE,
    )
    private fun initPermission() {
        if (PermissionUtils.hasPermission(this, PERMISSIONS)) {
            onPermissionGranted()
        } else {
            PermissionUtils.request(this, PERMISSIONS, PERMISSION_CODE)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PERMISSION_CODE && PermissionUtils.hasPermission(this, PERMISSIONS)) {
            onPermissionGranted()
        }
    }

    private fun onPermissionGranted() {
        feedRecyclerView.postDelayed({
            tryPreCacheVideoIfNeed(feedRecyclerView, "initPreload")
        }, 1000)
    }

    private fun initView() {
        backView = findViewById(R.id.top_action_back_view)
        backView.setOnClickListener {
            finish()
        }
        feedRecyclerView = findViewById(R.id.appbar_recyclerview)
        feedLayoutManager = StaggeredGridLayoutManager(2, StaggeredGridLayoutManager.VERTICAL)
        feedRecyclerView.layoutManager = feedLayoutManager
        feedRecyclerView.addItemDecoration(XhsItemDecoration(2, 10, includeEdge = false))
        feedRecyclerView.itemAnimator = DefaultItemAnimator()
        feedRecyclerView.setHasFixedSize(true)
        feedRecyclerView.addOnItemTouchListener(object :
            RecyclerView.SimpleOnItemTouchListener() {})
        feedRecyclerView.addOnScrollListener(object : RecyclerView.OnScrollListener() {
            override fun onScrollStateChanged(recyclerView: RecyclerView, newState: Int) {
                when (newState) {
                    // the RV scroll state to IDLE：start preload the video feed in current screen
                    RecyclerView.SCROLL_STATE_IDLE -> {
                        endScrollY = recyclerView.computeVerticalScrollOffset()
                        rvScrollDistance = endScrollY - startScrollY
                        tryPreCacheVideoIfNeed(recyclerView, "onScrollStateChangedV2")
                    }
                    // the RV scroll state to DRAGGING： stop preload the video feed
                    RecyclerView.SCROLL_STATE_DRAGGING -> {
                        startScrollY = recyclerView.computeVerticalScrollOffset()
                        mediaPreload.stop()
                    }
                    // the RV scroll state to SETTLING：stop preload the video feed
                    RecyclerView.SCROLL_STATE_SETTLING -> {
                        mediaPreload.stop()
                    }
                }
            }

            override fun onScrolled(recyclerView: RecyclerView, dx: Int, dy: Int) {
                super.onScrolled(recyclerView, dx, dy)
                clearAndStopOtherPlayer()
                if (recyclerView.canScrollVertically(1)) {
                    return
                }
                val moreDataList = getHomeFeedData()
                feedAdapter.loadMoreData(moreDataList)
            }
        })
        // init Adapter
        initFeedAdapter()
    }

    private fun initFeedAdapter() {
        feedAdapter = XhsFeedAdapter(this, getHomeFeedData())
        feedAdapter.setOnItemClickListener(this)
        feedRecyclerView.adapter = feedAdapter
    }

    /** init preload module */
    private fun initPreload() {
        mediaPreload = RedMediaPreload().apply {
            addPreloadCacheListener(object : PreloadCacheListener {
                override fun onPreloadSuccess(
                    cacheFilePath: String,
                    cacheFileSize: Long,
                    request: VideoCacheRequest
                ) {
                    Log.d(TAG, "the request $request is preload success.")
                }

                override fun onPreloadError(errorMsg: Long, request: VideoCacheRequest) {
                    Log.d(TAG, "the request $request is preload error, the error code is:$errorMsg")
                }

                override fun onPreloadStart(request: VideoCacheRequest) {
                    Log.d(TAG, "the request $request is preload start.")
                }
            })
        }
    }

    @MainThread
    private fun tryPreCacheVideoIfNeed(recyclerView: RecyclerView, invoke: String = "unknown") {
        // 1.0 release the last preload result.
        preloadDisposable?.dispose()
        val layoutManager = recyclerView.layoutManager
        if (layoutManager !is StaggeredGridLayoutManager) {
            return
        }
        // 1.1 find the visible items in screen: async execute
        preloadDisposable = layoutManager.getVisibleItemsInfo(recyclerView)
            .doOnNext { visibleItems ->
                // 1.2 covert the visible item model to video cache request
                val pendingCacheReqList = mutableListOf<VideoCacheRequest>()
                for (visibleItemInfo in visibleItems) {
                    if (visibleItemInfo.index == RecyclerView.NO_POSITION) continue
                    val cacheRequest = generateCacheRequest(recyclerView, visibleItemInfo.index)
                    cacheRequest?.let { pendingCacheReqList.add(it) }
                }
                // 1.3 start preload the video cache request
                mediaPreload.start(pendingCacheReqList)
            }
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe { visibleItems ->
                // 1.4 try to play the feed: random select one
                tryFetchVideoPlay(visibleItems, rvScrollDistance)
            }.also { Log.d(TAG, "tryPreCacheVideoIfNeed(), invoke by $invoke") }
    }

    override fun onClick(holder: RecyclerView.ViewHolder?, position: Int, model: XhsFeedModel?) {
        val intent = Intent(this, XhsPlayerActivity::class.java)
        intent.putExtra(INTENT_KEY_URLS, listOf(model?.finalUrl) as Serializable)
        intent.putExtra(INTENT_KEY_SHOW_LOADING, model?.isShowLoading)
        startActivity(intent)
    }

    /** try find the current feed to play */
    private fun tryFetchVideoPlay(visibleItems: List<XhsVisibleItemInfo>, distance: Int) {
        if (!settingConfig.enableCoverVideoPlayer) {
            return
        }
        if (!Utils.beyondScreenOneThirdHeight(this@XhsFeedActivity, distance)) {
            return
        }
        val runnable = Runnable {
            val visibleItem =
                Utils.getElementRandom(visibleItems.filter { it.priority == 100 })
                    ?: return@Runnable
            val currentPosition = visibleItem.index
            val currentItemHolder = currentPlayerMap.remove(currentPosition)
            if (currentItemHolder != null) {
                clearAndStopOtherPlayer()
                // has played, fetch and continue play
                currentItemHolder.videoPlayerView.start()
                currentItemHolder.videoPlayerView.setLoop(true)
                currentItemHolder.videoPlayerView.setVolume(0f, 0f)
                currentItemHolder.videoPlayerView.setOnStartPlayListener { _, _, _ ->
                    currentItemHolder.imageView.visibility = View.INVISIBLE
                }
                currentPlayerMap[currentPosition] = currentItemHolder
            } else {
                clearAndStopOtherPlayer()
                val itemModel = feedAdapter.items.getOrNull(currentPosition) ?: return@Runnable
                if (itemModel.canPlay == false) {
                    return@Runnable
                }
                // has never played, init player and start play
                val itemView =
                    feedLayoutManager.findViewByPosition(currentPosition) ?: return@Runnable
                val itemHolder = feedRecyclerView.getChildViewHolder(itemView) as XhsFeedViewHolder
                itemHolder.videoPlayerView.setVideoPath(itemModel.finalUrl)
                itemHolder.videoPlayerView.setLoop(true)
                itemHolder.videoPlayerView.setVolume(0f, 0f)
                itemHolder.videoPlayerView.setOnStartPlayListener { _, _, _ ->
                    itemHolder.imageView.visibility = View.INVISIBLE
                }
                currentPlayerMap[currentPosition] = itemHolder
            }
        }
        feedRecyclerView.postDelayed(runnable, 100)
    }

    /** stop all media player */
    private fun clearAndStopOtherPlayer() {
        currentPlayerMap.values.forEach { holder ->
            holder.imageView.visibility = View.VISIBLE
            holder.videoPlayerView.release()
        }
        currentPlayerMap.clear()
    }
}