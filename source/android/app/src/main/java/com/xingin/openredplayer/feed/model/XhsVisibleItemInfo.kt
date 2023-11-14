package com.xingin.openredplayer.feed.model

import android.graphics.Rect
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.StaggeredGridLayoutManager
import com.xingin.openredplayer.XhsMediaApplication
import com.xingin.openredplayer.feed.XhsFeedAdapter
import com.xingin.openredplayer.utils.Utils
import com.xingin.openredpreload.impl.model.VideoCacheRequest
import kotlin.math.min
import io.reactivex.rxjava3.core.Observable
import io.reactivex.rxjava3.schedulers.Schedulers
import java.util.TreeMap

/**
 * RecyclerView VisibleItem
 *
 * @property index
 * @property priority
 * @property column
 */
class XhsVisibleItemInfo constructor(
    val index: Int,
    val priority: Int,
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false
        other as XhsVisibleItemInfo
        if (index != other.index) return false
        return true
    }

    override fun hashCode(): Int {
        return index
    }

    override fun toString(): String {
        return "XhsVisibleItemInfo(index=$index, priority=$priority)"
    }

}

/** get StaggeredGridLayoutManager visible Items in screen */
fun StaggeredGridLayoutManager.getVisibleItemsInfo(recyclerView: RecyclerView): Observable<List<XhsVisibleItemInfo>> {
    val recyclerViewRect = Rect()
    recyclerView.getGlobalVisibleRect(recyclerViewRect)
    val minVisibleItemPosition: Int =
        findFirstVisibleItemPositions(null).minOrNull() ?: RecyclerView.NO_POSITION
    val maxVisibleItemPosition: Int =
        findLastVisibleItemPositions(null).maxOrNull() ?: RecyclerView.NO_POSITION
    if (minVisibleItemPosition == RecyclerView.NO_POSITION || maxVisibleItemPosition == RecyclerView.NO_POSITION) {
        return Observable.empty()
    }
    val resultMap = TreeMap<Int, XhsVisibleItemInfo>()
    return Observable.range(minVisibleItemPosition, maxVisibleItemPosition).map { position ->
        val itemView = findViewByPosition(position)
        val itemRect = Rect().apply {
            itemView?.getGlobalVisibleRect(this)
        }
        val itemHeight = itemView?.height ?: 0
        return@map Triple(
            itemRect, itemHeight, position
        )
    }.observeOn(Schedulers.io()).map { triple ->
        val itemRect = triple.first
        val itemHeight = triple.second
        val itemPosition = triple.third
        val percentFirst = min(
            when {
                // the item height is 0: no impression, ignore
                itemHeight == 0 -> 0
                // the item is in the rv: calculate the impression percent
                itemRect.bottom >= recyclerViewRect.bottom -> {
                    (recyclerViewRect.bottom - itemRect.top) * 100 / itemHeight
                }
                // the item beyond the rv: calculate the percent in rv
                else -> {
                    (itemRect.bottom - recyclerViewRect.top) * 100 / itemHeight
                }
            }, 100
        )
        return@map XhsVisibleItemInfo(itemPosition, percentFirst)
    }.toList().map { items ->
        items.forEach {
            if (!resultMap.containsKey(it.index)) {
                resultMap[it.index] = it
            }
        }
        // return with the order in screen: also can order by biz priority
        return@map resultMap.values.toList()
    }.toObservable()
}

/** get the index feed in adapter, and convert the video cache request */
fun generateCacheRequest(recyclerView: RecyclerView, index: Int): VideoCacheRequest? {
    val adapter = recyclerView.adapter ?: return null
    if (adapter !is XhsFeedAdapter) {
        return null
    }
    return when (val feed = adapter.items.getOrNull(index)) {
        is XhsFeedModel -> {
            createCacheRequest(feed)
        }

        else -> null
    }
}

val DEFAULT_CACHE_VIDEO_DIR = Utils.getVideCacheDir()
const val DEFAULT_CACHE_VIDEO_SIZE_MOBILE = 512 * 1024L
const val DEFAULT_CACHE_VIDEO_MAX_SIZE = 200 * 1024L * 1024L
const val DEFAULT_CACHE_VIDEO_MAX_ENTRIES = 200L
const val DEFAULT_CACHE_VIDEO_BUSINESS_LINE = "home_feed"

/** create the VideoCacheRequest through feed */
fun createCacheRequest(xhsFeedModel: XhsFeedModel): VideoCacheRequest {
    return VideoCacheRequest(
        "",
        xhsFeedModel.videoUrl ?: "",
        xhsFeedModel.videoJsonUrl ?: "",
        DEFAULT_CACHE_VIDEO_DIR,
        DEFAULT_CACHE_VIDEO_SIZE_MOBILE,
        DEFAULT_CACHE_VIDEO_MAX_SIZE,
        DEFAULT_CACHE_VIDEO_MAX_ENTRIES,
        DEFAULT_CACHE_VIDEO_BUSINESS_LINE,
    )
}