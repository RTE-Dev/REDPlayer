/**
 * Player APM track info.
 */
export class RedPlayerTrackModel {
  /**
   * Player create call time.
   */
  createPlayerTime: number = 0

  /**
   * Player created time.
   */
  onPlayerCreatedTime: number = 0

  /**
   * Player prepare called time.
   */
  prepareCalledTime: number = 0

  /**
   * Player prepared time.
   */
  onPrepareTime: number = 0

  /**
   * Player first start called time
   */
  startCalledTime: number = 0

  /**
   * Player start render time.
   */
  renderStart: number = 0

  /**
   * Player start on play time.
   */
  startOnPlaying: number = 0

  /**
   * Player release called time.
   */
  releaseCalledTime: number = 0
}