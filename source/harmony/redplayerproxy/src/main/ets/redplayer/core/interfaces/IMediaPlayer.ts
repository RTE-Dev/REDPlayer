import { RedPlayerDataSource } from '../RedPlayerDataSource';
import { RedPlayerTrackModel } from '../track/RedPlayerTrackModel';
import { PlayerState } from './IPlayerState';

/**
 * Red player player API defines.
 */
export interface IMediaPlayer extends IMediaPlayerControl, IMediaPlayerParamGetter {
  // nothing
}

/**
 * Part of player control.
 */
export interface IMediaPlayerControl {
  /**
   * Create player instance.
   */
  createPlayer(): Promise<void>

  /**
   * Config render surface of player.
   */
  setSurfaceId(id: string): void

  /**
   * Config media source info to play.
   * Must call after {@link IMediaPlayerStateListener.onPlayerCreated}
   */
  setDataSource(dataSource: RedPlayerDataSource): Promise<void>

  /**
   * Config options of player.
   * Must call before {@link IMediaPlayerControl.prepare}
   */
  setConfigStr(type: number, name: string, value: string)
  setConfigInt(type: number, name: string, value: number)

  /**
   * Do player prepare.
   * Must call after {@link IMediaPlayerControl.setDataSource}
   */
  prepare(): Promise<void>

  /**
   * Seek player to target time position.
   * @param timeMs: position to seek, in ms
   */
  seek(timeMs: number): void

  /**
   * Config play speed multiple.
   * Default in 1.0
   */
  setSpeed(speed: number)

  /**
   * Config player volume.
   * Default in 1.0
   * Range：0 - 1
   */
  setVolume(volume: number)

  /**
   * Sets the player to be looping or non-looping.
   */
  setLoop(loop: boolean)

  /**
   * Start player to play
   * Must call after {@link IMediaPlayerStateListener.onPrepared}
   */
  start(): Promise<void>

  /**
   * Pause player.
   */
  pause(): Promise<void>

  /**
   * Stop player.
   */
  stop(): Promise<void>

  /**
   * Release player.
   */
  release(): Promise<void>

  /**
   * Reset player to idle state.
   */
  reset(): Promise<void>
}

export interface IMediaPlayerParamGetter {
  /**
   * Get current player state.
   */
  getPlayerState(): PlayerState

  /**
   * Get current playing position, in ms.
   */
  getCurrentPosition(): number

  /**
   * Get video total duration, in ms.
   */
  getVideoDuration(): number

  /**
   * Get player log tag.
   * Red player logs will contains it.
   */
  getLogHead(): string

  /**
   * Get video width, in px.
   * Must call after {@link IMediaPlayerStateListener.onPrepared}
   */
  getVideoWidth(): number

  /**
   * Get video height, in px.
   * Must call after {@link IMediaPlayerStateListener.onPrepared}
   */
  getVideoHeight(): number

  /**
   * Get current player unique id.
   */
  getPlayerId(): string

  /**
   * Get current playing video url.
   * Must call after {@link IMediaPlayerStateListener.onPrepared}
   */
  getUrl(): string

  /**
   * Get player track info.
   */
  getTrackModel(): RedPlayerTrackModel

  /**
   * If player is in playing state.
   */
  isPlaying(): boolean

  /**
   * If player is in looping state.
   */
  isLooping(): boolean

  /**
   * Video in playing download cached size, in byte.
   * Not support if use system avplayer
   */
  getVideoCachedSizeBytes(): number

  /**
   * Audio in playing download cached size, in byte.
   * Not support if use system avplayer.
   */
  getAudioCachedSizeBytes(): number

  /**
   * Video in playing download cached duration, in ms.
   * Not support if use system avplayer.
   */
  getVideoCachedSizeMs(): number

  /**
   * Audio in playing download cached duration, in ms.
   * Not support if use system avplayer.
   */
  getAudioCachedSizeMs(): number

  /**
   * Return video codec info.
   * Not support if use system avplayer.
   */
  getVideoCodecInfo(): string

  /**
   * Return audio codec info.
   * Not support if use system avplayer.
   */
  getAudioCodecInfo(): string
}