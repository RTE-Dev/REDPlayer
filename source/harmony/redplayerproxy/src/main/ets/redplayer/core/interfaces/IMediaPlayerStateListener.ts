export interface IMediaPlayerStateListener {
  /**
   * Player instance created.
   */
  onPlayerCreated(): void;

  /**
   * Player source config finished.
   */
  onInitialized(): void;

  /**
   * Player prepared.
   */
  onPrepared(): void;

  /**
   * Player called to start play.
   */
  onStarted(): void;

  /**
   * Player video first frame rendered.
   */
  onFirstFrameRendered(): void;

  /**
   * Player playing position update.
   */
  onPositionUpdated(time: number): void;

  /**
   * Player paused.
   */
  onPaused(): void;

  /**
   * Player stopped.
   */
  onStopped(): void;

  /**
   * Player play completed.
   */
  onCompleted(): void;

  /**
   * Player released.
   */
  onReleased(): void;

  /**
   * Player error occurs.
   */
  onError(reason: string): void

  /**
   * Player seek finished.
   */
  onSeekDone(): void

  /**
   * Player start in buffering state.
   */
  onBufferingStart(): void

  /**
   * Player buffering state end.
   */
  onBufferingEnd(): void
}