// clang-format Language: C

#ifndef SYS_H
#define SYS_H

#include "lib.h"

/**
 * @brief Reads an entire file into memory.
 *
 * @param path The full path to the file.
 * @param arena Arena to allocate memory to read into
 * @param result Reading result
 * @return ErrCode Error information
 */
ErrCode sys_file_read(const char *path, Arena *arena, MemBuf *result);

/**
 * @brief Reads a chunk of a file
 *
 * @param path The full path to the file.
 * @param chunk_size Size of the reading chunk
 * @param result Buffer to read into
 * @return ErrCode Error information
 */
ErrCode sys_file_read_chunk(const char *path, uint32_t chunk_size, MemBuf *result);

/**
 * @brief Gets the size of a file
 *
 * @param path Absolute path of the file
 * @param result Size of the file
 * @return ErrCode Error information
 */
ErrCode sys_file_get_size(const char *path, size_t *result);

/**
 * @brief
 *
 * @param path
 * @param result
 * @return ErrCode
 */
ErrCode sys_file_get_last_write_time(const char *path, size_t *result);

#endif // SYS_H
