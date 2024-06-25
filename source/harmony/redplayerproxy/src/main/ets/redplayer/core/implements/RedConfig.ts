export enum RedConfigType {
  TYPE_FORMAT  = 1,
  TYPE_CODEC   = 2,
  TYPE_SWS     = 3,
  TYPE_PLAYER  = 4,
  TYPE_SWR     = 5
}

export class GlobalConfig {
  static cachePath: string | undefined
}