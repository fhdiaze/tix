// clang-format Language: C

#ifndef APP_H
#define APP_H

#include <stdint.h>

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
	size_t perm_size;
	unsigned char *perm_base_address;

	size_t trans_size;
	unsigned char *trans_base_address;
} Storage;

typedef enum {
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
	char context_path[MAX_FILE_PATH];
	size_t tix_line;
	size_t tix_column;
	Grid grid;
	Storage storage;
} Tix;

// =============================================================================
// Api
// =============================================================================

void tix_editor_init(void);

// Parsing

// structure: tree, lines
// primitives: chars

#endif // APP_H
