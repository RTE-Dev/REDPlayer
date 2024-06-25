import { util } from '@kit.ArkTS'
import { PlayerCoreType } from '../core/RedPlayerFactory'

/**
 * Red player XComponent wrapper.
 */
export class RedPlayerXComponentController extends XComponentController {
  private readonly _id: string = util.generateRandomUUID()
  private coreType: PlayerCoreType = PlayerCoreType.TYPE_RED_PLAYER

  constructor(coreType?: PlayerCoreType) {
    super()
    if (coreType) {
      this.coreType = coreType
    }
  }

  getSurfaceId(): string {
    if (this.coreType == PlayerCoreType.TYPE_RED_PLAYER) {
      return this._id
    }
    return this.getXComponentSurfaceId()
  }

  getSurfaceType(): string {
    return 'surface'
  }

  getLibraryName(): string | null {
    if (this.coreType == PlayerCoreType.TYPE_RED_PLAYER) {
      return 'redplayerproxy'
    }
    return null
  }
}
