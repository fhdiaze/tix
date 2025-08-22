#define _CRT_SECURE_NO_WARNINGS

#include "log.h"
#include <corecrt_search.h>
#include <stdio.h>
#include <stdlib.h>

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL

int main(int const argc, char const *const argv[static argc + 1]) {
    if (argc < 2) {
        logf("tix needs a file to be open\n");
        exit(EXIT_FAILURE);
    }

    logt("Starting the editor\n");

    char const *filename = argv[1];
    FILE *file = fopen(filename, "re");
    if (file == NULL) {
        logf("The file %s could not be open\n", filename);
        exit(EXIT_FAILURE);
    }

    char line[1000];
    fgets(line, 1000, file);

    logi("Read line: %s", line);

    fclose(file);

    return EXIT_SUCCESS;
}
