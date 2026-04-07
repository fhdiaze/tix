#include "log.h"
#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
        logi("%p", (void *)hInstance);
        logi("%p", (void *)hPrevInstance);
        logi("%p", (void *)lpCmdLine);
        logi("%d", nCmdShow);
}

int main(const int argc, const char *const argv[static argc + 1])
{
        if (argc < 2) {
                logf("tix needs a file to be open\n");
                exit(EXIT_FAILURE);
        }

        logt("Starting the editor\n");

        const char *filename = argv[1];
        FILE *file = fopen(filename, "re");
        if (file == nullptr) {
                logf("The file %s could not be open\n", filename);
                return EXIT_FAILURE;
        }

        char line[1000];
        fgets(line, 1000, file);

        logi("Read line: %s", line);

        fclose(file);

        constexpr int QUIT_CHAR = 'q';
        int c = getchar();

        while (c != EOF) {
                if (c == QUIT_CHAR) { break; }

                c = getchar();
        }

        return EXIT_SUCCESS;
}
