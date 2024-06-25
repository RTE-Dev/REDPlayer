import PlayerController from './DemoPlayerController'
import DemoStateHolder from './DemoStateHolder'
import { PlayerCoreType, RedPlayerXComponentController, SourceType } from '@xhs/redplayer'
import { router } from '@kit.ArkUI'
import PlayerSettingsConfig from '../global/PlayerSettingsConfig'

@Entry
@Component
struct DemoRedPlayerPage {
  url: string = (router.getParams() as Record<string, string>)['url']
  fd: number = (router.getParams() as Record<string, number>)['fd']
  source: string = (router.getParams() as Record<string, string>)['source']
  useSystemPlayer: boolean = PlayerSettingsConfig.useSystemPlayer
  coreType: PlayerCoreType = PlayerSettingsConfig.useSystemPlayer ? PlayerCoreType.TYPE_AV_PLAYER : PlayerCoreType.TYPE_RED_PLAYER
  @State demoStateHolder: DemoStateHolder = new DemoStateHolder()
  playerController = new PlayerController(this.demoStateHolder)
  xComponentController: RedPlayerXComponentController = new RedPlayerXComponentController(this.coreType)

  build() {
    Stack() {
      // player render part
      Column() {
        XComponent({
          id: this.xComponentController.getSurfaceId(),
          type: this.xComponentController.getSurfaceType(),
          libraryname: this.xComponentController.getLibraryName(),
          controller: this.xComponentController
        })
          .onLoad(() => {
            this.playerController.createMediaPlayer(this.coreType).then((player) => {
              this.playerController.setDataSource({
                url: this.url,
                fd: this.fd,
                sourceType:  this.source == 'local' ? SourceType.FD : SourceType.URL,
                logHead: "demoPlay",
                enableAccurateSeek: true,
                isAutoLoop: PlayerSettingsConfig.useLoopPlay,
                useSoftDecoder: PlayerSettingsConfig.useSoftDecoder,
                isLive: this.source == 'live',
              })?.then(() => {
                this.playerController.setSurfaceId(this.xComponentController.getSurfaceId())
                this.playerController.prepare()?.then(() => {
                  this.playerController.start()
                })
              })
            })
          })
          .onDestroy(() => {
            this.playerController.release()
          })
          .width(this.demoStateHolder.videoViewWidth)
          .height(this.demoStateHolder.videoViewHeight)
      }.width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)

      // player control part
      Column() {
        Row() {
          Image($r('app.media.icon_back'))
            .width(36)
            .height(36)
            .objectFit(ImageFit.Contain)
            .margin(20)
            .onClick(() => {
              router.back()
            })
          Text('Demo of video detail page')
            .fontColor('#FFFFFF')
            .fontSize(18)
            .fontWeight(FontWeight.Bold)
        }
        .width('100%')
        .alignItems(VerticalAlign.Center)

        Blank()

        // pause & start button
        Row() {
          Image($r(this.demoStateHolder.isStart ? 'app.media.icon_pause' : 'app.media.icon_play'))
            .width(36)
            .height(36)
            .objectFit(ImageFit.Contain)
            .margin(15)
            .onClick(() => {
              if (this.demoStateHolder.isStart) {
                this.playerController.pause()
              } else {
                this.playerController.start()
              }
            })
          Slider({
            value: this.demoStateHolder.videoPosition,
            min: 0,
            max: this.demoStateHolder.videoDuration,
            style: SliderStyle.OutSet
          })
            .showTips(true)
            .onChange((value: number, mode: SliderChangeMode) => {
              if (mode == SliderChangeMode.End || mode == SliderChangeMode.Click) {
                this.playerController.seek(value)
              }
            })
            .width('60%')
            .selectedColor('#FFFFFF')
            .trackColor('#AAAAAA')
          Blank()
          Button('speed')
            .margin(10)
            .onClick(() => {
              TextPickerDialog.show({
                range: ['X0.5', 'X1', 'X1.5', 'X2'],
                selected: this.demoStateHolder.speedIndex,
                onAccept: (value: TextPickerResult) => {
                  this.demoStateHolder.speedIndex = value.index as number
                  this.playerController.setSpeed(0.5 * (this.demoStateHolder.speedIndex + 1))
                }
              })
            })
        }
        .width('100%')
        .alignItems(VerticalAlign.Center)
      }
      .width('100%')
      .height('100%')

      // player debug info part
      DebugInfoView({
        videoUrl: this.demoStateHolder.url,
        videoWidth: this.demoStateHolder.videoWidth,
        videoHeight: this.demoStateHolder.videoHeight,
        videoCodec: this.demoStateHolder.videoCodec,
        audioCodec: this.demoStateHolder.audioCodec,
      })
      .margin({top: 50, left: 15})
      .visibility(PlayerSettingsConfig.displayDebugPanel ? Visibility.Visible : Visibility.None)
    }
    .alignContent(Alignment.TopStart)
    .width('100%')
    .height('100%')
    .backgroundColor('#000000')
  }
}

@Component
struct DebugInfoView {
  @Prop videoWidth: number = 0
  @Prop videoHeight: number = 0
  @Prop videoUrl: string = ''
  @Prop videoCodec: string = ''
  @Prop audioCodec: string = ''

  build() {
    Column() {
      // videoWidth
      Row() {
        Text('videoWidth')
          .fontSize(10)
          .fontColor('#00FF00')
          .width(60)
          .textAlign(TextAlign.Start)
        Text(this.videoWidth.toString())
          .fontSize(10)
          .fontColor('#00FF00')
          .width(200)
          .textAlign(TextAlign.Start)
          .margin({left: 5})
      }

      // videoHeight
      Row() {
        Text('videoHeight')
          .fontSize(10)
          .fontColor('#00FF00')
          .width(60)
          .textAlign(TextAlign.Start)
        Text(this.videoHeight.toString())
          .fontSize(10)
          .fontColor('#00FF00')
          .width(200)
          .textAlign(TextAlign.Start)
          .margin({left: 5})
      }

      // videoUrl
      Row() {
        Text('videoUrl')
          .fontSize(10)
          .fontColor('#00FF00')
          .width(60)
          .textAlign(TextAlign.Start)
        Text(this.videoUrl)
          .fontSize(10)
          .fontColor('#00FF00')
          .width(200)
          .textAlign(TextAlign.Start)
          .margin({left: 5})
      }

      // coreType
      Row() {
        Text('coreType')
          .fontSize(10)
          .fontColor('#00FF00')
          .width(60)
          .textAlign(TextAlign.Start)
        Text(PlayerSettingsConfig.useSystemPlayer ? 'AVPlayerCore' : 'RedPlayerCore')
          .fontSize(10)
          .fontColor('#00FF00')
          .width(200)
          .textAlign(TextAlign.Start)
          .margin({left: 5})
      }

      // video codec info
      Row() {
        Text('videoCodec')
          .fontSize(10)
          .fontColor('#00FF00')
          .width(60)
          .textAlign(TextAlign.Start)
        Text(this.videoCodec)
          .fontSize(10)
          .fontColor('#00FF00')
          .width(200)
          .textAlign(TextAlign.Start)
          .margin({left: 5})
      }

      // audio codec info
      Row() {
        Text('audioCodec')
          .fontSize(10)
          .fontColor('#00FF00')
          .width(60)
          .textAlign(TextAlign.Start)
        Text(this.audioCodec)
          .fontSize(10)
          .fontColor('#00FF00')
          .width(200)
          .textAlign(TextAlign.Start)
          .margin({left: 5})
      }
    }
    .backgroundColor('#22FFFFFF')
    .padding(10)
  }
}