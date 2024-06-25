import { PlayerNativeEventAdapter } from "../../../ets/redplayer/core/implements/HmRedMediaPlayer"
import { NativeLoggerAdapter } from "../../../ets/redplayer/core/log/NativeLoggerAdapter"
import { RedPlayerPreloadAdapter } from "../../../ets/redplayer/preload/RedPlayerPreload"

// ==================================
// player control function part start
export const createPlayer: (playerId: string) => void
export const setSurfaceId: (playerId: string, id: string) => void
export const setDataSource: (playerId: string, url: string) => void
export const setDataSourceFd: (playerId: string, fd: number) => void
export const setConfigStr: (playerId: string, type: number, name: string, value: string) => void
export const setConfigInt: (playerId: string, type: number, name: string, value: number) => void
export const prepare: (playerId: string) => void
export const seek: (playerId: string, timeMs: number) => void
export const setSpeed: (playerId: string, speed: number) => void
export const setVolume: (playerId: string, volume: number) => void
export const setLoop: (playerId: string, loop: number) => void
export const start: (playerId: string) => void
export const pause: (playerId: string) => void
export const stop: (playerId: string) => void
export const release: (playerId: string) => void
export const reset: (playerId: string) => void
// player control function part end
// ==================================

// ==================================
// player get function part start
export const getCurrentPosition: (playerId: string) => number
export const getVideoDuration: (playerId: string) => number
export const getVideoWidth: (playerId: string) => number
export const getVideoHeight: (playerId: string) => number
export const getUrl: (playerId: string) => string
export const getAudioCodecInfo: (playerId: string) => string
export const getVideoCodecInfo: (playerId: string) => string
export const isPlaying: (playerId: string) => boolean
export const isLooping: (playerId: string) => boolean
export const getVideoCachedSizeBytes: (playerId: string) => number
export const getAudioCachedSizeBytes: (playerId: string) => number
export const getVideoCachedSizeMs: (playerId: string) => number
export const getAudioCachedSizeMs: (playerId: string) => number
// player get function part end
// ==================================

// ==================================
// player callback part start
export const registerPlayerEventListener: (adapter: PlayerNativeEventAdapter) => void
export const registerLogListener: (adapter: NativeLoggerAdapter) => void
// player callback part end
// ==================================

// ==================================
// player preload part start
export const preloadGlobalInit: (path: string, maxSize: number, threadPoolSize: number, maxDirSize: number) => void
export const preloadRegisterCallback: (adapter: RedPlayerPreloadAdapter) => void
export const preloadOpen: (instanceId: string, url: string, path: string, downsize: number) => void
export const preloadOpenJson: (instanceId: string, urlJson: string, path: string, downsize: number) => void
export const preloadRelease: (url: string) => void
export const preloadStop: () => void
// player preload part end
// ==================================