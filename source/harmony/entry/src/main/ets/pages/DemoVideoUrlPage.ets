import { buffer } from '@kit.ArkTS'
import { promptAction, router } from '@kit.ArkUI'
import { RedPlayerPreload } from '@xhs/redplayer'

@Entry
@Component
struct DemoVideoUrlPage {
  source: string = (router.getParams() as Record<string, string>)['source']
  static originDemoUrl = buffer.from('aHR0cHM6Ly9zbnMtdmlkZW8tYWwueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzQwNS8wMWU1ODNjYjZlMGZlZDVhMDEwMzcwMDM4YzhhZDk2MmZiXzQwNS5tcDQ=', 'base64').toString('binary')
  @State urlText: string = DemoVideoUrlPage.originDemoUrl
  currentUrl: string = this.urlText
  controller: TextAreaController = new TextAreaController()

  build() {
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
        Text('Demo of custom URL page')
          .fontColor('#FFFFFF')
          .fontSize(18)
          .fontWeight(FontWeight.Bold)
      }.width('100%').alignItems(VerticalAlign.Center)
      TextArea({
        placeholder: 'Input ' + this.source + ' url for next step',
        controller: this.controller,
        text: this.urlText
      })
        .placeholderFont({ size: 16, weight: 400 })
        .width('100%')
        .height('35%')
        .margin(20)
        .fontSize(16)
        .fontColor('#182431')
        .backgroundColor('#FFFFFF')
        .onChange((value: string) => {
          this.currentUrl = value
        })
      Row() {
        Button('Url')
          .width('20%')
          .height(36)
          .margin(10)
          .onClick(() => {
            this.urlText = DemoVideoUrlPage.originDemoUrl
          })
        Button('Clear')
          .width('20%')
          .height(36)
          .margin(10)
          .onClick(() => {
            this.urlText = ''
          })
        Button(this.source == 'preload' ? 'Preload' : 'Play')
          .width('20%')
          .height(36)
          .margin(10)
          .onClick(() => {
            if (this.source == 'preload') {
              RedPlayerPreload.open(this.currentUrl, getContext(this).cacheDir + '/redplayer', 768 * 1024)
              promptAction.showToast({
                message: 'Start preload:' + this.currentUrl,
                duration: 2000,
              });
            } else {
              router.pushUrl({
                url: 'pages/DemoRedPlayerPage',
                params: {
                  url: this.currentUrl,
                  source: this.source
                }
              })
            }
          })
      }.width('100%')
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#000000')
  }
}