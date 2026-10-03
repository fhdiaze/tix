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
#define LINE_COUNT_MAX 1000000
#define FILE_PATH_SIZE_MAX 4096
#define DIRECT_CODE_POINT_MIN 32  // SP (Space)
#define DIRECT_CODE_POINT_MAX 126 // ~
#define DIRECT_CODE_POINTS_COUNT (DIRECT_CODE_POINT_MAX - DIRECT_CODE_POINT_MIN + 1)
#define ATLAS_PIXEL_SIZE 1
#define ATLAS_TILE_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE)
#define ATLAS_BUF_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE * DIRECT_CODE_POINTS_COUNT)

typedef enum KEY : uint8_t {
	KEY_SP,
	KEY_EXCL,
	KEY_QUOTE,
	KEY_NUM,
	KEY_DOLLAR,
	KEY_PERCNT,
	KEY_AMP,
	KEY_APOS,
	KEY_LPAREN,
	KEY_RPAREN,
	KEY_AST,
	KEY_PLUS,
	KEY_COMMA,
	KEY_DASH,
	KEY_PERIOD,
	KEY_SLASH,

	KEY_ZERO,
	KEY_ONE,
	KEY_TWO,
	KEY_THREE,
	KEY_FOUR,
	KEY_FIVE,
	KEY_SIX,
	KEY_SEVEN,
	KEY_EIGHT,
	KEY_NINE,

	KEY_COLON,
	KEY_SEMI,
	KEY_LT,
	KEY_EQ,
	KEY_GT,
	KEY_QUEST,
	KEY_COMMAT,

	KEY_UA,
	KEY_UB,
	KEY_UC,
	KEY_UD,
	KEY_UE,
	KEY_UF,
	KEY_UG,
	KEY_UH,
	KEY_UI,
	KEY_UJ,
	KEY_UK,
	KEY_UL,
	KEY_UM,
	KEY_UN,
	KEY_UO,
	KEY_UP,
	KEY_UQ,
	KEY_UR,
	KEY_US,
	KEY_UT,
	KEY_UU,
	KEY_UV,
	KEY_UW,
	KEY_UX,
	KEY_UY,
	KEY_UZ,

	KEY_LSQB,
	KEY_BSLASH,
	KEY_RSQB,
	KEY_HAT,
	KEY_LOWBAR,
	KEY_GRAVE,

	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,

	KEY_LCUB,
	KEY_PIPE,
	KEY_RCUB,
	KEY_TILDE,
	KEY_DEL,

	KEY_SHIFT,
	KEY_CTRL,
	KEY_RETURN,
	KEY_ESC,

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

typedef struct AtlasIdx {
	uint32_t value;
} AtlasIdx;

typedef struct Tile {
	AtlasIdx glyph_idx;

	uint32_t fg;
	uint32_t bg;
	uint32_t flags; // cursor/selection/etc., added in Stage 3
} Tile;

typedef struct Line {
	size_t start_idx;
	size_t newline_idx;
	uint8_t contains_complex_chars;
} Line;

typedef struct Storage {
	size_t buf_size;
	void *buf;

	uint8_t is_initialized;
} Storage;

typedef struct Caret {
	size_t line_idx;
	size_t col_idx;
	uint32_t flags;
} Caret;

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
	uint32_t pitch_size;
	uint32_t width_px;
	uint32_t height_px;
	unsigned char buf[BACKBUF_SIZE_MAX];
} Backbuf;

typedef struct Buffer {
	Line lines[LINE_COUNT_MAX];
	size_t lines_count;
} Buffer;

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
	Atlas atlas;
	Caret caret;
	size_t scroll_idx;
	size_t lines_count;
	Backbuf backbuf;
	Buffer buffer;
	TileGrid grid;
	KeyState stack_key_states[KEY_STACK_COUNT_MAX];
	uint16_t stack_key_count;
	ContextMode context_mode;
	CaretMode caret_mode;
	KEY stack_key_codes[KEY_STACK_COUNT_MAX];
	char context_path[FILE_PATH_SIZE_MAX];
} Tix;

void app_init(Storage *storage);
void app_update_and_render(Storage *storage);

#endif // APP_H
