import util from '@ohos.util'
import netCacheNative from 'libredplayer.so'

/**
 * Red player preload.
 */
export class RedPlayerPreload {

  /**
   * Init preload module before using.
   * Call once in a progress.
   *
   * @param path: preload cache path.
   * @param maxSize: max size of preload tasks.
   * @param threadPoolSize: thread pool size of preload tasks.
   * @param maxDirSize: max disk size of preload caches.
   */
  static globalInit(path: string, maxSize: number, threadPoolSize: number, maxDirSize: number) {
    netCacheNative.preloadGlobalInit(path, maxSize, threadPoolSize, maxDirSize)
    netCacheNative.preloadRegisterCallback(RedPlayerPreloadAdapter.getInstance())
  }

  static open(url: string, path: string, downsize: number): string {
    let instanceId = util.generateRandomUUID()
    netCacheNative.preloadOpen(instanceId, url, path, downsize)
    return instanceId
  }

  static release(url: string) {
    netCacheNative.preloadRelease(url)
  }

  static openJson(urlJson: string, path: string, downsize: number): string {
    let instanceId = util.generateRandomUUID()
    netCacheNative.preloadOpenJson(instanceId, urlJson, path, downsize)
    return instanceId
  }

  static stop() {
    netCacheNative.preloadStop()
  }
}

export class RedPlayerPreloadAdapter {
  private constructor() {
  }

  private static instance: RedPlayerPreloadAdapter;
  private _listener: RedPlayerPreloadListener = new RedPlayerPreloadListener();

  public static getInstance(): RedPlayerPreloadAdapter {
    if (!RedPlayerPreloadAdapter.instance) {
      RedPlayerPreloadAdapter.instance = new RedPlayerPreloadAdapter();
    }
    return RedPlayerPreloadAdapter.instance;
  }

  getListener(): RedPlayerPreloadListener | undefined {
    return this._listener;
  }
}

export class RedPlayerPreloadListener implements IRedPlayerPreloadListener {
  public constructor() {}

  onNativeEvent(instanceId: string, event: number, url: string): void {

  }
}

export default interface IRedPlayerPreloadListener {
  onNativeEvent(instanceId: string, event: number, url: string): void;
}