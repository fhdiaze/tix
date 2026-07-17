// clang-format Language: C

#ifndef APP_H
#define APP_H

#include <stdint.h>

#if DEBUG
#define MEMORY_BASE_ADDRESS ((void *)TB_TO_BYTES(2))
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

typedef struct Storage {
	size_t perm_size_byte;
	unsigned char *perm_base_address;

	size_t trans_size_byte;
	unsigned char *trans_base_address;
} Storage;

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

typedef struct Grid {
	Cell *cells;
	uint32_t width;
	uint32_t height;
} Grid;

typedef struct Tix {
	uint8_t is_running;

	CursorMode cursor_mode;

	ContextMode context_mode;
	char context_path[MAX_FILE_PATH];

	size_t tix_line;
	size_t tix_column;
	Grid grid;
	Storage storage;
} Tix;

// =============================================================================
// Api
// =============================================================================

void tix_init(Tix *tix);

// Parsing

// structure: tree, lines
// primitives: chars

#endif // APP_H
