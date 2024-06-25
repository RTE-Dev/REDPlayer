/**
 * Player log related.
 */
import BaseRedPlayerLoggerImpl from './BaseRedPlayerLoggerImpl'
import {IRedPlayerLogger} from './IRedPlayerLogger'

export default class RedPlayerLogger {
  static loggerImpl: IRedPlayerLogger = new BaseRedPlayerLoggerImpl()

  public static i(tag: string, msg?: string) {
    this.loggerImpl.i(tag, msg)
  }

  public static d(tag: string, msg?: string) {
    this.loggerImpl.d(tag, msg)
  }

  public static w(tag: string, msg?: string) {
    this.loggerImpl.w(tag, msg)
  }

  public static e(tag: string, msg?: string, error?: Error) {
    this.loggerImpl.e(tag, msg, error)
  }
}