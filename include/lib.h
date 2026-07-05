// clang-format Language: C

#ifndef LIB_H
#define LIB_H

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h> // IWYU pragma: keep
#include <string.h>
#include <time.h>

// =============================================================================
// Memory
// =============================================================================

#define KB_TO_BYTES(_pr_v) ((_pr_v) * 1024)
#define MB_TO_BYTES(_pr_v) (KB_TO_BYTES(_pr_v) * 1024)
#define GB_TO_BYTES(_pr_v) (MB_TO_BYTES(_pr_v) * 1024)
#define TB_TO_BYTES(_pr_v) (GB_TO_BYTES(_pr_v) * 1024)

#define ARENA_PUSH_ARRAY(arena, type, count) (type *)arena_push((arena), sizeof(type) * (count))
#define ARENA_PUSH_STRUCT(arena, type) (type *)arena_push((arena), sizeof(type))

#define ARENA_PUSH_ARRAY_ZERO(arena, type, count) (type *)arena_push_zero((arena), sizeof(type) * (count))
#define ARENA_PUSH_STRUCT_ZERO(arena, type) (type *)arena_push_zero((arena), sizeof(type))

typedef struct Arena {
	size_t capacity_bytes;
	unsigned char *base_address;
	size_t used_bytes;
} Arena;

void arena_init(Arena *restrict arena, const size_t size_bytes, unsigned char *const restrict base)
{
	arena->capacity_bytes = size_bytes;
	arena->base_address = base;
	arena->used_bytes = 0;
}

void *arena_push(Arena *arena, size_t size_bytes)
{
	assert(arena->used_bytes + size_bytes <= arena->capacity_bytes);

	void *result = arena->base_address + arena->used_bytes;
	arena->used_bytes += size_bytes;

	return result;
}

void *arena_push_zero(Arena *arena, size_t size_bytes)
{
	void *result = arena_push(arena, size_bytes);

	memset(result, 0, size_bytes);

	return nullptr;
}

// =============================================================================
// String
// =============================================================================

#define STRINGIFY(n) #n
#define XSTRINGIFY(n) STRINGIFY(n)

// =============================================================================
// Logging
// =============================================================================

#define LOG_TSTAMP_BUF_SIZE 32
#define LOG_LEVEL_ALL 0UL
#define LOG_LEVEL_TRACE 1UL
#define LOG_LEVEL_DEBUG 2UL
#define LOG_LEVEL_INFO 3UL
#define LOG_LEVEL_WARN 4UL
#define LOG_LEVEL_ERROR 5UL
#define LOG_LEVEL_FATAL 6UL
#define LOG_LEVEL_OFF 7UL

// Defines what is the minimum priority of a message to be logged.
// Anything with higher priority is going to be logged.
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL
#endif // LOG_LEVEL

#if DEBUG
#define IMPL_LOG_WRITE(fmt, ...)                                                     \
	do {                                                                         \
		FILE *_pr_log_file = fopen("log.txt", "a+");                         \
		if (_pr_log_file != nullptr) {                                       \
			(void)fprintf(_pr_log_file, fmt __VA_OPT__(, ) __VA_ARGS__); \
			(void)fclose(_pr_log_file);                                  \
		}                                                                    \
	} while (false)
#else
#define IMPL_LOG_WRITE(fmt, ...) printf(fmt __VA_OPT__(, ) __VA_ARGS__)
#endif

#define IMPL_LOG_MSG(log_level, fmt, file_name, func_name, line_number, ...)                                   \
	do {                                                                                                   \
		char _pr_tstamp_str[LOG_TSTAMP_BUF_SIZE];                                                      \
		struct timespec _pr_ts;                                                                        \
		struct tm _pr_tm;                                                                              \
                                                                                                               \
		if (!timespec_get(&_pr_ts, TIME_UTC)) {                                                        \
			break;                                                                                 \
		}                                                                                              \
		if (gmtime_s(&_pr_tm, &_pr_ts.tv_sec)) {                                                       \
			break;                                                                                 \
		}                                                                                              \
		if (strftime(_pr_tstamp_str, LOG_TSTAMP_BUF_SIZE, "%FT%T", &_pr_tm) == 0) {                    \
			break;                                                                                 \
		}                                                                                              \
                                                                                                               \
		IMPL_LOG_WRITE("%c[%s.%09ldZ] %s:%s:%s: " fmt "\n", log_level, _pr_tstamp_str, _pr_ts.tv_nsec, \
		               file_name, func_name, XSTRINGIFY(line_number) __VA_OPT__(, ) __VA_ARGS__);      \
	} while (false)

#define IMPL_LOG_MSG_NOOP(...) ((void)0)

// Logs a trace message if LOG_LEVEL <= LOG_LEVEL_TRACE
// Usage: LOG_TRACE("Log trace: x=%d", x);
#if LOG_LEVEL <= LOG_LEVEL_TRACE
#define LOG_TRACE(fmt, ...) IMPL_LOG_MSG('T', fmt, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...) IMPL_LOG_MSG_NOOP()
#endif // LOG_TRACE

// Logs a debug message if LOG_LEVEL <= LOG_LEVEL_DEBUG.
// Usage: LOG_DEBUG("log debug: x=%d", x);
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) IMPL_LOG_MSG('D', fmt, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) IMPL_LOG_MSG_NOOP()
#endif // LOG_DEBUG

// Logs an information message if LOG_LEVEL <= LOG_LEVEL_INFO
// Usage: LOG_INFO("Log info: x=%d", x);
#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) IMPL_LOG_MSG('I', fmt, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) IMPL_LOG_MSG_NOOP()
#endif // LOG_INFO

// Logs a warning message if LOG_LEVEL <= LOG_LEVEL_WARN
// Usage: LOG_WARN("Log warn: x=%d", x);
#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) IMPL_LOG_MSG('W', fmt, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) IMPL_LOG_MSG_NOOP()
#endif // LOG_WARN

// Logs an error message if LOG_LEVEL <= LOG_LEVEL_ERROR
// Usage: LOG_ERROR("Log error: x=%d", x);
#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) IMPL_LOG_MSG('E', fmt, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) IMPL_LOG_MSG_NOOP()
#endif // LOG_ERROR

// Logs a fatal message if LOG_LEVEL <= LOG_LEVEL_FATAL
// Usage: LOG_FATAL("Log fatal: x=%d", x);
#if LOG_LEVEL <= LOG_LEVEL_FATAL
#define LOG_FATAL(fmt, ...) IMPL_LOG_MSG('F', fmt, __FILE__, __func__, __LINE__, __VA_ARGS__)
#else
#define LOG_FATAL(fmt, ...) IMPL_LOG_MSG_NOOP()
#endif // LOG_FATAL

#endif // LIB_H
