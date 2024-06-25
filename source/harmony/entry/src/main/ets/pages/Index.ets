import PlayerSettingsConfig from '../global/PlayerSettingsConfig'
import { router } from '@kit.ArkUI'
import picker from '@ohos.file.picker'
import { BusinessError } from '@ohos.base'
import fs from '@ohos.file.fs'

@Entry
@Component
struct Index {
  @State fontColor: string = '#AAAAAA'
  @State selectedFontColor: string = '#FFFFFF'
  @State currentIndex: number = 0
  private controller: TabsController = new TabsController()

  @Builder
  TabBuilder(index: number, tabName: string, icon: Resource | string, iconSelected: Resource | string) {
    Column() {
      Image(this.currentIndex === index ? iconSelected : icon)
        .width(24)
        .height(24)
        .margin({ bottom: 4 })
        .objectFit(ImageFit.Contain)
      Text(tabName)
        .fontColor(this.currentIndex === index ? this.selectedFontColor : this.fontColor)
        .fontSize(10)
        .fontWeight(500)
        .lineHeight(14)
        .padding({ bottom: 10 })
    }.width('100%')
  }

  build() {
    Column() {
      // red player title content
      Row() {
        Image($r('app.media.icon_title_logo'))
          .width(24)
          .height(24)
          .objectFit(ImageFit.Contain)
        Text($r('app.string.app_name'))
          .fontColor('#FFFFFF')
          .fontSize(18)
          .fontWeight(FontWeight.Bold)
          .margin({ left: 10 })
      }
      .width('100%')
      .height('10%')
      .alignItems(VerticalAlign.Center)
      .justifyContent(FlexAlign.Center)
      .padding({ top: 20, bottom: 20 })

      // tab content
      Tabs({ barPosition: BarPosition.End, controller: this.controller }) {
        // home tab
        TabContent() {
          Column() {
            HomeItemView({
              icon: $r('app.media.icon_url'),
              title: 'VIDEO URL'
            }).onClick(() => {
              router.pushUrl({
                url: 'pages/DemoVideoUrlPage',
                params: {
                  source: 'video'
                }
              })
            })
            HomeItemView({
              icon: $r('app.media.icon_live'),
              title: 'LIVE URL'
            }).onClick(() => {
              router.pushUrl({
                url: 'pages/DemoVideoUrlPage',
                params: {
                  source: 'live'
                }
              })
            })
            HomeItemView({
              icon: $r('app.media.icon_album'),
              title: 'Play local Resource'
            }).onClick(() => {
              try {
                let videoSelectOptions = new picker.PhotoSelectOptions()
                videoSelectOptions.MIMEType = picker.PhotoViewMIMETypes.VIDEO_TYPE
                videoSelectOptions.maxSelectNumber = 1
                let photoPicker = new picker.PhotoViewPicker()
                photoPicker.select(videoSelectOptions).then((selectResult) => {
                  if (selectResult.photoUris.length <= 0) {
                    return
                  }
                  console.info('PhotoViewPicker.select successfully, PhotoSelectResult uri: ' + JSON.stringify(selectResult))
                  fs.open(selectResult.photoUris[0]).then((file: fs.File) => {
                    router.pushUrl({
                      url: 'pages/DemoRedPlayerPage',
                      params: {
                        fd: file.fd,
                        source: 'local'
                      }
                    })
                  }).catch((err: BusinessError) => {
                    console.error("open file failed with error message: " + err.message + ", error code: " + err.code)
                  })
                })
              } catch (err) {
                console.error('PhotoViewPicker failed with err: ' + err)
              }
            })
            HomeItemView({
              icon: $r('app.media.icon_url'),
              title: 'Preload URL'
            }).onClick(() => {
              router.pushUrl({
                url: 'pages/DemoVideoUrlPage',
                params: {
                  source: 'preload'
                }
              })
            })
          }.width('100%').height('100%')
        }.tabBar(this.TabBuilder(0, 'HOME', $r('app.media.icon_home'), $r('app.media.icon_home_selected')))

        // settings tab
        TabContent() {
          Column() {
            SettingsSectionTitleView({
              title: 'Media Render Setting'
            })
            SettingsItemView({
              title: 'Display Debug Panel',
              hint: 'Can show FPS, decoder, and other information.',
              configType: SettingConfig.DISPLAY_DEBUG_PANEL,
            })

            SettingsSectionTitleView({
              title: 'Media Player Setting'
            })
            SettingsItemView({
              title: 'Use Soft Decoder',
              hint: 'Default use system hard decoder, switch to soft decoder if open.',
              configType: SettingConfig.USE_SOFT_DECODER,
            })
            SettingsItemView({
              title: 'Use System Player',
              hint: 'Red player opensource core is used by default, switch to harmony system AVPlayer if open.',
              configType: SettingConfig.USE_SYSTEM_PLAYER,
            })

            SettingsSectionTitleView({
              title: 'Video Play Setting'
            })
            SettingsItemView({
              title: 'Loop Play',
              hint: 'If open, player will auto loop play after once complete.',
              configType: SettingConfig.USE_LOOP_PLAY,
            })
          }.width('100%').height('100%')
        }.tabBar(this.TabBuilder(1, 'SETTINGS', $r('app.media.icon_setting'), $r('app.media.icon_setting_selected')))
      }
      .vertical(false)
      .onChange((index: number) => {
        this.currentIndex = index
      })
      .width('100%')
      .height('90%')
    }
    .width('100%')
    .backgroundColor('#000000')
  }
}

