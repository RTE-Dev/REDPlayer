import AbilityStage from '@ohos.app.ability.AbilityStage'
import { RedPlayerConfigBuilder, RedPlayerInitManager, RedPlayerPreload } from '@xhs/redplayer'
import LoggerImpl from './entryability/LoggerImpl'

export default class DemoMainAbilityStage extends AbilityStage {
  onCreate() {
    let configBuilder = new RedPlayerConfigBuilder()
    configBuilder.cachePath = this.context.cacheDir + '/redplayer'
    configBuilder.logger = new LoggerImpl()
    RedPlayerInitManager.initPlayer(configBuilder)
    RedPlayerPreload.globalInit(this.context.cacheDir + '/redplayer', 20, 4, 0)
  }
}