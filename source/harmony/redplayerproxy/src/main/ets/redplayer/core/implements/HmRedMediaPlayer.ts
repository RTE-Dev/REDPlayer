import { IMediaPlayer } from '../interfaces/IMediaPlayer'
import { IMediaPlayerStateListener } from '../interfaces/IMediaPlayerStateListener'
import util from '@ohos.util'
import { PlayerState } from '../interfaces/IPlayerState'
import { RedPlayerTrackModel } from '../track/RedPlayerTrackModel'
import { RedPlayerDataSource, SourceType } from '../RedPlayerDataSource'
import libnative from 'libredplayer.so'
import { PlayerInstanceManager } from './PlayerInstanceManager'
import { media_event_type, media_info_type } from './RedPlayerNativeEvent'
import { GlobalConfig, RedConfigType } from './RedConfig'

/**
 * Media player ts wrapper class engined by RedPlayer core
 */
export default class HmRedMediaPlayer implements IMediaPlayer, IPlayerEventListener {
  private playerId = util.generateRandomUUID()
  private playerStateListener?: IMediaPlayerStateListener
  private dataSource?: RedPlayerDataSource
  private trackModel: RedPlayerTrackModel = new RedPlayerTrackModel()
  private positionTimer: PositionTimer = new PositionTimer()
  private currentState: PlayerState = PlayerState.STATE_UNDEFINED
  private prepareResolve: () => void

  constructor(playerStateListener?: IMediaPlayerStateListener) {
    this.playerStateListener = playerStateListener
  }

  // ==================================
  // player control function part start
  /**
   * See desc @link{IMediaPlayerControl.createPlayer}
   */
  createPlayer(): Promise<void> {
    this.trackModel.createPlayerTime = Date.now()
    PlayerInstanceManager.onPlayerCreate(this)
    return new Promise<void>((resolve, reject) => {
      libnative.createPlayer(this.playerId)
      this.currentState = PlayerState.STATE_IDLE
      this.trackModel.onPlayerCreatedTime = Date.now()
      this.playerStateListener?.onPlayerCreated()
      resolve()
    })
  }

  /**
   * See desc @link{IMediaPlayerControl.setSurfaceId}
   */
  setSurfaceId(id: string): void {
    libnative.setSurfaceId(this.playerId, id)
  }

