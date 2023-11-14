package com.xingin.openredplayer.player.section

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.xingin.openredplayer.R

class XhsSectionAdapter(
    private val context: Activity,
    private val models: List<String>,
    private var currentUrl: String
) : RecyclerView.Adapter<XhsSectionViewHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): XhsSectionViewHolder {
        val item =
            LayoutInflater.from(parent.context).inflate(R.layout.item_section_layout, parent, false)
        return XhsSectionViewHolder(item)
    }

    @SuppressLint("SetTextI18n")
    override fun onBindViewHolder(holder: XhsSectionViewHolder, position: Int) {
        val model = models[position]
        holder.textView.text =
            context.getString(R.string.detail_play_section_item_title) + (position + 1)
        if (model == currentUrl) {
            holder.itemView.setBackgroundResource(R.drawable.item_corners_5_background_fb7299)
            holder.textView.setTextColor(context.resources.getColor(R.color.color_FB7299))
            holder.loadingIndicatorView.visibility = View.VISIBLE
        } else {
            holder.itemView.setBackgroundResource(R.drawable.item_corners_5_background_ffffff)
            holder.textView.setTextColor(context.resources.getColor(R.color.white))
            holder.loadingIndicatorView.visibility = View.GONE
        }
        holder.textView.setOnClickListener {
            mOnItemClickListener?.onClick(holder, position, model)
        }
    }

    override fun getItemCount(): Int {
        return models.size
    }

    private var mOnItemClickListener: OnSectionItemClickListener? = null
    fun setOnSectionItemClickListener(onItemClickListener: OnSectionItemClickListener?) {
        mOnItemClickListener = onItemClickListener
    }

    @SuppressLint("NotifyDataSetChanged")
    fun updateCurrentPlayUrl(url: String) {
        currentUrl = url
        notifyDataSetChanged()
    }

    interface OnSectionItemClickListener {
        fun onClick(holder: RecyclerView.ViewHolder?, position: Int, model: String?)
    }
}