@Component
struct HomeItemView {
  @Prop icon: Resource | string
  @Prop title: string

  build() {
    Row() {
      Image(this.icon)
        .width(32)
        .height(32)
        .objectFit(ImageFit.Contain)
      Text(this.title)
        .fontColor('#FFFFFF')
        .fontSize(16)
        .fontWeight(FontWeight.Medium)
        .margin({ left: 10 })
    }.width('100%')
    .alignItems(VerticalAlign.Center)
    .padding({
      top: 8,
      bottom: 8,
      left: 16,
      right: 16
    })
    .margin({ top: 8 })
  }
}

@Component
struct SettingsItemView {
  @Prop title: string
  @Prop hint: string
  @Prop configType: SettingConfig

  build() {
    Row() {
      Column() {
        Text(this.title)
          .fontColor('#FFFFFF')
          .fontSize(14)
          .fontWeight(FontWeight.Medium)
          .margin({ left: 10 })
        Text(this.hint)
          .fontColor('#AAAAAA')
          .fontSize(11)
          .margin({ left: 10, top: 10 })
      }.width('80%')
      .alignItems(HorizontalAlign.Start)
      .justifyContent(FlexAlign.Center)

      Toggle({ type: ToggleType.Switch, isOn: false })
        .selectedColor('#007DFF')
        .switchPointColor('#FFFFFF')
        .switchStyle({
          unselectedColor: '#AAAAAA'
        })
        .onChange((isOn: boolean) => {
          if (this.configType == SettingConfig.DISPLAY_DEBUG_PANEL) {
            PlayerSettingsConfig.displayDebugPanel = isOn
          } else if (this.configType == SettingConfig.USE_SOFT_DECODER) {
            PlayerSettingsConfig.useSoftDecoder = isOn
          } else if (this.configType == SettingConfig.USE_SYSTEM_PLAYER) {
            PlayerSettingsConfig.useSystemPlayer = isOn
          } else if (this.configType == SettingConfig.USE_LOOP_PLAY) {
            PlayerSettingsConfig.useLoopPlay = isOn
          }
        })
    }
    .width('100%')
    .margin({ top: 8 })
  }
}

@Component
struct SettingsSectionTitleView {
  @Prop title: string

  build() {
    Text(this.title)
      .fontColor('#007DFF')
      .fontSize(13)
      .fontWeight(FontWeight.Bold)
      .margin({ left: 10, top: 20, bottom: 8 })
      .width('100%')
      .textAlign(TextAlign.Start)
  }
}

enum SettingConfig {
  DISPLAY_DEBUG_PANEL,
  USE_SOFT_DECODER,
  USE_SYSTEM_PLAYER,
  USE_LOOP_PLAY,
}