  /**
   * See desc @link{IMediaPlayerControl.setDataSource}
   */
  setDataSource(dataSource: RedPlayerDataSource): Promise<void> {
    this.dataSource = dataSource
    return new Promise<void>((resolve, reject) => {
      if (dataSource.sourceType == SourceType.FD) {
        libnative.setDataSourceFd(this.playerId, dataSource.fd)
      } else {
        libnative.setDataSource(this.playerId, dataSource.url)
      }

      if (dataSource.useSoftDecoder) {
        this.setConfigInt(RedConfigType.TYPE_PLAYER, "enable-harmony_vdec", 0)
      }
      if (dataSource.seekAtStart && dataSource.seekAtStart > 0) {
        this.setConfigInt(RedConfigType.TYPE_PLAYER, "seek-at-start", dataSource.seekAtStart)
      }
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "start-on-prepared", dataSource.isAutoStart ? 1 : 0)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "enable-accurate-seek", dataSource.enableAccurateSeek ? 1 : 0)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "drop-frame-before-decoder", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "overlay-format", 842225234)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "soundtouch", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "max-fps", 30)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "framedrop", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "load-on-prepared", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "video-first-no-i-frame", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "speed-release", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "speed-reconnect", 0)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "smart-cache-duration-enable", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "max-buffer-size", 8 * 1024 * 1024)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "first-high-water-mark-ms", 100)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "thread-opt", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "af-force-output-format-sample-fmt", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "af-force-output-sample-fmt", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "opensles", 0)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "speed-switch-cdn", 1)
      this.setConfigStr(RedConfigType.TYPE_PLAYER, "protocol_whitelist", "ffconcat,file,http,https,concat,tcp,tls")
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "enable-netcache", 1)
      this.setConfigInt(RedConfigType.TYPE_PLAYER, "enable-recoder", 1)

      if (GlobalConfig.cachePath) {
        this.setConfigStr(RedConfigType.TYPE_FORMAT, "cache_file_dir", GlobalConfig.cachePath)
      }
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "http-detect-range-support", 0)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "dns_cache_timeout", 0)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "cache_max_dir_capacity", 300 * 1024 * 1024)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "cache_file_forwards_capacity", 256 * 1024)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "auto_save_map", 1)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "reconnect", 0)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "reconnect_streamed", 1)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "timeout", 5 * 1000 * 1000)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "recv_buffer_size", 292 * 1000)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "send_buffer_size", 292 * 1000)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "formatprobesize", 2048)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "fpsprobesize", 1)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "analyzeduration", 90000000)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "analyzemaxduration", 100)
      this.setConfigInt(RedConfigType.TYPE_FORMAT, "flush_packets", 1)
      this.setConfigStr(RedConfigType.TYPE_FORMAT, "fflags", "fastseek")

      this.setConfigInt(RedConfigType.TYPE_CODEC, "skip_loop_filter", 0)
      this.setConfigInt(RedConfigType.TYPE_CODEC, "skip_frame", 0)

      if (dataSource.isLive == true) {
        this.setConfigInt(RedConfigType.TYPE_PLAYER, "infbuf", 1)
        this.setConfigInt(RedConfigType.TYPE_PLAYER, "without-filter", 1)
        this.setConfigInt(RedConfigType.TYPE_FORMAT, "probesize", 8 * 1024)
        this.setConfigInt(RedConfigType.TYPE_FORMAT, "load_file", 0)
        this.setConfigInt(RedConfigType.TYPE_FORMAT, "read_async", 0)
      }

      if (dataSource.isAutoLoop) {
        this.setLoop(true)
      }

      this.currentState = PlayerState.STATE_INITIALIZED
      this.playerStateListener?.onInitialized()
      resolve()
    })
  }

  /**
   * See desc @link{IMediaPlayerControl.setConfigStr}
   */
  setConfigStr(type: number, name: string, value: string): void {
    libnative.setConfigStr(this.playerId, type, name, value)
  }

  /**
   * See desc @link{IMediaPlayerControl.setConfigInt}
   */
  setConfigInt(type: number, name: string, value: number): void {
    libnative.setConfigInt(this.playerId, type, name, value)
  }

  /**
   * See desc @link{IMediaPlayerControl.prepare}
   */
  prepare(): Promise<void> {
    if (this.trackModel.prepareCalledTime == 0) {
      this.trackModel.prepareCalledTime = Date.now()
    }
    this.currentState = PlayerState.STATE_PREPARING
    return new Promise<void>((resolve, reject) => {
      libnative.prepare(this.playerId)
      this.prepareResolve = resolve
    })
  }

  /**
   * See desc @link{IMediaPlayerControl.seek}
   */
  seek(timeMs: number): void {
    libnative.seek(this.playerId, timeMs)
  }

  /**
   * See desc @link{IMediaPlayerControl.setSpeed}
   */
  setSpeed(speed: number): void {
    libnative.setSpeed(this.playerId, speed)
  }

  /**
   * See desc @link{IMediaPlayerControl.setVolume}
   */
  setVolume(volume: number): void {
    libnative.setVolume(this.playerId, volume)
  }

  /**
   * See desc @link{IMediaPlayerControl.setLoop}
   */
  setLoop(loop: boolean): void {
    let loopInt = loop ? 100000 : 0
    this.setConfigInt(RedConfigType.TYPE_PLAYER, "loop", loopInt)
    libnative.setLoop(this.playerId, loopInt)
  }

  /**
   * See desc @link{IMediaPlayerControl.start}
   */
  start(): Promise<void> {
    if (this.trackModel.startCalledTime == 0) {
      this.trackModel.startCalledTime = Date.now()
    }
    return new Promise<void>((resolve, reject) => {
      libnative.start(this.playerId)
      this.currentState = PlayerState.STATE_PLAYING
      this.playerStateListener?.onStarted()
      this.startPositionTimer()
      resolve()
    })
  }

  /**
   * See desc @link{IMediaPlayerControl.pause}
   */
  pause(): Promise<void> {
    this.positionTimer.stop()
    return new Promise<void>((resolve, reject) => {
      libnative.pause(this.playerId)
      this.currentState = PlayerState.STATE_PAUSED
      this.playerStateListener?.onPaused()
      resolve()
    })
  }

  /**
   * See desc @link{IMediaPlayerControl.stop}
   */
  stop(): Promise<void> {
    this.positionTimer.stop()
    return new Promise<void>((resolve, reject) => {
      libnative.stop(this.playerId)
      this.currentState = PlayerState.STATE_STOPPED
      this.playerStateListener?.onStopped()
      resolve()
    })
  }

  /**
   * See desc @link{IMediaPlayerControl.release}
   */
  release(): Promise<void> {
    if (this.trackModel.releaseCalledTime == 0) {
      this.trackModel.releaseCalledTime = Date.now()
    }
    PlayerInstanceManager.onPlayerRelease(this)
    this.positionTimer.stop()
    return new Promise<void>((resolve, reject) => {
      libnative.release(this.playerId)
      this.currentState = PlayerState.STATE_RELEASED
      this.playerStateListener?.onReleased()
      resolve()
    })
  }

  /**
   * See desc @link{IMediaPlayerControl.reset}
   */
  reset(): Promise<void> {
    if (this.trackModel.releaseCalledTime == 0) {
      this.trackModel.releaseCalledTime = Date.now()
    }
    this.positionTimer.stop()
    return new Promise<void>((resolve, reject) => {
      libnative.reset(this.playerId)
      this.currentState = PlayerState.STATE_RELEASED
      this.playerStateListener?.onReleased()
      this.trackModel = new RedPlayerTrackModel()
      this.positionTimer = new PositionTimer()
      this.currentState = PlayerState.STATE_UNDEFINED
      resolve()
    })
  }
  // player control function part end
  // ==================================

  // ==================================
  // player get function part start
  /**
   * See desc @link{IMediaPlayerParamGetter.getPlayerState}
   */
  getPlayerState(): PlayerState {
    return this.currentState
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getCurrentPosition}
   */
  getCurrentPosition(): number {
    return libnative.getCurrentPosition(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoDuration}
   */
  getVideoDuration(): number {
    return libnative.getVideoDuration(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getLogHead}
   */
  getLogHead(): string {
    return "HmRedMediaPlayer[" + this + "]" + "{" + this.dataSource?.logHead + "}: "
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoWidth}
   */
  getVideoWidth(): number {
    return libnative.getVideoWidth(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoHeight}
   */
  getVideoHeight(): number {
    return libnative.getVideoHeight(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getPlayerId}
   */
  getPlayerId(): string {
    return this.playerId
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getUrl}
   */
  getUrl(): string {
    return libnative.getUrl(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getTrackModel}
   */
  getTrackModel(): RedPlayerTrackModel {
    return this.trackModel
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.isPlaying}
   */
  isPlaying(): boolean {
    return libnative.isPlaying(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.isLooping}
   */
  isLooping(): boolean {
    return libnative.isLooping(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoCachedSizeBytes}
   */
  getVideoCachedSizeBytes(): number {
    return libnative.getVideoCachedSizeBytes(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getAudioCachedSizeBytes}
   */
  getAudioCachedSizeBytes(): number {
    return libnative.getAudioCachedSizeBytes(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoCachedSizeMs}
   */
  getVideoCachedSizeMs(): number {
    return libnative.getVideoCachedSizeMs(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getAudioCachedSizeMs}
   */
  getAudioCachedSizeMs(): number {
    return libnative.getAudioCachedSizeMs(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoCodecInfo}
   */
  getVideoCodecInfo(): string {
    return libnative.getVideoCodecInfo(this.playerId)
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getAudioCodecInfo}
   */
  getAudioCodecInfo(): string {
    return libnative.getAudioCodecInfo(this.playerId)
  }

  // player get function part end
  // ==================================

  /**
   * Player core native event called.
   */
  onNativeEvent(instanceId: string, time: number, event: number, arg1: number, arg2: number): void {
    if (event == media_event_type.MEDIA_INFO) {
      if (arg1 == media_info_type.MEDIA_INFO_VIDEO_RENDERING_START) {
        if (this.trackModel.renderStart == 0) {
          this.trackModel.renderStart = time > 0 ? time : Date.now()
        }
        if (this.trackModel.startOnPlaying == 0) {
          this.trackModel.startOnPlaying = time > 0 ? time : Date.now()
        }
        this.playerStateListener?.onFirstFrameRendered()
      } else if (arg1 == media_info_type.MEDIA_INFO_BUFFERING_START) {
        this.playerStateListener?.onBufferingStart()
      } else if (arg1 == media_info_type.MEDIA_INFO_BUFFERING_END) {
        this.playerStateListener?.onBufferingEnd()
      }
    } else if (event == media_event_type.MEDIA_ERROR) {
      this.playerStateListener?.onError(arg1.toString())
    } else if (event == media_event_type.MEDIA_SEEK_COMPLETE) {
      this.playerStateListener?.onSeekDone()
    } else if (event == media_event_type.MEDIA_PLAYBACK_COMPLETE) {
      this.currentState = PlayerState.STATE_COMPLETED
      this.playerStateListener?.onCompleted()
    } else if (event == media_event_type.MEDIA_PREPARED) {
      this.currentState = PlayerState.STATE_PREPARED
      if (this.trackModel.onPrepareTime == 0) {
        this.trackModel.onPrepareTime = time > 0 ? time : Date.now()
      }
      this.prepareResolve?.()
      this.playerStateListener?.onPrepared()
      if (this.dataSource && this.dataSource.isAutoStart) {
        // if auto start after prepared
        this.startPositionTimer()
      }
    }
  }

  private startPositionTimer() {
    this.positionTimer.start(() => {
      let currentPosition = this.getCurrentPosition()
      if (currentPosition && this.positionTimer.currentPosition != currentPosition) {
        this.positionTimer.currentPosition = currentPosition
        this.playerStateListener?.onPositionUpdated(currentPosition)
      }
    })
  }
}

/**
 * Player core native event callback adapter.
 */
export class PlayerNativeEventAdapter {
  private constructor() {
  }

  private static instance: PlayerNativeEventAdapter
  private _listener: PlayerGlobalNativeEventListener = new PlayerGlobalNativeEventListener()

  public static getInstance(): PlayerNativeEventAdapter {
    if (!PlayerNativeEventAdapter.instance) {
      PlayerNativeEventAdapter.instance = new PlayerNativeEventAdapter()
    }
    return PlayerNativeEventAdapter.instance
  }

  getListener(): PlayerGlobalNativeEventListener | undefined {
    return this._listener
  }
}

class PlayerGlobalNativeEventListener implements IPlayerEventListener {
  public constructor() {}

  onNativeEvent(instanceId: string, time: number, event: number, arg1: number, arg2: number): void {
    let player = PlayerInstanceManager.findPlayerById(instanceId)
    if (player instanceof HmRedMediaPlayer) {
      player.onNativeEvent(instanceId, time, event, arg1, arg2)
    }
  }
}

interface IPlayerEventListener {
  onNativeEvent(instanceId: string, time: number, event: number, arg1: number, arg2: number): void;
}

/**
 * Player position loop update timer.
 */
class PositionTimer {
  private timerId = 0
  currentPosition = 0

  start(listener: () => void) {
    this.stop()
    this.timerId = setInterval(() => {
      listener()
    }, 100)
  }

  stop() {
    if (this.timerId > 0) {
      clearInterval(this.timerId)
      this.timerId = 0
    }
  }
}