// clang-format Language: C

#ifndef SYS_H
#define SYS_H

#include "lib.h"

/**
 * @brief Reads an entire file into memory.
 *
 * @param arena
 * @param path The full path to the file.
 * @return MemBuf
 */
MemBuf sys_read_file(Arena *arena, const char *path);

#endif // SYS_H
