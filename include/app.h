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

#define PIXEL_SIZE 4
#define WINDOW_WIDTH_PX_MAX 7680
#define WINDOW_HEIGHT_PX_MAX 4320
#define BACKBUF_SIZE_MAX (WINDOW_WIDTH_PX_MAX * WINDOW_HEIGHT_PX_MAX * PIXEL_SIZE)
#define BUFFER_POOL_SIZE_MAX GB_TO_BYTE(3ULL)
#define LINES_PER_NOTCH 3
#define POINTS_PER_INCH 72
#define TILE_SIDE_PX_MAX 256
#define KEY_STACK_COUNT_MAX 256
#define FILE_PATH_SIZE_MAX 4096
#define DIRECT_CODE_POINT_MIN 32
#define DIRECT_CODE_POINT_MAX 126
#define DIRECT_CODE_POINTS_COUNT (DIRECT_CODE_POINT_MAX - DIRECT_CODE_POINT_MIN + 1)
#define ATLAS_PIXEL_SIZE 1
#define ATLAS_TILE_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE)
#define ATLAS_BUF_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE * DIRECT_CODE_POINTS_COUNT)

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

	KEY key_stack[KEY_STACK_COUNT_MAX];
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
	void *buf;
	size_t buf_size;

	uint8_t is_initialized;
} Storage;

typedef struct CaretPos {
	uint32_t row;
	uint32_t col;
} CaretPos;

/**
 * @brief (0,0) is on the top left corner. Top-To-Bottom.
 * The byte order in a register (little endian) is AA RR GG BB
 */
typedef struct Bitmap {
	void *buf;
	size_t buf_size;

	unsigned width_px;
	unsigned height_px;
} Bitmap;

typedef struct Atlas {
	unsigned char buf[ATLAS_BUF_SIZE_MAX];
	size_t buf_size;
} Atlas;

typedef struct Backbuf {
	unsigned char buf[BACKBUF_SIZE_MAX];
	uint32_t width_px;
	uint32_t height_px;
} Backbuf;

typedef struct Tix {
	Arena arena;
	Arena renderer_arena;
	Arena buffers_arena;

	size_t scroll_offset;
	size_t lines_count;

	Atlas atlas;
	Backbuf backbuf;

	CaretPos caret_pos;

	ContextMode context_mode;
	CaretMode caret_mode;

	char context_path[FILE_PATH_SIZE_MAX];
} Tix;

void app_init(Storage *storage);
void app_update_and_render(Storage *storage);

#endif // APP_H
