/**
 * Interface define of player logger.
 */
export interface IRedPlayerLogger {
  i(tag: string, msg?: string): void
  d(tag: string, msg?: string): void
  w(tag: string, msg?: string): void
  e(tag: string, msg?: string, error?: Error): void
}