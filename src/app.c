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

constexpr char WORD_CHAR_SET[] = { 'a', 'b' };
constexpr char WHITE_SPACE_CHAR_SET[] = { ' ', '	' };

typedef enum KeyCode : uint8_t {
	KEY_CODE_SP,
	KEY_CODE_EXCL,
	KEY_CODE_QUOTE,
	KEY_CODE_NUM,
	KEY_CODE_DOLLAR,
	KEY_CODE_PERCNT,
	KEY_CODE_AMP,
	KEY_CODE_APOS,
	KEY_CODE_LPAREN,
	KEY_CODE_RPAREN,
	KEY_CODE_AST,
	KEY_CODE_PLUS,
	KEY_CODE_COMMA,
	KEY_CODE_DASH,
	KEY_CODE_PERIOD,
	KEY_CODE_SLASH,

	KEY_CODE_ZERO,
	KEY_CODE_ONE,
	KEY_CODE_TWO,
	KEY_CODE_THREE,
	KEY_CODE_FOUR,
	KEY_CODE_FIVE,
	KEY_CODE_SIX,
	KEY_CODE_SEVEN,
	KEY_CODE_EIGHT,
	KEY_CODE_NINE,

	KEY_CODE_COLON,
	KEY_CODE_SEMI,
	KEY_CODE_LT,
	KEY_CODE_EQ,
	KEY_CODE_GT,
	KEY_CODE_QUEST,
	KEY_CODE_COMMAT,

	KEY_CODE_UA,
	KEY_CODE_UB,
	KEY_CODE_UC,
	KEY_CODE_UD,
	KEY_CODE_UE,
	KEY_CODE_UF,
	KEY_CODE_UG,
	KEY_CODE_UH,
	KEY_CODE_UI,
	KEY_CODE_UJ,
	KEY_CODE_UK,
	KEY_CODE_UL,
	KEY_CODE_UM,
	KEY_CODE_UN,
	KEY_CODE_UO,
	KEY_CODE_UP,
	KEY_CODE_UQ,
	KEY_CODE_UR,
	KEY_CODE_US,
	KEY_CODE_UT,
	KEY_CODE_UU,
	KEY_CODE_UV,
	KEY_CODE_UW,
	KEY_CODE_UX,
	KEY_CODE_UY,
	KEY_CODE_UZ,

	KEY_CODE_LSQB,
	KEY_CODE_BSLASH,
	KEY_CODE_RSQB,
	KEY_CODE_HAT,
	KEY_CODE_LOWBAR,
	KEY_CODE_GRAVE,

	KEY_CODE_A,
	KEY_CODE_B,
	KEY_CODE_C,
	KEY_CODE_D,
	KEY_CODE_E,
	KEY_CODE_F,
	KEY_CODE_G,
	KEY_CODE_H,
	KEY_CODE_I,
	KEY_CODE_J,
	KEY_CODE_K,
	KEY_CODE_L,
	KEY_CODE_M,
	KEY_CODE_N,
	KEY_CODE_O,
	KEY_CODE_P,
	KEY_CODE_Q,
	KEY_CODE_R,
	KEY_CODE_S,
	KEY_CODE_T,
	KEY_CODE_U,
	KEY_CODE_V,
	KEY_CODE_W,
	KEY_CODE_X,
	KEY_CODE_Y,
	KEY_CODE_Z,

	KEY_CODE_LCUB,
	KEY_CODE_PIPE,
	KEY_CODE_RCUB,
	KEY_CODE_TILDE,
	KEY_CODE_DEL,

	KEY_CODE_SHIFT,
	KEY_CODE_CTRL,
	KEY_CODE_RETURN,
	KEY_CODE_ESC,

	KEY_CODE_COUNT,
} KeyCode;

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

typedef enum Command : uint8_t {
	COMMAND_W,
} Command;

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

	KeyState keys[KEY_CODE_COUNT];
} TixInput;

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
	size_t copy_col_idx;
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
	size_t buf_size;
	uint32_t tile_size;
	unsigned char buf[ATLAS_BUF_SIZE_MAX];
} Atlas;

typedef struct Backbuf {
	size_t buf_size;
	unsigned char buf[BACKBUF_SIZE_MAX];
	uint32_t pitch_size;
	uint32_t width_px;
	uint32_t height_px;
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
	KeyCode stack_key_codes[KEY_STACK_COUNT_MAX];
	char context_path[FILE_PATH_SIZE_MAX];
} Tix;

// Parsing

// ui: pane, gutter, tab, panel,
// entities: buffer, document, span/range, anchor, mark, selection
// structure: tree, lines
// primitives: chars, code_point, grapheme cluster, glyph, rune,

static void buffer_split(Buffer *buffer)
{
}

void app_init(Storage *storage)
{
	buffer_split(nullptr);
}

static void tix_update_backbuf_dims(Tix *tix, uint32_t width_px, uint32_t height_px)
{
	ASSERT(width_px * height_px * PIXEL_SIZE <= BACKBUF_SIZE_MAX);

	tix->backbuf.width_px = width_px;
	tix->backbuf.height_px = height_px;
	tix->backbuf.buf_size = (size_t)tix->backbuf.width_px * tix->backbuf.height_px * PIXEL_SIZE;
	tix->backbuf.pitch_size = tix->backbuf.width_px * PIXEL_SIZE;

	tix->grid.tile_count_x = tix->backbuf.width_px / tix->grid.tile_width_px;
	tix->grid.tile_count_y = tix->backbuf.height_px / tix->grid.tile_height_px;
}

static void tix_update_tile_dims(Tix *tix, uint32_t width_px, uint32_t height_px, uint32_t ascent_px)
{
	tix->grid.tile_width_px = width_px;
	tix->grid.tile_height_px = height_px;
	tix->grid.tile_ascent_px = ascent_px; // Includes the internal leading

	tix->atlas.tile_size = tix->grid.tile_width_px * tix->grid.tile_height_px * ATLAS_PIXEL_SIZE;
	tix->atlas.buf_size = (size_t)DIRECT_CODE_POINTS_COUNT * tix->atlas.tile_size;

	ASSERT(tix->grid.tile_width_px <= TILE_SIDE_PX_MAX);
	ASSERT(tix->grid.tile_height_px <= TILE_SIDE_PX_MAX);
	ASSERT(tix->atlas.tile_size <= ATLAS_TILE_SIZE_MAX);
	ASSERT(tix->atlas.buf_size <= (size_t)ATLAS_BUF_SIZE_MAX);
}
