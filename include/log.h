#define LOG_LEVEL_ALL INT_MIN
#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARN 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_FATAL 5
#define LOG_LEVEL_OFF INT_MAX

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define logt(...)                                                              \
  do {                                                                         \
    if (LOG_LEVEL <= LOG_LEVEL_TRACE)                                          \
      printf("[TRACE] " __VA_ARGS__);                                          \
  } while (0)

/**
 * logd - Logs a debug message if LOG_LEVEL <= LOG_LEVEL_DEBUG.
 * Usage: logd("Debug info: x=%d\n", x);
 */
#define logd(...)                                                              \
  do {                                                                         \
    if (LOG_LEVEL <= LOG_LEVEL_DEBUG)                                          \
      printf("[DEBUG] " __VA_ARGS__);                                          \
  } while (0)

#define logi(...)                                                              \
  do {                                                                         \
    if (LOG_LEVEL <= LOG_LEVEL_INFO)                                           \
      printf("[INFO] " __VA_ARGS__);                                           \
  } while (0)

#define logw(...)                                                              \
  do {                                                                         \
    if (LOG_LEVEL <= LOG_LEVEL_WARN)                                           \
      printf("[WARN] " __VA_ARGS__);                                           \
  } while (0)

#define loge(...)                                                              \
  do {                                                                         \
    if (LOG_LEVEL <= LOG_LEVEL_ERROR)                                          \
      printf("[ERROR] " __VA_ARGS__);                                          \
  } while (0)
#define logf(...)                                                              \
  do {                                                                         \
    if (LOG_LEVEL <= LOG_LEVEL_FATAL)                                          \
      printf("[FATAL] " __VA_ARGS__);                                          \
  } while (0)
