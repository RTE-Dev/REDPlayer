import HmAVMediaPlayer from './implements/HmAVMediaPlayer'
import HmRedMediaPlayer from './implements/HmRedMediaPlayer'
import {IMediaPlayer} from './interfaces/IMediaPlayer'
import {IMediaPlayerStateListener} from './interfaces/IMediaPlayerStateListener'

/**
 * Red player instance factory.
 */
export class RedPlayerFactory {

  /**
   * Create a new player instance.
   * @param listener: player callback impl.
   * @param coreType: player core type. Recommend use red player, or switch to AVPlayer core in specific use.
   * @returns: player instance
   */
  public static createMediaPlayer(
    listener?: IMediaPlayerStateListener,
    coreType?: PlayerCoreType,
  ): Promise<IMediaPlayer> {
    if (coreType == PlayerCoreType.TYPE_AV_PLAYER) {
      let player = new HmAVMediaPlayer(listener)
      return new Promise<IMediaPlayer>((resolve, reject) => {
        player.createPlayer().then(() => {
          resolve(player)
        }).catch((error) => {
          reject(error)
        })
      })
    } else {
      let player = new HmRedMediaPlayer(listener)
      return new Promise<IMediaPlayer>((resolve, reject) => {
        player.createPlayer().then(() => {
          resolve(player)
        }).catch((error) => {
          reject(error)
        })
      })
    }
  }
}

export enum PlayerCoreType {
  TYPE_AV_PLAYER = 1, // Harmony system AVPlayer core
  TYPE_RED_PLAYER = 2, // Red player core.
}
