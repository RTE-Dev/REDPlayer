import {IMediaPlayer, IMediaPlayerStateListener, RedPlayerFactory, PlayerCoreType, RedPlayerDataSource} from '@xhs/redplayer'
import DemoStateHolder from './DemoStateHolder'
import display from '@ohos.display';

export default class DemoPlayerController implements IMediaPlayerStateListener {
  static TAG = "DemoPlayerController"
  private player: IMediaPlayer | null = null
  demoStateHolder: DemoStateHolder
  dataSource: RedPlayerDataSource | undefined

  constructor(demoStateHolder: DemoStateHolder) {
    this.demoStateHolder = demoStateHolder
  }

  public createMediaPlayer(coreType: PlayerCoreType) {
    return RedPlayerFactory.createMediaPlayer(this, coreType).then((player) => {
      this.player = player
    })
  }

  public setDataSource(dataSource: RedPlayerDataSource): Promise<void> | undefined {
    this.dataSource = dataSource
    return this.player?.setDataSource(dataSource)
  }

  public setSurfaceId(id: string) {
    this.player?.setSurfaceId(id)
  }

  public prepare(): Promise<void> | undefined {
    return this.player?.prepare()
  }

  public start() {
    this.player?.start()
  }

  public pause() {
    this.player?.pause()
  }

  public stop() {
    this.player?.stop()
  }

  public release() {
    this.player?.release()
  }

  public seek(time: number) {
    if (this.dataSource?.isLive != true) {
      this.player?.seek(time)
      console.log(DemoPlayerController.TAG + " seek " + time)
    }
  }

  public setSpeed(speed: number) {
    this.player?.setSpeed(speed)
    console.log(DemoPlayerController.TAG + " setSpeed " + speed)
  }

  public setVolume(volume: number) {
    this.player?.setVolume(volume)
  }

  onPlayerCreated() {}

  onInitialized() {}

  onPrepared() {
    if (this.demoStateHolder && this.player) {
      this.demoStateHolder.videoDuration = this.player.getVideoDuration()
      let width = this.player.getVideoWidth()
      let height = this.player.getVideoHeight()

      this.demoStateHolder.videoWidth = width
      this.demoStateHolder.videoHeight = height
      this.demoStateHolder.url = this.player.getUrl()

      this.demoStateHolder.videoCodec = this.player.getVideoCodecInfo()
      this.demoStateHolder.audioCodec = this.player.getAudioCodecInfo()

      let screenWidth = display.getDefaultDisplaySync().width
      let screenHeight = display.getDefaultDisplaySync().height

      this.demoStateHolder.videoViewHeight = this.player.getVideoHeight()
      let videoRatio = width / height
      let screenRatio = screenWidth / screenHeight
      if (videoRatio > screenRatio) {
        this.demoStateHolder.videoViewWidth = screenWidth + 'px'
        this.demoStateHolder.videoViewHeight = screenWidth / videoRatio + 'px'
      } else {
        this.demoStateHolder.videoViewHeight = screenHeight + 'px'
        this.demoStateHolder.videoViewWidth = screenHeight * videoRatio + 'px'
      }
    }
    if (this.dataSource && this.dataSource.isAutoStart) {
      this.demoStateHolder.isStart = true
    }
    console.log("onPrepared " + "video width * height:" + this.player?.getVideoWidth() + " * " + this.player?.getVideoHeight())
  }

  onStarted() {
    console.log(DemoPlayerController.TAG + " onStarted ")
    this.demoStateHolder.isStart = true
  }

  onFirstFrameRendered() {
    console.log(DemoPlayerController.TAG + " onFirstFrameRendered ")
  }

  onPositionUpdated(time: number) {
    console.log(DemoPlayerController.TAG + " onPositionUpdated " + time)
    if (this.demoStateHolder) {
      this.demoStateHolder.videoPosition = time
    }
  }

  onPaused() {
    console.log(DemoPlayerController.TAG + " onPaused ")
    this.demoStateHolder.isStart = false
  }

  onStopped() {
    console.log(DemoPlayerController.TAG + " onStopped ")
    this.demoStateHolder.isStart = false
  }

  onCompleted() {
    console.log(DemoPlayerController.TAG + " onCompleted ")
    if (this.dataSource && this.dataSource.isAutoLoop) {
      this.demoStateHolder.isStart = false
    }
  }

  onReleased() {
    console.log(DemoPlayerController.TAG + " onReleased ")
    this.demoStateHolder.isStart = false
  }

  onError(reason: string) {
    console.log(DemoPlayerController.TAG + " onError " + reason)
  }

  onSeekDone(): void {
    console.log(DemoPlayerController.TAG + " onSeekDone ")
  }

  onBufferingStart(): void {
    console.log(DemoPlayerController.TAG + " onBufferingStart ")
  }

  onBufferingEnd(): void {
    console.log(DemoPlayerController.TAG + " onBufferingEnd ")
  }
}