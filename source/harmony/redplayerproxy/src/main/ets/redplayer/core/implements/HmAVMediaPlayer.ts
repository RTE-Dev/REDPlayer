import media from '@ohos.multimedia.media';
import {IMediaPlayer} from '../interfaces/IMediaPlayer';
import {RedPlayerDataSource, SourceType} from '../RedPlayerDataSource';
import RedPlayerLogger from '../log/RedPlayerLogger';
import RedPlayerLogConst from '../log/RedPlayerLogConst';
import {IMediaPlayerStateListener} from '../interfaces/IMediaPlayerStateListener';
import { PlayerState } from '../interfaces/IPlayerState';
import {RedPlayerTrackModel} from '../track/RedPlayerTrackModel';
import util from '@ohos.util';
import { PlayerInstanceManager } from './PlayerInstanceManager';

/**
 * Media player ts wrapper class engined by HarmonyOS system AVPlayer
 */
export default class HmAVMediaPlayer implements IMediaPlayer {
  private playerId = util.generateRandomUUID()
  private avPlayer?: media.AVPlayer
  private playerStateListener?: IMediaPlayerStateListener
  private dataSource?: RedPlayerDataSource
  private trackModel: RedPlayerTrackModel = new RedPlayerTrackModel()
  private isPreparing: boolean = false
  private dataSourcePromiseResolve?: () => void = null

  constructor(playerStateListener?: IMediaPlayerStateListener) {
    this.playerStateListener = playerStateListener
  }

