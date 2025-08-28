// clang-format Language: C

#ifndef LOG_H
#define LOG_H

#define LOG_LEVEL_ALL INT_MIN
#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_FATAL 5
#define LOG_LEVEL_OFF INT_MAX

// Defines what is the minimum priority of a message to be logged.
// Anything with higher priority is going to be logged.
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Logs a trace message if LOG_LEVEL <= LOG_LEVEL_TRACE
#define logt(...)                                                                                                                                                                  \
    do {                                                                                                                                                                           \
        if (LOG_LEVEL <= LOG_LEVEL_TRACE)                                                                                                                                          \
            printf("[TRACE] " __VA_ARGS__);                                                                                                                                        \
    } while (0)

// Logs a debug message if LOG_LEVEL <= LOG_LEVEL_DEBUG.
// Usage: logd("Debug info: x=%d\n", x);
#define logd(...)                                                                                                                                                                  \
    do {                                                                                                                                                                           \
        if (LOG_LEVEL <= LOG_LEVEL_DEBUG)                                                                                                                                          \
            printf("[DEBUG] " __VA_ARGS__);                                                                                                                                        \
    } while (0)

// Logs an information message if LOG_LEVEL <= LOG_LEVEL_INFO
#define logi(...)                                                                                                                                                                  \
    do {                                                                                                                                                                           \
        if (LOG_LEVEL <= LOG_LEVEL_INFO)                                                                                                                                           \
            printf("[INFO] " __VA_ARGS__);                                                                                                                                         \
    } while (0)

// Logs a warning message if LOG_LEVEL <= LOG_LEVEL_WARN
#define logw(...)                                                                                                                                                                  \
    do {                                                                                                                                                                           \
        if (LOG_LEVEL <= LOG_LEVEL_WARN)                                                                                                                                           \
            printf("[WARN] " __VA_ARGS__);                                                                                                                                         \
    } while (0)

// Logs an error message if LOG_LEVEL <= LOG_LEVEL_ERROR
#define loge(...)                                                                                                                                                                  \
    do {                                                                                                                                                                           \
        if (LOG_LEVEL <= LOG_LEVEL_ERROR)                                                                                                                                          \
            printf("[ERROR] " __VA_ARGS__);                                                                                                                                        \
    } while (0)

// Logs a fatal message if LOG_LEVEL <= LOG_LEVEL_FATAL
#define logf(...)                                                                                                                                                                  \
    do {                                                                                                                                                                           \
        if (LOG_LEVEL <= LOG_LEVEL_FATAL)                                                                                                                                          \
            printf("[FATAL] " __VA_ARGS__);                                                                                                                                        \
    } while (0)

#endif
