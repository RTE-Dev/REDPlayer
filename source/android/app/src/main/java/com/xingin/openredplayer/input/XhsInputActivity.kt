package com.xingin.openredplayer.input

import android.content.Intent
import android.os.Bundle
import android.widget.EditText
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.xingin.openredplayer.R
import com.xingin.openredplayer.feed.model.JSON_DATA_SOURCE
import com.xingin.openredplayer.feed.model.VIDEO_ONE
import com.xingin.openredplayer.player.XhsPlayerActivity
import java.io.Serializable

class XhsInputActivity : AppCompatActivity() {
    private lateinit var backView: ImageView
    private lateinit var titleView: TextView
    private lateinit var editText: EditText
    private lateinit var urlButton: TextView
    private lateinit var jsonButton: TextView
    private lateinit var clearButton: TextView
    private lateinit var playButton: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_xhs_input_layout)
        initView()
    }

    private fun initView() {
        backView = findViewById(R.id.top_action_back_view)
        backView.setOnClickListener {
            finish()
        }
        titleView = findViewById(R.id.video_title_view)
        titleView.setText(R.string.input_url_title)
        editText = findViewById(R.id.edit_text_view)
        urlButton = findViewById(R.id.url_button)
        urlButton.setOnClickListener {
            editText.setText(VIDEO_ONE)
        }
        jsonButton = findViewById(R.id.json_button)
        jsonButton.setOnClickListener {
            editText.setText(JSON_DATA_SOURCE)
        }
        playButton = findViewById(R.id.play_button)
        playButton.setOnClickListener {
            val url = editText.text.toString()
            if (url.isEmpty()) {
                return@setOnClickListener
            }
            val intent = Intent(this, XhsPlayerActivity::class.java)
            intent.putExtra(
                XhsPlayerActivity.INTENT_KEY_URLS,
                listOf(url) as Serializable
            )
            intent.putExtra(XhsPlayerActivity.INTENT_KEY_SHOW_LOADING, true)
            startActivity(intent)
        }
        clearButton = findViewById(R.id.clear_button)
        clearButton.setOnClickListener {
            editText.setText("")
        }
    }
}