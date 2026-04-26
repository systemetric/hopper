#ifndef logging_hpp_INCLUDED
#define logging_hpp_INCLUDED

#include <iostream>
#include <unistd.h>

#if __has_include(<unistd.h>)
#include <unistd.h>
inline bool is_tty() { return isatty(STDOUT_FILENO); }
#else
inline bool is_tty() { return false; }
#endif

namespace hopper {

enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
};

#define _HOPPER_TRACE_STR "TRACE   "
#define _HOPPER_DEBUG_STR "DEBUG   "
#define _HOPPER_INFO_STR "INFO    "
#define _HOPPER_WARN_STR "WARN    "
#define _HOPPER_ERROR_STR "ERROR   "

inline const char *_HOPPER_LEVEL_STR[] = {
    _HOPPER_TRACE_STR,
    _HOPPER_DEBUG_STR,
    _HOPPER_INFO_STR,
    _HOPPER_WARN_STR,
    _HOPPER_ERROR_STR,
    "\x1b[1;35m" _HOPPER_TRACE_STR "\x1b[0m",
    "\x1b[1;34m" _HOPPER_DEBUG_STR "\x1b[0m",
    "\x1b[1;32m" _HOPPER_INFO_STR "\x1b[0m",
    "\x1b[1;33m" _HOPPER_WARN_STR "\x1b[0m",
    "\x1b[1;31m" _HOPPER_ERROR_STR "\x1b[0m"};

class Logger {
private:
    LogLevel m_level;
    bool m_has_color;

    template <typename... Args> void log(LogLevel level, Args &&...args) {
        if (level < m_level)
            return;

        std::cout << _HOPPER_LEVEL_STR[static_cast<int>(level) +
                                       (m_has_color ? 5 : 0)];
        (std::cout << ... << args) << '\n';
    }

public:
    Logger(LogLevel level) : m_level(level), m_has_color(is_tty()) {};

    template <typename... Args> void trace(Args &&...args) {
        log(LogLevel::Trace, std::forward<Args>(args)...);
    }

    template <typename... Args> void debug(Args &&...args) {
        log(LogLevel::Debug, std::forward<Args>(args)...);
    }

    template <typename... Args> void info(Args &&...args) {
        log(LogLevel::Info, std::forward<Args>(args)...);
    }

    template <typename... Args> void warn(Args &&...args) {
        log(LogLevel::Warn, std::forward<Args>(args)...);
    }

    template <typename... Args> void error(Args &&...args) {
        log(LogLevel::Error, std::forward<Args>(args)...);
    }
};

} // namespace hopper

#endif // logging_hpp_INCLUDED
