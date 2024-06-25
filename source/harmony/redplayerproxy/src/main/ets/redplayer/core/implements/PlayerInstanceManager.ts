import { IMediaPlayer } from '../interfaces/IMediaPlayer'

export class PlayerInstanceManager {
  private static players = new Map()

  static onPlayerCreate(player: IMediaPlayer) {
    PlayerInstanceManager.players.set(player.getPlayerId(), player)
  }

  static onPlayerRelease(player: IMediaPlayer) {
    PlayerInstanceManager.players.delete(player.getPlayerId())
  }

  static findPlayerById(playerId: string): IMediaPlayer {
    let result = undefined
    PlayerInstanceManager.players.forEach((value, key) => {
      if (this.compareStrings(key, playerId)) {
        result =  value
      }
    })
    return result
  }

  // avoid native string length not match with ts
  static compareStrings(str1: string, str2: string): boolean {
    for (let i = 0; i < str1.length; i++) {
      if (str1[i] !== str2[i]) {
        return false;
      }
    }
    return true;
  }
}