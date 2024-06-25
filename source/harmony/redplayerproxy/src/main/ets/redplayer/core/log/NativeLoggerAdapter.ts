import RedPlayerLogger from './RedPlayerLogger';

/**
 * Player core native log callback impl.
 */
export class NativeLoggerAdapter {
  private constructor() {
  }

  private static instance: NativeLoggerAdapter;
  private _logListener: LogsListener = new LogsListener();

  public static getInstance(): NativeLoggerAdapter {
    if (!NativeLoggerAdapter.instance) {
      NativeLoggerAdapter.instance = new NativeLoggerAdapter();
    }
    return NativeLoggerAdapter.instance;
  }

  getLogsListener(): LogsListener | undefined {
    return this._logListener;
  }

  setLogsListener(value: LogsListener): void {
    this._logListener = value;
  }
}

/**
 * Player core native log instance to call.
 */
export class LogsListener implements OnLogsListener {
  public constructor() {}

  onLogs(level: LogLevel, tag: string, message: string): void {
    switch (level) {
      case LogLevel.RED_LOG_DEFAULT:
      case LogLevel.RED_LOG_VERBOSE:
      case LogLevel.RED_LOG_DEBUG:
        RedPlayerLogger.d(tag, message)
        break;
      case LogLevel.RED_LOG_INFO:
        RedPlayerLogger.i(tag, message)
        break;
      case LogLevel.RED_LOG_WARN:
        RedPlayerLogger.w(tag, message)
        break;
      case LogLevel.RED_LOG_ERROR:
      case LogLevel.RED_LOG_FATAL:
        RedPlayerLogger.e(tag, message)
        break;
      default:
        break;
    }
  }
}

/**
 * Native log level define.
 */
enum LogLevel {
  RED_LOG_UNKNOWN,
  RED_LOG_DEFAULT,
  RED_LOG_VERBOSE,
  RED_LOG_DEBUG,
  RED_LOG_INFO,
  RED_LOG_WARN,
  RED_LOG_ERROR,
  RED_LOG_FATAL,
  RED_LOG_SILENT
}

export default interface OnLogsListener {
  onLogs(level: number, tag: string, message: string): void;
}