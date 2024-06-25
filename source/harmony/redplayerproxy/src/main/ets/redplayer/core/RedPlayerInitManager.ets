import {IRedPlayerLogger} from './log/IRedPlayerLogger';
import { NativeLoggerAdapter } from './log/NativeLoggerAdapter';
import RedPlayerLogger from './log/RedPlayerLogger';
import libnative from 'libredplayer.so'
import { PlayerNativeEventAdapter } from './implements/HmRedMediaPlayer';
import { GlobalConfig } from './implements/RedConfig';

/**
 * Init management of red player.
 */
export class RedPlayerInitManager {

  /**
   * Init global config of red player
   * Call once in a progress, before player use.
   */
  public static initPlayer(builder: RedPlayerConfigBuilder) {
    libnative.registerLogListener(NativeLoggerAdapter.getInstance())
    libnative.registerPlayerEventListener(PlayerNativeEventAdapter.getInstance())
    if (builder.cachePath) {
      GlobalConfig.cachePath = builder.cachePath
    }
    if (builder.logger) {
      RedPlayerLogger.loggerImpl = builder.logger
    }
  }
}

/**
 * Player global config options define.
 */
export class RedPlayerConfigBuilder {
  /**
   * Player video cache disk path.
   */
  cachePath: string | undefined

  /**
   * Custom define logger.
   */
  logger: IRedPlayerLogger | undefined
}