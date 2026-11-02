// clang-format Language: C

#ifndef SYS_H
#define SYS_H

#include "lib.h"

/**
 * @brief Reads an entire file into memory.
 *
 * @param arena
 * @param path The full path to the file.
 * @return ErrCode The error code
 */
MemBuf sys_file_read(Arena *arena, const char *path);

size_t sys_file_get_size(const char *path);

#endif // SYS_H
