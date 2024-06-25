export class RedPlayerDataSource {
  /**
   * Video source url.
   */
  url?: string = ""

  /**
   * Video source local file fd.
   */
  fd?: number = 0

  /**
   * Video source type.
   * See @link{SourceType}
   */
  sourceType?: SourceType = SourceType.URL

  /**
   * Log tag.
   * Red player logs will contain log tag.
   */
  logHead?: string = ""

  /**
   * Set if need auto looping play.
   */
  isAutoLoop?: boolean = true

  /**
   * Set if need auto start play after prepare.
   * Only support in RedPlayer core, not support in AVPlayer core.
   */
  isAutoStart?: boolean = false

  /**
   * Set if enable accurate seek.
   * Only support in RedPlayer core, not support in AVPlayer core.
   */
  enableAccurateSeek?: boolean = false

  /**
   * Set if need start in specific position.
   * Only support in RedPlayer core, not support in AVPlayer core.
   */
  seekAtStart?: number = 0

  /**
   * Set if need force use soft decoder.
   * Only support in RedPlayer core, not support in AVPlayer core.
   */
  useSoftDecoder?: boolean = false

  /**
   * Set if play live stream.
   */
  isLive?: boolean = false
}

/**
 * Video source type
 */
export enum SourceType {
  /**
   * Video url.
   * Support network url or disk path.
   */
  URL,

  /**
   * Local file fd.
   */
  FD,
}