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

#define LOG(level, component, ...)                                                   \
    do {                                                                             \
        if ((level) <= LOG_LEVEL_GLOBAL) {                                           \
            log_emit((LogLevel)(level), component, __VA_ARGS__);                     \
        }                                                                            \
    } while (0)

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_ERROR
#define LOG_ERROR(component, ...) log_emit(LOG_LEVEL_ERROR, component, __VA_ARGS__)
#else
#define LOG_ERROR(component, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_WARN
#define LOG_WARN(component, ...) log_emit(LOG_LEVEL_WARN, component, __VA_ARGS__)
#else
#define LOG_WARN(component, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_INFO
#define LOG_INFO(component, ...) log_emit(LOG_LEVEL_INFO, component, __VA_ARGS__)
#else
#define LOG_INFO(component, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(component, ...) log_emit(LOG_LEVEL_DEBUG, component, __VA_ARGS__)
#else
#define LOG_DEBUG(component, ...) ((void)0)
#endif

#if LOG_LEVEL_GLOBAL >= LOG_LEVEL_TRACE
#define LOG_TRACE(component, ...) log_emit(LOG_LEVEL_TRACE, component, __VA_ARGS__)
#else
#define LOG_TRACE(component, ...) ((void)0)
#endif

#endif // LOGGING_H
