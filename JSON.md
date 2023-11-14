There is an example of JSON format.

```
{
"stream": { /* Different encoding format streams */
"h264": [{
"width": 720, /* video width */
"height": 1280, /* video height */
"avg_bitrate": 1035154, /* video average bitrate */
"master_url": "http://example_h264_720x1080_master.mp4", /* video URL */
"backup_urls": ["http://example_h264_720x1080_backup1.mp4", "http://example_h264_720x1080_backup2.mp4", "http://example_h264_720x1080_backup3.mp4", "http://example_h264_720x1080_backup4.mp4"] /* Backup URLs used when there is a problem playing the master URL */
}],
/* Multiple streams can exist within the same encoding format */
"h265": [{ /* h265 stream1 */
"width": 720,
"height": 1280,
"avg_bitrate": 687026,
"master_url": "http://example_h265_720x1080_master.mp4",
"backup_urls": ["http://example_h265_720x1080_backup1.mp4", "http://example_h265_720x1080_backup2.mp4", "http://example_h265_720x1080_backup3.mp4", "http://example_h265_720x1080_backup4.mp4"]
}, { /* h265 stream2 */
"width": 1080,
"height": 1920,
"avg_bitrate": 939554,
"master_url": "http://example_h265_1080x1920_master.mp4",
"backup_urls": ["http://example_h265_1080x1920_backup1.mp4", "http://example_h265_1080x1920_backup2.mp4", "http://example_h265_1080x1920_backup3.mp4", "http://example_h265_1080x1920_backup4.mp4"]
}]
}
}
```