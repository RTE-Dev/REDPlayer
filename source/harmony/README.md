# Project Construction
- Module [entry](./entry): Demo guide of Harmony RedPlayer
- Module [redplayerproxy](./redplayerproxy): Harmony RedPlayer SDK
- Shell [build.sh](./build.sh): Harmony RedPlayer SDK HAR build shell

# My build environment
- MacOS 13.3.1
- M1 Pro CPU
- DevEco Studio NEXT Developer Beta1(5.0.3.403)

⚠️NOTICE: HarmonyOS is in internal testing stage, APIs may change in different DevEco Studio version. If different DevEco version is used, there may be some extra adaptation work.

DevEco Studio Download Path: https://developer.huawei.com/consumer/cn/deveco-studio/

# ▶️ Getting Started

## How to integrate into your project
#### STEP-1: Build SDK HAR
- Configure your compile environment in [build.sh](./build.sh)
- Run [build.sh](./build.sh), then get HAR product in path /output/redplayerproxy.har
#### STEP-2: Integrate SDK HAR to your project
- Copy [HAR](/output/redplayerproxy.har) product to your project target module libs
- Open oh-package.json5 file in your project target module, and add har dependency
```json
{
  // ...
  "dependencies": {
    "@xhs/redplayer": "file:./libs/redplayerproxy.har" // your HAR path
  }
  // ...
}
```
- Then import from RedPlayer SDK HAR
```typescript
import { IMediaPlayer } from '@xhs/redplayer'
```
💡TIPS: See more available imports in [index.ets](./redplayerproxy/index.ets)

## How to use RedPlayer to play
#### STEP-0: Initialize RedPlayer global config
Before using RedPlayer, we need initialize RedPlayer components first. Recommend do it in APP AbilityStage#onCreate to ensure initialized in process boot.
```typescript
class YourAbilityStage extends AbilityStage {
  onCreate() {
    let configBuilder = new RedPlayerConfigBuilder()
    configBuilder.cachePath = 'your player cache path'
    configBuilder.logger = 'your custom logger'
    // TIPS: See more global config in RedPlayerInitManager.RedPlayerConfigBuilder
    RedPlayerInitManager.initPlayer(configBuilder) // Init RedPlayer
    RedPlayerPreload.globalInit('your preload cache path', 20, 4, 0) // Init RedPlayer preload component
  }
}
```

#### STEP-1: Create player listener and instance
```typescript
let listener: IMediaPlayerStateListener = new YourListenerImpl()
let player: IMediaPlayer | null = null
RedPlayerFactory.createMediaPlayer(listener).then((p) => {
  // player created
  player = p
})
```
💡TIPS: See all listener callback defines in [IMediaPlayerStateListener](./redplayerproxy/src/main/ets/redplayer/core/interfaces/IMediaPlayerStateListener.ts)

#### STEP-2: Add and config player render view
```typescript
@Entry
@Component
struct PlayerPage {
  // step1: create controller for XComponent
  xComponentController: RedPlayerXComponentController = new RedPlayerXComponentController()
  build() {
    Stack() {
      // ...
      // step2: add and config XComponent to your page for player render
      XComponent({
        id: this.xComponentController.getSurfaceId(),
        type: this.xComponentController.getSurfaceType(),
        libraryname: this.xComponentController.getLibraryName(),
        controller: this.xComponentController
      }).onLoad(() => {
        // step3: setSurfaceId to player after XComponent onLoad
        player.setSurfaceId(this.xComponentController._id)
        // if AVPlayer core:
        // this.playerController.setSurfaceId(this.xComponentController.getXComponentSurfaceId())
      }).width('100%')
        .height('100%')
      // ...
    }
  }
}
```

#### STEP-3: Setup your play source and config
```typescript
// Must call after player created
player.setDataSource({
  url: this.url, // url to play 
  useSoftDecoder: false, // If use soft decoder to play. Hard decoder is used by default
  // other play option
}).then(() => {
  // source data set finished
})
```
💡TIPS: See all player source and config defines in [RedPlayerDataSource](./redplayerproxy/src/main/ets/redplayer/core/RedPlayerDataSource.ts)

⚠️NOTICE: If using DevEco Studio Emulator to run RedPlayer, please force use soft-decoder. Hard-decoder is currently not supported in Emulator.

#### STEP-4: Prepare your player
```typescript
// Must call after source data set finished
player.prepare().then(() => {
  // player prepared
})
```

#### STEP-5: Start play
```typescript
// Must call after player prepared
player.start()
```

#### STEP-6: Other core player control APIs
```typescript
player.pause() // Pause player play. Resume play by call start after pause
player.stop() // Stop player play
player.release() // Release player
```
💡TIPS: See more player API references in [IMediaPlayer.ts](./redplayerproxy/src/main/ets/redplayer/core/interfaces/IMediaPlayer.ts)#interface IMediaPlayer

## A simple demo
Player control part:
```typescript
let listener: IMediaPlayerStateListener = new YourListenerImpl()
RedPlayerFactory.createMediaPlayer(listener).then((player) => {
  player.setDataSource({
    url: this.url, // url to play
  }).then(() => {
    // source data set finished
    player.prepare().then(() => {
      // player prepared
      player.start()
    })
  })
})
```

Player view part:
```typescript
@Entry
@Component
struct PlayerPage {
  // step1: create controller for XComponent
  xComponentController: RedPlayerXComponentController = new RedPlayerXComponentController()
  build() {
    Stack() {
      // ...
      // step2: add and config XComponent to your page for player render
      XComponent({
        id: this.xComponentController.getSurfaceId(),
        type: this.xComponentController.getSurfaceType(),
        libraryname: this.xComponentController.getLibraryName(),
        controller: this.xComponentController
      }).onLoad(() => {
        // step3: setSurfaceId to player after XComponent onLoad
        player.setSurfaceId(this.xComponentController._id)
        // if AVPlayer core:
        // this.playerController.setSurfaceId(this.xComponentController.getXComponentSurfaceId())
      }).width('100%')
        .height('100%')
      // ...
    }
  }
}
```

## How to use RedPlayer to preload
RedPlayer support preload, to prefetch url file ahead of player create by provided method.  
You can preload url file by following:
```typescript
let url = 'url to preload'
let cachePath = 'path to cache'
let cacheSize = 768 * 1024 // preload size
RedPlayerPreload.open(url, cachePath, cacheSize)
```