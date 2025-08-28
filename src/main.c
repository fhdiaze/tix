#include "config.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL

typedef enum Tix_Mode { tm_normal = 0, tm_view = 1 } Tix_Mode;

typedef struct Tix_State {
    Tix_Mode tix_mode;
    char *tix_Path;
    size_t tix_line;
    size_t tix_column;
} Tix_State;

int main(const int argc, const char *const argv[static argc + 1]) {
    if (argc < 2) {
        logf("tix needs a file to be open\n");
        exit(EXIT_FAILURE);
    }

    logt("Starting the editor\n");

    const char *filename = argv[1];
    FILE *file = fopen(filename, "re");
    if (file == NULL) {
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
        if (c == QUIT_CHAR) {
            break;
        }

        c = getchar();
    }

    return EXIT_SUCCESS;
}
