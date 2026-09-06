// clang-format Language: C

#ifndef SYS_H
#define SYS_H

#include "app.h"
#include "lib.h"
typedef struct ReadFileResult {
	size_t size_byte;
	void *buf;
} ReadFileResult;

/**
 * @brief Reads an entire file into memory.
 *
 * @param path The full path to the file.
 * @return ReadFileResult
 */
ReadFileResult sys_read_file(const char *path);

#endif // SYS_H
