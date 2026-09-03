// clang-format Language: C

#ifndef APP_H
#define APP_H

#include <stdint.h>

#include "lib.h"

#if DEBUG
#define MEMORY_BASE_ADDRESS ((void *)TB_TO_BYTE(2ULL))
#else
#define MEMORY_BASE_ADDRESS (nullptr)
#endif // MEMORY_BASE_ADDRESS

#define MAX_FILE_PATH 4096
#define MAX_KEYBOARD_KEYS 256

typedef struct KeyState {
	// Half transition count per frame
	uint32_t half_transition_count;
	uint8_t ended_down;
} KeyState;

typedef struct TixInput {
	float place_holder;

	union {
		KeyState keys[MAX_KEYBOARD_KEYS];
		struct {
			KeyState move_up;
			KeyState move_down;
			KeyState move_left;
			KeyState move_right;
			KeyState scape;
		};
	};
} TixInput;

typedef enum ContextMode : uint8_t {
	CONTEXT_MODE_FOLDER = 0,
	CONTEXT_MODE_FILE = 1,
} ContextMode;

typedef enum CursorMode : uint8_t {
	CURSOR_MODE_NORMAL = 0,
	CURSOR_MODE_VIEW = 1,
} CursorMode;

typedef struct Cell {
	char c;
} Cell;

/**
 * @brief
 *
 */
typedef struct Point {
	size_t x;
	size_t y;
} Point;

typedef struct Grid {
	Cell *cells;
	uint32_t width;
	uint32_t height;
} Grid;

typedef struct Line {
	size_t start_idx;
	size_t newline_idx;
	uint8_t contains_complex_chars;
} Line;

typedef struct Run {
	size_t starts_at_byte;
	size_t ends_at_plus_one_byte;
	uint8_t contains_complex_chars;
} Run;

typedef struct Tix {
	Arena arena;

	size_t scroll_offset;
	size_t lines_count;

	size_t caret_line;
	size_t caret_column;

	Grid grid;

	ContextMode context_mode;
	CursorMode cursor_mode;


	char context_path[MAX_FILE_PATH];
} Tix;

void app_init(Tix *tix);
void app_update_and_render(Tix *tix);

#endif // APP_H
