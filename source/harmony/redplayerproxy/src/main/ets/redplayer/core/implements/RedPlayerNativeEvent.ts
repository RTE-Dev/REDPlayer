export enum media_event_type {
    MEDIA_NOP               = 0,        // interface test message
    MEDIA_PREPARED          = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE  = 3,        // arg1 = percentage, arg2 = cached duration
    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_SET_VIDEO_SIZE    = 5,        // arg1 = width, arg2 = height
    MEDIA_GET_IMG_STATE     = 6,        // arg1 = timestamp, arg2 = result code, obj = file name
    MEDIA_BPREPARED         = 7,        // arg1=0,arg2=0
    MEDIA_TIMED_TEXT        = 99,       // not supported yet
    MEDIA_ERROR             = 100,      // arg1, arg2
    MEDIA_INFO              = 200,      // arg1, arg2


    MEDIA_SET_VIDEO_SAR     = 10001,    // arg1 = sar.num, arg2 = sar.den
};

export enum media_info_type {
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,
    // The player was started because it was used as the next player for another
    // player, which just completed playback
    MEDIA_INFO_STARTED_AS_NEXT = 2,
    // The player just pushed the very first video frame for rendering
    MEDIA_INFO_VIDEO_RENDERING_START = 3,
    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // MediaPlayer is temporarily pausing playback internally in order to
    // buffer more data.
    MEDIA_INFO_BUFFERING_START = 701,
    // MediaPlayer is resuming playback after filling buffers.
    MEDIA_INFO_BUFFERING_END = 702,
    // Bandwidth in recent past
    MEDIA_INFO_NETWORK_BANDWIDTH = 703,

    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
    MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
    MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
    MEDIA_INFO_METADATA_UPDATE = 802,

    //9xx
    MEDIA_INFO_TIMED_TEXT_ERROR = 900,

    //100xx
    MEDIA_INFO_VIDEO_ROTATION_CHANGED = 10001,
    MEDIA_INFO_AUDIO_RENDERING_START  = 10002,
    MEDIA_INFO_AUDIO_DECODED_START    = 10003,
    MEDIA_INFO_VIDEO_DECODED_START    = 10004,
    MEDIA_INFO_OPEN_INPUT             = 10005,
    MEDIA_INFO_FIND_STREAM_INFO       = 10006,
    MEDIA_INFO_COMPONENT_OPEN         = 10007,
    MEDIA_INFO_VIDEO_SEEK_RENDERING_START = 10008,
    MEDIA_INFO_AUDIO_SEEK_RENDERING_START = 10009,
    MEDIA_INFO_VIDEO_FIRST_PACKET_IN_DECODER =10010,
    MEDIA_INFO_VIDEO_START_ON_PLAYING = 10011,
    MEDIA_INFO_VIDEO_SEI_INFO = 10012,
    MEDIA_INFO_AUDIO_PCMDB_INFO = 10013,
    MEDIA_INFO_URL_CHANGE = 10014,
    MEDIA_INFO_VIDEO_STREAM_ADDED = 10015,

    MEDIA_INFO_MEDIA_ACCURATE_SEEK_COMPLETE = 10100,
    MEDIA_INFO_MEDIA_SEEK_LOOP_COMPLETE =10101,
};
