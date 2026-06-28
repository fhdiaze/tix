#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	LOG_INFO("%p", (void *)hInstance);
	LOG_INFO("%p", (void *)hPrevInstance);
	LOG_INFO("%p", (void *)lpCmdLine);
	LOG_INFO("%d", nShowCmd);

	LOG_INFO("Starting the editor\n");

	const char *filename = "test.txt";
	FILE *file = fopen(filename, "re");
	if (file == nullptr) {
		LOG_FATAL("The file %s could not be open\n", filename);
		return EXIT_FAILURE;
	}

	char line[1000];
	if (fgets(line, 1000, file) == nullptr) {
		LOG_FATAL("Failed to read from file %s\n", filename);
		if (fclose(file) == EOF) {
			LOG_FATAL("Failed to close file %s\n", filename);

			return EXIT_FAILURE;
		}

		return EXIT_FAILURE;
	}

	LOG_INFO("Read line: %s", line);

	if (fclose(file) == EOF) {
		LOG_FATAL("Failed to close file %s\n", filename);

		return EXIT_FAILURE;
	}

	constexpr int QUIT_CHAR = 'q';
	int c = getchar();

	while (c != EOF) {
		if (c == QUIT_CHAR) {
			break;
		}

		c = getchar();
	}

	return EXIT_SUCCESS;
}
