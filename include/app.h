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
#define BACKBUF_SIZE_MAX (7680 * 4320 * PIXEL_SIZE)
#define BUFFER_POOL_SIZE_MAX GB_TO_BYTE(3ULL)
#define LINES_PER_NOTCH 3
#define POINTS_PER_INCH 72
#define TILE_SIDE_PX_MAX 256
#define KEY_STACK_COUNT_MAX 8
#define FILE_PATH_SIZE_MAX 4096
#define DIRECT_CODE_POINT_MIN 32
#define DIRECT_CODE_POINT_MAX 126
#define DIRECT_CODE_POINTS_COUNT (DIRECT_CODE_POINT_MAX - DIRECT_CODE_POINT_MIN + 1)
#define ATLAS_PIXEL_SIZE 1
#define ATLAS_TILE_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE)
#define ATLAS_BUF_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE * DIRECT_CODE_POINTS_COUNT)

typedef enum KEY : uint8_t {
	KEY_SHIFTED,
	KEY_CTRL,

	KEY_D,
	KEY_G,
	KEY_H,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_U,
	KEY_W,

	KEY_USCORE,

	KEY_COUNT,
} KEY;

typedef struct KeyState {
	// Half transition count per frame
	uint32_t half_transition_count;
	uint8_t ended_down;
} KeyState;

typedef struct TixInput {
	uint32_t mouse_x;
	uint32_t mouse_y;

	/**
	 * @brief A "notch" refers to one discrete click/detent of a physical mouse wheel
	 */
	int32_t mouse_notches;

	KeyState keys[KEY_COUNT];
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
	size_t line_idx;
	size_t col_idx;
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

typedef struct TileGrid {
	uint32_t tile_width_px;
	uint32_t tile_height_px;
	uint32_t tile_ascent_px;

	uint32_t tile_count_x;
	uint32_t tile_count_y;
} TileGrid;

typedef struct Tix {
	Arena arena;
	Arena renderer_arena;
	Arena buffers_arena;

	KEY stack_key_codes[KEY_STACK_COUNT_MAX];
	KeyState stack_key_states[KEY_STACK_COUNT_MAX];
	uint16_t stack_key_count;

	size_t scroll_idx;
	size_t lines_count;

	TileGrid grid;
	Atlas atlas;
	Backbuf backbuf;

	CaretPos caret;

	ContextMode context_mode;
	CaretMode caret_mode;

	char context_path[FILE_PATH_SIZE_MAX];
} Tix;

void app_init(Storage *storage);
void app_update_and_render(Storage *storage);

#endif // APP_H
