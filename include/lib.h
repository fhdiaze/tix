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
// Compiler detection
// =============================================================================

#ifndef LIB_COMPILER_MSVC
#define LIB_COMPILER_MSVC 0
#endif

#ifndef LIB_COMPILER_LLVM
#define LIB_COMPILER_LLVM 0
#endif

#if !LIB_COMPILER_MSVC && !LIB_COMPILER_LLVM
#ifdef _MSC_VER
#undef LIB_COMPILER_MSVC
#define LIB_COMPILER_MSVC 1
#else
#undef LIB_COMPILER_LLVM
#define LIB_COMPILER_LLVM 1
#endif
#endif

#if LIB_COMPILER_MSVC
#include <intrin.h>
#endif

// =============================================================================
// Standard
// =============================================================================

#if DEBUG
#if LIB_COMPILER_MSVC
#define ASSERT(cond)        \
	do {                    \
		if (!(cond)) {      \
			__debugbreak(); \
		}                   \
	} while (0)
#endif
#else
#define ASSERT(cond) (void;)
#endif

// =============================================================================
// Bit operations
// =============================================================================

/**
 * @brief Count trailing zeroes - returns the index of the first least-significant non-zero bit if there is one.
 *
 * @param value The value to be checked
 * @param[out] count Set to the number of trailing zeroes when a non-zero bit is found; left untouched otherwise.
 * @return 1 if a non-zero bit was found, otherwise 0
 */
uint32_t uint_ctz(uint32_t value, uint32_t *count)
{
	uint32_t was_found = 0U;

#if LIB_COMPILER_MSVC
	unsigned long ctz_tmp = 0;
	was_found = _BitScanForward(&ctz_tmp, value);
	*count = (uint32_t)ctz_tmp;
#elif LIB_COMPILER_LLVM
	if (value != 0U) {
		was_found = 1U;
		*count = (uint32_t)__builtin_ctz(value);
	}
#else
	for (unsigned test = 0; test < 32; ++test) {
		if (value & (1U << test)) {
			was_found = 1U;
			*count = test;

			break;
		}
	}
#endif

	return was_found;
}

// =============================================================================
// Math
// =============================================================================

#define IS_POWER_OF_TWO(v) (((v) & ((v) - 1)) == 0)

typedef union Vtwo {
	struct {
		float x;
		float y;
	};
	float e[2];
} Vtwo;

/**
 * @brief Flips a vector in the x axis (inverts its x coordinate)
 *
 * @param a The vector to flip
 * @return Vtwo The flipped vector (-a.x, a.y)
 *
 * @example
 * Vtwo v = {3.0f, 4.0f};
 * Vtwo flipped = vtwo_flip_x(v);  // {-3.0f, 4.0f}
 */
inline Vtwo vtwo_flip_x(Vtwo a)
{
	Vtwo result = {
		.x = a.x,
		.y = -a.y,
	};

	return result;
}

/**
 * @brief Flips a vector in the y axis (inverts its y coordinate)
 *
 * @param a The vector to flip
 * @return Vtwo The flipped vector (a.x, -a.y)
 *
 * @example
 * Vtwo v = {3.0f, 4.0f};
 * Vtwo flipped = vtwo_flip_y(v);  // {3.0f, -4.0f}
 */
inline Vtwo vtwo_flip_y(Vtwo a)
{
	Vtwo result = {
		.x = a.x,
		.y = -a.y,
	};

	return result;
}

/**
 * @brief Negates a vector (inverts its direction)
 *
 * @param a The vector to negate
 * @return Vtwo The negated vector (-a.x, -a.y)
 *
 * @example
 * Vtwo v = {3.0f, 4.0f};
 * Vtwo neg = vtwo_inv(v);  // {-3.0f, -4.0f}
 */
inline Vtwo vtwo_neg(Vtwo a)
{
	Vtwo result = {
		.x = -a.x,
		.y = -a.y,
	};

	return result;
}

/**
 * @brief Adds two vectors
 *
 * @param a The first vector
 * @param b The second vector
 * @return Vtwo The sum of a and b (a.x + b.x, a.y + b.y)
 *
 * @example
 * Vtwo a = {1.0f, 2.0f};
 * Vtwo b = {3.0f, 4.0f};
 * Vtwo sum = vtwo_add(a, b);  // {4.0f, 6.0f}
 */
inline Vtwo vtwo_add(Vtwo a, Vtwo b)
{
	Vtwo result = {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};

	return result;
}

/**
 * @brief Subtracts vector b from vector a (calculates a - b)
 *
 * @param a The vector to subtract from
 * @param b The vector to subtract from a
 * @return Vtwo The difference (a.x - b.x, a.y - b.y)
 *
 * @example
 * Vtwo a = {1.0f, 2.0f};
 * Vtwo b = {5.0f, 7.0f};
 * Vtwo diff = vtwo_sub(a, b);  // {-4.0f, -5.0f}
 */
inline Vtwo vtwo_sub(Vtwo a, Vtwo b)
{
	Vtwo result = {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};

	return result;
}

/**
 * @brief Scales x axis
 *
 * @param a The vector
 * @param s The scalar
 * @return Vtwo The scaled vector (a.x * s, a.y)
 */
inline Vtwo vtwo_scale_x(Vtwo a, float s)
{
	Vtwo result = {
		.x = a.x * s,
		.y = a.y,
	};

	return result;
}

