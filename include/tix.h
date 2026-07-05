// clang-format Language: C

#ifndef TIX_H
#define TIX_H

#include <stdint.h>

#define MAX_FILE_PATH 4096
#define MAX_KEYBOARD_KEYS 256

typedef enum {
	tm_normal = 0,
	tm_view = 1,
} TixMode;

typedef struct KeyState {
	// Half transition count per frame
	uint32_t half_transition_count;
	uint8_t ended_down;
} KeyState;

typedef struct KeyboardState {
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
} KeyboardState;

typedef struct TixState {
	uint8_t is_running;
	TixMode tix_mode;
	char tix_path[MAX_FILE_PATH];
	size_t tix_line;
	size_t tix_column;
} TixState;

extern void tix_editor_init(void);

// Parsing

// structure: tree, lines
// primitives: chars

#endif // TIX_H
