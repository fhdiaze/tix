// clang-format Language: C

#ifndef SYS_H
#define SYS_H

typedef struct ReadFileResult {
	size_t size_byte;
	void *buf;
} ReadFileResult;

ReadFileResult sys_read_file(const char *path);

#endif // SYS_H
