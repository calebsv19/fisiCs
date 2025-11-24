#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>
#include <stdio.h>

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN  = 1,
    LOG_LEVEL_INFO  = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_TRACE = 4
} LogLevel;

#ifndef LOG_LEVEL_GLOBAL
#define LOG_LEVEL_GLOBAL LOG_LEVEL_INFO
#endif

static inline const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_TRACE: return "TRACE";
        default:              return "LOG";
    }
}

static inline void log_emit(LogLevel level, const char* component, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const char* comp = component ? component : "core";
    fprintf(stderr, "[%s][%s] ", log_level_to_string(level), comp);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

#define LOG(level, component, fmt, ...)                                              \
    do {                                                                             \
        if ((level) <= LOG_LEVEL_GLOBAL) {                                           \
            log_emit((LogLevel)(level), component, fmt, ##__VA_ARGS__);              \
        }                                                                            \
    } while (0)

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_ERROR
#define LOG_ERROR(component, fmt, ...) log_emit(LOG_LEVEL_ERROR, component, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(component, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_WARN
#define LOG_WARN(component, fmt, ...) log_emit(LOG_LEVEL_WARN, component, fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(component, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_INFO
#define LOG_INFO(component, fmt, ...) log_emit(LOG_LEVEL_INFO, component, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(component, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(component, fmt, ...) log_emit(LOG_LEVEL_DEBUG, component, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(component, fmt, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_TRACE
#define LOG_TRACE(component, fmt, ...) log_emit(LOG_LEVEL_TRACE, component, fmt, ##__VA_ARGS__)
#else
#define LOG_TRACE(component, fmt, ...) ((void)0)
#endif

#endif // LOGGING_H
