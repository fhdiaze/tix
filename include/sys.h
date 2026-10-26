// clang-format Language: C

#ifndef SYS_H
#define SYS_H

typedef struct FileBuf {
	size_t size;
	void *buf;
} FileBuf;

/**
 * @brief Reads an entire file into memory.
 *
 * @param path The full path to the file.
 * @return A buffer of the file
 */
FileBuf sys_read_file(const char *path);

#endif // SYS_H