/**
 * @brief Scalar multiplication
 *
 * @param a The vector
 * @param s The scalar
 * @return Vtwo The scaled vector (a.x * s, a.y * s)
 */
inline Vtwo vtwo_scale(Vtwo a, float s)
{
	Vtwo result = {
		.x = a.x * s,
		.y = a.y * s,
	};

	return result;
}

/**
 * @brief Scalar addition
 *
 * @param a The vector
 * @param s The scalar
 * @return Vtwo The translated vector (a.x * s, a.y * s)
 */
inline Vtwo vtwo_add_scalar(Vtwo a, float s)
{
	Vtwo result = {
		.x = a.x + s,
		.y = a.y + s,
	};

	return result;
}

/**
 * @brief Calculates the dot product between two bidimensional vectors
 *
 * @param a One vector
 * @param b Other vector
 * @return float The dot product a . b
 */
inline float vtwo_dot(Vtwo a, Vtwo b)
{
	float result = a.x * b.x + a.y * b.y;

	return result;
}

/**
 * @brief Calculates the squared norm (squared length) of a vector
 *
 * Avoids a square root; use when only comparing magnitudes.
 *
 * @param a A vector
 * @return float ||a||^2 = a.x^2 + a.y^2
 */
inline float vtwo_norm_sq(Vtwo a)
{
	float norm_sq = vtwo_dot(a, a);

	return norm_sq;
}

inline float vtwo_norm(Vtwo a)
{
	float norm_sq = vtwo_norm_sq(a);
	float result = sqrtf(norm_sq);

	return result;
}

inline Vtwo vtwo_normalize(Vtwo a)
{
	float norm = vtwo_norm(a);
	Vtwo result = vtwo_scale(a, 1.0F / norm);

	return result;
}

// =============================================================================
// Memory
// =============================================================================

#define KB_TO_BYTE(_pr_v) ((_pr_v) * 1024)
#define MB_TO_BYTE(_pr_v) (KB_TO_BYTE(_pr_v) * 1024)
#define GB_TO_BYTE(_pr_v) (MB_TO_BYTE(_pr_v) * 1024)
#define TB_TO_BYTE(_pr_v) (GB_TO_BYTE(_pr_v) * 1024)

#define ARENA_PUSH_ARRAY(arena, type, count) (type *)arena_push((arena), sizeof(type) * (count))
#define ARENA_PUSH_STRUCT(arena, type) (type *)arena_push((arena), sizeof(type))

#define ARENA_PUSH_ARRAY_ZERO(arena, type, count) (type *)arena_push_zero((arena), sizeof(type) * (count))
#define ARENA_PUSH_STRUCT_ZERO(arena, type) (type *)arena_push_zero((arena), sizeof(type))

typedef struct Arena {
	size_t buf_size_byte;
	unsigned char *buf;
	size_t offset_byte;
	size_t prev_offset_byte;
} Arena;

void arena_init(Arena *restrict arena, const size_t buf_size_byte, unsigned char *const restrict buf)
{
	arena->buf_size_byte = buf_size_byte;
	arena->buf = buf;
	arena->offset_byte = 0;
}

void *arena_push(Arena *arena, size_t size_byte)
{
	assert(arena->offset_byte + size_byte <= arena->buf_size_byte);

	void *result = arena->buf + arena->offset_byte;
	arena->offset_byte += size_byte;

	return result;
}

void *arena_push_zero(Arena *arena, size_t size_byte)
{
	void *result = arena_push(arena, size_byte);

	memset(result, 0, size_byte);

	return result;
}

void arena_reset(Arena *arena)
{
	arena->offset_byte = 0;
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
#define IMPL_LOG_WRITE(fmt, ...)                                         \
	do {                                                                 \
		FILE *_pr_log_file = fopen("log.txt", "a+");                     \
		if (_pr_log_file != nullptr) {                                   \
			(void)fprintf(_pr_log_file, fmt __VA_OPT__(, ) __VA_ARGS__); \
			(void)fclose(_pr_log_file);                                  \
		}                                                                \
	} while (false)
#else
#define IMPL_LOG_WRITE(fmt, ...) printf(fmt __VA_OPT__(, ) __VA_ARGS__)
#endif

#define IMPL_LOG_MSG(log_level, fmt, file_name, func_name, line_number, ...)                                      \
	do {                                                                                                          \
		char _pr_tstamp_str[LOG_TSTAMP_BUF_SIZE];                                                                 \
		struct timespec _pr_ts;                                                                                   \
		struct tm _pr_tm;                                                                                         \
                                                                                                                  \
		if (!timespec_get(&_pr_ts, TIME_UTC)) {                                                                   \
			break;                                                                                                \
		}                                                                                                         \
		if (gmtime_s(&_pr_tm, &_pr_ts.tv_sec)) {                                                                  \
			break;                                                                                                \
		}                                                                                                         \
		if (strftime(_pr_tstamp_str, LOG_TSTAMP_BUF_SIZE, "%FT%T", &_pr_tm) == 0) {                               \
			break;                                                                                                \
		}                                                                                                         \
                                                                                                                  \
		IMPL_LOG_WRITE("%c[%s.%09ldZ] %s:%s:%s: " fmt "\n", log_level, _pr_tstamp_str, _pr_ts.tv_nsec, file_name, \
		               func_name, XSTRINGIFY(line_number) __VA_OPT__(, ) __VA_ARGS__);                            \
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