  // ==================================
  // player control function part start
  // ==================================
  /**
   * See desc @link{IMediaPlayerControl.createPlayer}
   */
  public createPlayer(): Promise<void> {
    this.trackModel.createPlayerTime = Date.now()
    PlayerInstanceManager.onPlayerCreate(this)
    return media.createAVPlayer()
      .then((player) => {
        this.avPlayer = player
        this.registerAVPlayerEventListener(player)
        this.playerStateListener?.onPlayerCreated()
        this.trackModel.onPlayerCreatedTime = Date.now()
      })
      .catch((error) => {
        RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "createAVPlayer error", error)
      })
  }

  /**
   * See desc @link{IMediaPlayerControl.setSurfaceId}
   */
  public setSurfaceId(id: string) {
    if (this.avPlayer) {
      this.avPlayer.surfaceId = id
      RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setSurfaceId finished, id:" + id)
    } else {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setSurfaceId error: avPlayer undefined")
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.setDataSource}
   */
  public setDataSource(dataSource: RedPlayerDataSource): Promise<void> {
    return new Promise((resolve) => {
      RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setDataSource called")
      this.dataSource = dataSource
      if (this.avPlayer) {
        this.dataSourcePromiseResolve = resolve
        if (dataSource.sourceType == SourceType.FD) {
          this.avPlayer.fdSrc =  { fd: dataSource.fd }
        } else {
          this.avPlayer.url = dataSource.url
        }
      } else {
        RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setDataSource error: avPlayer undefined")
      }
    });
  }

  /**
   * Not support in AVPlayer
   */
  setConfigStr(type: number, name: string, value: string): void {
  }

  /**
   * Not support in AVPlayer
   */
  setConfigInt(type: number, name: string, value: number): void {
  }

  /**
   * See desc @link{IMediaPlayerControl.prepare}
   */
  public prepare(): Promise<void> {
    if (this.trackModel.prepareCalledTime == 0) {
      this.trackModel.prepareCalledTime = Date.now()
    }
    if (this.avPlayer) {
      RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "prepare called")
      this.isPreparing = true
      return this.avPlayer.prepare().then(() => {
        RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "prepare finished")
      }).catch((error) => {
        this.isPreparing = false
        RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "prepare error", error)
      })
    } else {
      let error = new Error("avPlayer undefined")
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "prepare error: ", error)
      return new Promise<void>((resolve, reject) => {
        reject(error)
      })
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.seek}
   */
  public seek(timeMs: number) {
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "seek: " + timeMs)
    if (this.avPlayer) {
      this.avPlayer.seek(timeMs)
    } else {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "seek error: avPlayer undefined")
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.setVolume}
   */
  public setVolume(volume: number): void {
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setVolume: " + volume)
    if (this.avPlayer) {
      this.avPlayer.setVolume(volume)
    } else {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setVolume error: avPlayer undefined")
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.setLoop}
   */
  setLoop(loop: boolean): void {
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setLoop: " + loop)
    if (this.avPlayer) {
      this.avPlayer.loop = loop
    } else {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setLoop error: avPlayer undefined")
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.start}
   */
  public start(): Promise<void> {
    if (this.trackModel.startCalledTime == 0) {
      this.trackModel.startCalledTime = Date.now()
    }
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "start called")
    if (this.avPlayer) {
      return this.avPlayer.play().then(() => {
        RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "start finished")
        this.playerStateListener?.onStarted()
      }).catch((error) => {
        RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "start error", error)
      })
    } else {
      let error = new Error("avPlayer undefined")
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "start error: ", error)
      return new Promise<void>((resolve, reject) => {
        reject(error)
      })
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.pause}
   */
  public pause(): Promise<void> {
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "pause called")
    if (this.avPlayer) {
      return this.avPlayer.pause().then(() => {
        RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "pause finished")
      }).catch((error) => {
        RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "pause error", error)
      })
    } else {
      let error = new Error("avPlayer undefined")
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "pause error: ", error)
      return new Promise<void>((resolve, reject) => {
        reject(error)
      })
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.stop}
   */
  public stop(): Promise<void> {
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "stop called")
    if (this.avPlayer) {
      return this.avPlayer.stop().then(() => {
        RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "stop finished")
      }).catch((error) => {
        RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "stop error", error)
      })
    } else {
      let error = new Error("avPlayer undefined")
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "stop error: ", error)
      return new Promise<void>((resolve, reject) => {
        reject(error)
      })
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.release}
   */
  public release(): Promise<void> {
    if (this.trackModel.releaseCalledTime == 0) {
      this.trackModel.releaseCalledTime = Date.now()
    }
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "release called")
    if (this.avPlayer) {
      PlayerInstanceManager.onPlayerRelease(this)
      return this.avPlayer.release().then(() => {
        RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "release finished")
      }).catch((error) => {
        RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "release error", error)
      })
    } else {
      let error = new Error("avPlayer undefined")
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "release error: ", error)
      return new Promise<void>((resolve, reject) => {
        reject(error)
      })
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.reset}
   */
  reset(): Promise<void> {
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "reset")
    if (this.avPlayer) {
      return this.avPlayer.reset()
    } else {
      let error = new Error("avPlayer undefined")
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "reset error: avPlayer undefined")
      return new Promise<void>((resolve, reject) => {
        reject(error)
      })
    }
  }

  /**
   * See desc @link{IMediaPlayerControl.setSpeed}
   */
  public setSpeed(speed: number) {
    if (!this.avPlayer) {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setSpeed error. Need create player first")
      return
    }
    RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setSpeed:" + speed)
    // AVPlayer only support limited selection of speed:
    switch (speed) {
      case 0.75:
        return this.avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_0_75_X)
      case 1:
        return this.avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_1_00_X)
      case 1.25:
        return this.avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_1_25_X)
      case 1.5:
        return this.avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_1_75_X)
      case 2:
        return this.avPlayer.setSpeed(media.PlaybackSpeed.SPEED_FORWARD_2_00_X)
      default:
        RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "setSpeed error, unsupported speed:" + speed)
    }
  }
  // ==================================
  // player control function part end
  // ==================================

  // ==================================
  // player get function part start
  // ==================================
  /**
   * See desc @link{IMediaPlayerParamGetter.getPlayerState}
   */
  public getPlayerState(): PlayerState {
    if (!this.avPlayer) {
      return PlayerState.STATE_UNDEFINED
    }
    let state = this.avPlayer.state
    if (this.isPreparing) {
      return PlayerState.STATE_PREPARING
    }
    switch (state) {
      case 'error':
        return PlayerState.STATE_ERROR
      case 'idle':
        return PlayerState.STATE_IDLE
      case 'released':
        return PlayerState.STATE_RELEASED
      case 'initialized':
        return PlayerState.STATE_INITIALIZED
      case 'prepared':
        return PlayerState.STATE_PREPARED
      case 'playing':
        return PlayerState.STATE_PLAYING
      case 'paused':
        return PlayerState.STATE_PAUSED
      case 'stopped':
        return PlayerState.STATE_STOPPED
      case 'completed':
        return PlayerState.STATE_COMPLETED
    }
    return PlayerState.STATE_IDLE
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getCurrentPosition}
   */
  public getCurrentPosition(): number {
    if (!this.avPlayer) {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "getCurrentPosition error. Need create player first")
      return 0
    }
    return this.avPlayer.currentTime
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoDuration}
   */
  public getVideoDuration(): number {
    if (!this.avPlayer) {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "getVideoDuration error. Need create player first")
      return 0
    }
    return this.avPlayer.duration
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getLogHead}
   */
  public getLogHead(): string {
    return "HmAVMediaPlayer[" + this + "]" + "{" + this.dataSource?.logHead + "}: "
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoWidth}
   */
  public getVideoWidth(): number {
    if (this.avPlayer) {
      return this.avPlayer.width
    } else {
      return 0
    }
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.getVideoHeight}
   */
  public getVideoHeight(): number {
    if (this.avPlayer) {
      return this.avPlayer.height
    } else {
      return 0
    }
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
    if (!this.avPlayer) {
      RedPlayerLogger.e(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "getUrl error. Need create player first")
      return ""
    }
    return this.avPlayer.url
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
    return this.getPlayerState() == PlayerState.STATE_PLAYING
  }

  /**
   * See desc @link{IMediaPlayerParamGetter.isLooping}
   */
  isLooping(): boolean {
    return this.avPlayer?.loop == true
  }

  /**
   * Not support in AVPlayer
   */
  getVideoCachedSizeBytes(): number {
    return 0
  }

  /**
   * Not support in AVPlayer
   */
  getAudioCachedSizeBytes(): number {
    return 0
  }

  /**
   * Not support in AVPlayer
   */
  getVideoCachedSizeMs(): number {
    return 0
  }

  /**
   * Not support in AVPlayer
   */
  getAudioCachedSizeMs(): number {
    return 0
  }

  /**
   * Not support in AVPlayer
   */
  getVideoCodecInfo(): string {
    return ''
  }

  /**
   * Not support in AVPlayer
   */
  getAudioCodecInfo(): string {
    return ''
  }

  // ==================================
  // player get function part end
  // ==================================

  /**
   * AVPlayer event callback.
   */
  private registerAVPlayerEventListener(avPlayer: media.AVPlayer) {
    // first frame rendered:
    avPlayer.on('startRenderFrame', () => {
      RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "startRenderFrame")
      this.playerStateListener?.onFirstFrameRendered()
      if (this.trackModel.startOnPlaying == 0) {
        this.trackModel.startOnPlaying = Date.now()
      }
    })
    // current position update:
    avPlayer.on('timeUpdate', (time: number) => {
      if (this.getPlayerState() == PlayerState.STATE_PREPARING) {
        return
      }
      this.playerStateListener?.onPositionUpdated(time)
    })
    // player state change:
    avPlayer.on('stateChange', async (state, reason) => {
      RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "stateChange:" + state + ",reason:" + reason)
      // defined by harmony
      switch (state) {
        case 'idle':
          break;
        case 'initialized':
          this.playerStateListener?.onInitialized()
          if (this.dataSourcePromiseResolve != null) {
            this.dataSourcePromiseResolve()
            this.dataSourcePromiseResolve = null
          }
          break;
        case 'prepared':
          this.avPlayer.loop = this.dataSource.isAutoLoop
          this.isPreparing = false
          this.playerStateListener?.onPrepared()
          if (this.trackModel.onPrepareTime == 0) {
            this.trackModel.onPrepareTime = Date.now()
          }
          break;
        case 'playing':
          if (this.trackModel.renderStart == 0) {
            this.trackModel.renderStart = Date.now()
          }
          break;
        case 'paused':
          this.playerStateListener?.onPaused()
          break;
        case 'completed':
          this.playerStateListener?.onCompleted()
          break;
        case 'stopped':
          this.playerStateListener?.onStopped()
          break;
        case 'released':
          this.playerStateListener?.onReleased()
          break;
        case 'error':
          this.playerStateListener?.onError(reason.toString())
          break;
        default:
          break;
      }
    })
    // player seek done:
    avPlayer.on('seekDone', async (arg) => {
      RedPlayerLogger.d(RedPlayerLogConst.TAG_RED_PLAYER_PLAY, this.getLogHead() + "seekDone")
      this.playerStateListener?.onSeekDone()
    })
  }
}