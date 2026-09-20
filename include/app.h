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

#define MAX_KEY_STACK 256
#define MAX_FILE_PATH 4096
#define MIN_DIRECT_CODE_POINT 32
#define MAX_DIRECT_CODE_POINT 126
#define DIRECT_CODE_POINTS_COUNT (MAX_DIRECT_CODE_POINT - MIN_DIRECT_CODE_POINT + 1)

typedef enum KEY : uint8_t {
	KEY_SHIFT,
	KEY_CAPS,
	KEY_L_CTRL,
	KEY_R_CTRL,

	KEY_G,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_H,

	KEY_USCORE,

	KEY_COUNT,
} KEY;

typedef struct KeyState {
	// Half transition count per frame
	uint32_t half_transition_count;
	uint8_t ended_down;
} KeyState;

typedef struct TixInput {
	float time_delta_s;

	unsigned mouse_x;
	unsigned mouse_y;

	/**
	 * @brief A "notch" refers to one discrete click/detent of a physical mouse wheel
	 */
	signed mouse_notches;

	KEY key_stack[MAX_KEY_STACK];
	KeyState key_states[KEY_COUNT];

	union {
		KeyState keys[KEY_COUNT];
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
	CONTEXT_MODE_FOLDER,
	CONTEXT_MODE_FILE,
} ContextMode;

typedef enum CaretMode : uint8_t {
	CARET_MODE_NORMAL,
	CARET_MODE_VIEW,
	CARET_MODE_INSERT,
	CARET_MODE_BLOCK,
} CaretMode;

typedef struct GlyphIdx {
	uint32_t value;
} GlyphIdx;

typedef struct Tile {
	GlyphIdx glyph_idx;

	uint32_t fg;
	uint32_t bg;
	uint32_t flags;
} Tile;

typedef struct Line {
	size_t start_idx;
	size_t newline_idx;
	uint8_t contains_complex_chars;
} Line;

typedef struct Storage {
	size_t buf_size_byte;
	void *buf;

	uint8_t is_initialized;
} Storage;

typedef struct CaretPos {
	uint32_t row;
	uint32_t col;
} CaretPos;

typedef struct Tix {
	Arena arena;
	Arena perm_arena;

	size_t scroll_offset;
	size_t lines_count;

	CaretPos caret_pos;

	Tile *tiles;
	uint32_t width_tile;
	uint32_t height_tile;

	void *atlas_buf;
	size_t atlas_buf_size_byte;

	ContextMode context_mode;
	CaretMode caret_mode;

	char context_path[MAX_FILE_PATH];
} Tix;

void app_init(Storage *storage);
void app_update_and_render(Storage *storage);

#endif // APP_H
