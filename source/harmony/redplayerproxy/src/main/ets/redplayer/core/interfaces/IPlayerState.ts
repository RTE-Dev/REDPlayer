export enum PlayerState {
  STATE_UNDEFINED, // unknown state
  STATE_ERROR, // playe in error
  STATE_IDLE, // idle state
  STATE_RELEASED, // player is released
  STATE_INITIALIZED, // player init
  STATE_PREPARING, // player is preparing
  STATE_PREPARED, // prepare finished
  STATE_PLAYING, // player in playing
  STATE_PAUSED, // player paused
  STATE_STOPPED, // player stopped
  STATE_COMPLETED, // player source play completed
}

export function getStateString(state: PlayerState): string {
  switch (state) {
    case PlayerState.STATE_UNDEFINED:
      return "STATE_UNDEFINED"
    case PlayerState.STATE_ERROR:
      return "STATE_ERROR"
    case PlayerState.STATE_IDLE:
      return "STATE_IDLE"
    case PlayerState.STATE_RELEASED:
      return "STATE_RELEASED"
    case PlayerState.STATE_INITIALIZED:
      return "STATE_INITIALIZED"
    case PlayerState.STATE_PREPARING:
      return "STATE_PREPARING"
    case PlayerState.STATE_PREPARED:
      return "STATE_PREPARED"
    case PlayerState.STATE_PLAYING:
      return "STATE_PLAYING"
    case PlayerState.STATE_PAUSED:
      return "STATE_PAUSED"
    case PlayerState.STATE_STOPPED:
      return "STATE_STOPPED"
    case PlayerState.STATE_COMPLETED:
      return "STATE_COMPLETED"
  }
}
