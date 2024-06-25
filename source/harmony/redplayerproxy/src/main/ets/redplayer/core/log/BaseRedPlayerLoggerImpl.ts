import {IRedPlayerLogger} from './IRedPlayerLogger';

/**
 * Default implement of player logger.
 */
export default class BaseRedPlayerLoggerImpl implements IRedPlayerLogger {
  i(tag: string, msg?: string) {
    console.info(tag, msg)
  }

  d(tag: string, msg?: string) {
    console.debug(tag, msg)
  }

  w(tag: string, msg?: string) {
    console.warn(tag, msg)
  }

  e(tag: string, msg?: string, error?: Error) {
    console.error(tag, msg, error)
  }
}