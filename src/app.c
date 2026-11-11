#include "sys.h"
#include <stdint.h>

#include "lib.h"

#if DEBUG
#define MEMORY_BASE_ADDRESS ((void *)TB_TO_BYTE(2ULL))
#else
#define MEMORY_BASE_ADDRESS (nullptr)
#endif // MEMORY_BASE_ADDRESS

#define CARET_WANTED_COL_IDX_END SIZE_MAX
#define PIXEL_SIZE 4
#define FRAMEBUF_SIZE_MAX (7680 * 4320 * PIXEL_SIZE)
#define FILES_BUF_SIZE_MAX GB_TO_BYTE(1ULL)
#define POINTS_PER_INCH 72
#define TILE_SIDE_PX_MAX 256
#define TILE_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * PIXEL_SIZE)
#define KEY_STACK_COUNT_MAX 8
#define LINES_PER_NOTCH 3
#define LINES_COUNT_MAX 2000000
#define FILE_PATH_SIZE_MAX 4096
#define RUNE_DIRECT_MIN 32  // SP (Space)
#define RUNE_DIRECT_MAX 126 // ~ (Tilde)
#define RUNE_DIRECT_COUNT (RUNE_DIRECT_MAX - RUNE_DIRECT_MIN + 1)
#define RUNE_IS_DIRECT(c) (c >= RUNE_DIRECT_MIN && c <= RUNE_DIRECT_MAX)
#define ATLAS_PIXEL_SIZE 1
#define ATLAS_TILE_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE)
#define RENDERER_BUF_SIZE_MAX (ATLAS_TILE_SIZE_MAX * 20)
#define ATLAS_BUF_SIZE_MAX (TILE_SIDE_PX_MAX * TILE_SIDE_PX_MAX * ATLAS_PIXEL_SIZE * RUNE_DIRECT_COUNT)

typedef uint32_t RuneTransitionFlags;

typedef enum RuneTransitionFlag : uint32_t {
	RUNE_TRANSITION_FLAG_ANY_TO_NONBLANK = (1U << 0U),
	RUNE_TRANSITION_FLAG_BLANK_TO_NONBLANK = (1U << 1U),
	RUNE_TRANSITION_FLAG_NONBLANK_TO_BLANK = (1U << 2U),
	RUNE_TRANSITION_FLAG_WORD_TO_PUNCT = (1U << 3U),
	RUNE_TRANSITION_FLAG_ANY_TO_EMPTYLINE = (1U << 4U),
	RUNE_TRANSITION_FLAG_PUNCT_TO_WORD = (1U << 5U),
	RUNE_TRANSITION_FLAG_CARETABLE_TO_CARETABLE = (1U << 6U),
	RUNE_TRANSITION_FLAG_CARETABLE_TO_NONCARETABLE = (1U << 7U),
	RUNE_TRANSITION_FLAG_NONCARETABLE_TO_CARETABLE = (1U << 8U),
	RUNE_TRANSITION_FLAG_NONCARETABLE_TO_NONCARETABLE = (1U << 9U),
	RUNE_TRANSITION_FLAG_ANY_TO_CARETABLE = (1U << 10U),
} RuneTransitionFlag;

typedef uint32_t CaretFlags;

typedef enum CaretFlag : uint32_t {
	CARET_FLAG_SELECTABLE = (1U << 0U),
	CARET_FLAG_NORMAL = (1U << 1U),
	CARET_FLAG_VIEW = (1U << 2U),
	CARET_FLAG_INSERT = (1U << 3U),
	CARET_FLAG_BLOCK = (1U << 4U),
	CARET_FLAG_ACTIVE = (1U << 5U),
} CaretFlag;

typedef uint32_t RuneFlags;

typedef enum RuneFlag : uint32_t {
	RUNE_FLAG_INVALID = (1U << 0U),
	RUNE_FLAG_WORD = (1U << 1U),
	RUNE_FLAG_PUNCT = (1U << 2U),
	RUNE_FLAG_BLANK = (1U << 3U),
	RUNE_FLAG_SURROGATE = (1U << 4U),
	RUNE_FLAG_DIGIT = (1U << 5U),
	RUNE_FLAG_UPPER = (1U << 6U),
	RUNE_FLAG_LOWER = (1U << 7U),
	RUNE_FLAG_CARETABLE = (1U << 8U),
} RuneFlag;

typedef uint32_t TileFlags;

typedef enum TileFlag : uint32_t {
	TILE_FLAG_CARETABLE = (1U << 0U),
	TILE_FLAG_SELECTED = (1U << 1U),
} TileFlag;

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

	KEY_CODE_HOME,
	KEY_CODE_END,

	KEY_CODE_ARROW_LEFT,
	KEY_CODE_ARROW_RIGHT,
	KEY_CODE_ARROW_DOWN,
	KEY_CODE_ARROW_UP,

	KEY_CODE_PAGE_DOWN,
	KEY_CODE_PAGE_UP,

	KEY_CODE_COUNT,
} KeyCode;

typedef enum ContextMode : uint8_t {
	CONTEXT_MODE_FOLDER,
	CONTEXT_MODE_FILE,
} ContextMode;

typedef enum Command : uint8_t {
	COMMAND_NEXT_WORD,
	COMMAND_PREV_WORD,
	COMMAND_NEXT_PHRASE,
	COMMAND_PREV_PHRASE,
	COMMAND_NEXT_ENDWORD,
	COMMAND_PREV_ENDWORD,
	COMMAND_NEXT_ENDPHRASE,
	COMMAND_PREV_ENDPHRASE,
	COMMAND_FIRST_NONBLANK,
	COMMAND_LAST_NONBLANK,
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

typedef struct Rune {
	uint32_t value;
} Rune;

typedef struct Tile {
	AtlasIdx glyph_idx;

	uint32_t fg;
	uint32_t bg;
	uint32_t flags; // cursor/selection/etc., added in Stage 3
} Tile;

typedef struct Line Line;
typedef struct Line {
	Line *next;
	size_t idx;
	size_t file_offset;
	size_t size;
	uint8_t contains_complex_chars;
} Line;

typedef struct TxtPos {
	size_t line_idx;
	size_t col_idx;
} TxtPos;

typedef struct Caret {
	TxtPos pos;
	size_t wanted_col_idx;
	CaretFlags flags;
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
	uint32_t tile_pitch_size;
	unsigned char buf[ATLAS_BUF_SIZE_MAX];
} Atlas;

typedef struct FrameBuf {
	size_t buf_size;
	unsigned char buf[FRAMEBUF_SIZE_MAX];
	uint32_t pitch_size;
	uint32_t width_px;
	uint32_t height_px;
} FrameBuf;

typedef struct TxtBuf {
	size_t lines_count;
	MemBuf file;
	size_t file_time;
	Line *head;
	Line *tail;
} TxtBuf;

typedef struct RopeBuf {
	uint32_t place_holder;
	Caret caret;
} RopeBuf;

typedef struct TileGrid {
	uint32_t tile_width_px;
	uint32_t tile_height_px;
	uint32_t tile_ascent_px;

	uint32_t tile_size;

	uint32_t tile_count_x;
	uint32_t tile_count_y;
} TileGrid;

typedef struct View {
	TxtBuf *buf;
	size_t scroll_idx;
	Caret caret;
} View;

typedef struct Tix {
	unsigned char renderer_buf[RENDERER_BUF_SIZE_MAX];
	unsigned char files_buf[FILES_BUF_SIZE_MAX];
	Line lines_buf[LINES_COUNT_MAX];

	Pool lines_pool;

	Arena renderer_arena;
	Arena files_arena;

	Atlas atlas;
	FrameBuf framebuf;
	TxtBuf txtbuf;
	TileGrid grid;
	View view;
	KeyState stack_key_states[KEY_STACK_COUNT_MAX];
	ContextMode context_mode;
	uint16_t stack_key_count;
	KeyCode stack_key_codes[KEY_STACK_COUNT_MAX];
	char context_path[FILE_PATH_SIZE_MAX];
} Tix;

static void tix_update_framebuf_dims(Tix *tix, uint32_t width_px, uint32_t height_px)
{
	ASSERT(width_px * height_px * PIXEL_SIZE <= FRAMEBUF_SIZE_MAX);

	tix->framebuf.width_px = width_px;
	tix->framebuf.height_px = height_px;
	tix->framebuf.buf_size = (size_t)tix->framebuf.width_px * tix->framebuf.height_px * PIXEL_SIZE;
	tix->framebuf.pitch_size = tix->framebuf.width_px * PIXEL_SIZE;

	tix->grid.tile_count_x = tix->framebuf.width_px / tix->grid.tile_width_px;
	tix->grid.tile_count_y = tix->framebuf.height_px / tix->grid.tile_height_px;
}

static void tix_update_tile_dims(Tix *tix, uint32_t width_px, uint32_t height_px, uint32_t ascent_px)
{
	tix->grid.tile_width_px = width_px;
	tix->grid.tile_height_px = height_px;
	tix->grid.tile_ascent_px = ascent_px; // Includes the internal leading
	tix->grid.tile_size = tix->grid.tile_width_px * tix->grid.tile_height_px * PIXEL_SIZE;

	ASSERT(tix->grid.tile_width_px <= TILE_SIDE_PX_MAX);
	ASSERT(tix->grid.tile_height_px <= TILE_SIDE_PX_MAX);
	ASSERT(tix->grid.tile_size <= TILE_SIZE_MAX);

	tix->atlas.tile_size = tix->grid.tile_width_px * tix->grid.tile_height_px * ATLAS_PIXEL_SIZE;
	tix->atlas.buf_size = (size_t)RUNE_DIRECT_COUNT * tix->atlas.tile_size;
	tix->atlas.tile_pitch_size = tix->grid.tile_width_px * ATLAS_PIXEL_SIZE;

	ASSERT(tix->atlas.tile_size <= ATLAS_TILE_SIZE_MAX);
	ASSERT(tix->atlas.buf_size <= (size_t)ATLAS_BUF_SIZE_MAX);
}

inline static uint32_t txtpos_are_equal(const TxtPos *const one, const TxtPos *const other)
{
	uint32_t result = one->line_idx == other->line_idx && one->col_idx == other->col_idx;

	return result;
}

inline static size_t txtbuf_hash_slot(size_t line_idx)
{
	size_t hash_value = line_idx * 19 + line_idx * 7 + line_idx * 3;
	size_t hash_slot_idx = hash_value % LINES_COUNT_MAX;

	return hash_slot_idx;
}

inline static Line *txtbuf_get_line(const TxtBuf *const buf, const size_t line_idx, Tix *tix)
{
	Line *result = {};
	size_t hash_slot_idx = txtbuf_hash_slot(line_idx);
	ASSERT(hash_slot_idx < LINES_COUNT_MAX);

	do {
		if (result->idx == line_idx) {
			break;
		}

		++hash_slot_idx;
	} while (hash_slot_idx < buf->lines_count);

	result = &tix->lines_buf[hash_slot_idx];

	return result;
}

inline static TxtPos txtbuf_prev_pos(TxtBuf *buf, TxtPos pos, Tix *tix)
{
	TxtPos prev_pos = pos;

	if (prev_pos.col_idx > 0) {
		--prev_pos.col_idx;
	} else if (prev_pos.line_idx > 0) {
		--prev_pos.line_idx;
		Line *prev_line = txtbuf_get_line(buf, prev_pos.line_idx, tix);
		ASSERT(prev_line->size > 0);
		prev_pos.col_idx = prev_line->size - 1;
	}

	return prev_pos;
}

inline static TxtPos txtbuf_next_pos(const TxtBuf *const buf, const TxtPos pos, Tix *tix)
{
	TxtPos result = pos;

	Line *line = txtbuf_get_line(buf, pos.line_idx, tix);
	ASSERT(line);
	if (pos.line_idx < buf->lines_count) {
		if (pos.col_idx + 1 < line->size) {
			++result.col_idx;
		} else if (pos.line_idx + 1 < buf->lines_count) {
			++result.line_idx;
			result.col_idx = 0;
		}
	}

	return result;
}

inline static Line *txtbuf_push_line(TxtBuf *buf, Tix *tix)
{
	Line *result = pool_push_zero(&tix->lines_pool);

	if (result) {
		if (buf->tail) {
			buf->tail->next = result;
			buf->tail = result;
		} else {
			buf->head = result;
			buf->tail = result;
		}
		++buf->lines_count;
	}

	return result;
}

inline static Rune txtbuf_get_rune(TxtBuf *buf, TxtPos pos, Tix *tix)
{
	Rune result = {};

	Line *line = txtbuf_get_line(buf, pos.line_idx, tix);

	if (line && line->size > 0) {
		ASSERT(pos.col_idx < line->size);
		size_t offset = line->file_offset + pos.col_idx;
		ASSERT(offset < buf->file.size);
		result.value = buf->file.buf[offset];
	}

	return result;
}

static RuneFlags txtbuf_get_rune_flags(TxtBuf *buf, TxtPos *pos, Rune rune, Tix *tix)
{
	RuneFlags rune_flags = {};

	Line *line = txtbuf_get_line(buf, pos->line_idx, tix);
	ASSERT(line);
	if (rune.value == '\n') {
		rune_flags |= RUNE_FLAG_BLANK;
		if (line->size == 1 && pos->col_idx == 0) {
			rune_flags |= RUNE_FLAG_CARETABLE;
		}
	} else if (rune.value == '\r') {
		rune_flags |= RUNE_FLAG_BLANK;
		if (line->size == 2 && pos->col_idx == 0) {
			// NOTE(fredy): assuming the last byte is '\n'
			rune_flags |= RUNE_FLAG_CARETABLE;
		}
	} else {
		rune_flags |= RUNE_FLAG_CARETABLE;
	}

	if (rune.value == ' ' || rune.value == '\t') {
		rune_flags |= RUNE_FLAG_BLANK;
	} else if ('0' <= rune.value && rune.value <= '9') {
		rune_flags |= RUNE_FLAG_DIGIT;
		rune_flags |= RUNE_FLAG_WORD;
	} else if ('A' <= rune.value && rune.value <= 'Z') {
		rune_flags |= RUNE_FLAG_UPPER;
		rune_flags |= RUNE_FLAG_WORD;
	} else if ('a' <= rune.value && rune.value <= 'z') {
		rune_flags |= RUNE_FLAG_LOWER;
		rune_flags |= RUNE_FLAG_WORD;
	} else if ((rune.value >= '!' && rune.value <= '/') || (rune.value >= ':' && rune.value <= '@') ||
	           (rune.value >= '[' && rune.value <= '`') || (rune.value >= '{' && rune.value <= '~')) {
		rune_flags |= RUNE_FLAG_PUNCT;
	}

	return rune_flags;
}

static void txtbuf_next_rune(TxtBuf *buf, TxtPos pos)
{
}

inline static RuneFlag rune_get_flag(RuneFlags flags, RuneFlag flag)
{
	RuneFlag result = flags & flag;

	return result;
}

inline static RuneTransitionFlags txtbuf_get_rune_transition_flags(TxtBuf *buf, Rune src_rune, TxtPos *src_pos,
                                                                   Rune dst_rune, TxtPos *dst_pos, Tix *tix)
{
	RuneTransitionFlags flags = {};

	RuneFlags src_rune_flags = txtbuf_get_rune_flags(buf, src_pos, src_rune, tix);
	RuneFlags dst_rune_flags = txtbuf_get_rune_flags(buf, dst_pos, dst_rune, tix);

	if (rune_get_flag(src_rune_flags, RUNE_FLAG_CARETABLE)) {
		if (!rune_get_flag(dst_rune_flags, RUNE_FLAG_CARETABLE)) {
			flags |= RUNE_TRANSITION_FLAG_CARETABLE_TO_NONCARETABLE;
		} else {
			flags |= RUNE_TRANSITION_FLAG_CARETABLE_TO_CARETABLE;
			flags |= RUNE_TRANSITION_FLAG_ANY_TO_CARETABLE;
		}
	} else {
		if (rune_get_flag(dst_rune_flags, RUNE_FLAG_CARETABLE)) {
			flags |= RUNE_TRANSITION_FLAG_NONCARETABLE_TO_CARETABLE;
			flags |= RUNE_TRANSITION_FLAG_ANY_TO_CARETABLE;
		} else {
			flags |= RUNE_TRANSITION_FLAG_NONCARETABLE_TO_NONCARETABLE;
		}
	}

	Line *dst_line = txtbuf_get_line(buf, dst_pos->line_idx, tix);
	ASSERT(dst_line);
	if (dst_pos->col_idx == 0 && (dst_rune.value == '\n' || dst_rune.value == '\r' || dst_line->size == 0)) {
		flags |= RUNE_TRANSITION_FLAG_ANY_TO_EMPTYLINE;
	}

	if (!rune_get_flag(dst_rune_flags, RUNE_FLAG_BLANK)) {
		flags |= RUNE_TRANSITION_FLAG_ANY_TO_NONBLANK;
	}

	if (dst_pos->col_idx == 0 && (dst_rune.value == '\n' || dst_rune.value == '\r' || dst_line->size == 0)) {
		flags |= RUNE_TRANSITION_FLAG_ANY_TO_EMPTYLINE;
	}

	if (!rune_get_flag(src_rune_flags, RUNE_FLAG_BLANK) && rune_get_flag(dst_rune_flags, RUNE_FLAG_BLANK)) {
		flags |= RUNE_TRANSITION_FLAG_NONBLANK_TO_BLANK;
	}

	if (rune_get_flag(src_rune_flags, RUNE_FLAG_WORD) && rune_get_flag(dst_rune_flags, RUNE_FLAG_PUNCT)) {
		flags |= RUNE_TRANSITION_FLAG_WORD_TO_PUNCT;
	}

	if (rune_get_flag(src_rune_flags, RUNE_FLAG_PUNCT) && rune_get_flag(dst_rune_flags, RUNE_FLAG_WORD)) {
		flags |= RUNE_TRANSITION_FLAG_PUNCT_TO_WORD;
	}

	if (rune_get_flag(src_rune_flags, RUNE_FLAG_BLANK) && !rune_get_flag(dst_rune_flags, RUNE_FLAG_BLANK)) {
		flags |= RUNE_TRANSITION_FLAG_BLANK_TO_NONBLANK;
	}

	return flags;
}

inline static RuneTransitionFlag rune_transition_get_flag(RuneTransitionFlags flags, RuneTransitionFlag flag)
{
	RuneTransitionFlag result = flags & flag;

	return result;
}

inline static TxtPos txtbuf_last_caretable_pos(TxtBuf *buf, size_t line_idx, Tix *tix)
{
	TxtPos result = { .line_idx = line_idx, .col_idx = 0 };
	Line *line = txtbuf_get_line(buf, line_idx, tix);
	ASSERT(line);
	size_t line_size = line->size;
	if (line_size > 0) {
		result.col_idx = line_size - 1;

		while (result.col_idx > 0) {
			Rune dst_rune = txtbuf_get_rune(buf, result, tix);
			RuneFlags flags = txtbuf_get_rune_flags(buf, &result, dst_rune, tix);
			if (!rune_get_flag(flags, RUNE_FLAG_CARETABLE)) {
				--result.col_idx;
			} else {
				break;
			}
		}
	} else {
		result.col_idx = 0U;
	}

	return result;
}

static ErrCode txtbuf_index_lines(TxtBuf *buf, Tix *tix)
{
	ErrCode error = {};

	buf->lines_count = 0;

	// TODO(fredy): should it be a circular buffer?
	static uint8_t overhang_mask[64] = {
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	};

	size_t file_offset = 0;

	__m256i newline_needle = _mm256_set1_epi8('\n');
	__m256i complex_mask = _mm256_set1_epi8((char)0x80);
	size_t line_offset = 0;

	while (file_offset < buf->file.size) {
		Line *line = txtbuf_push_line(buf, tix);
		if (!line) {
			error = ERR_CODE_FAILED;
			ASSERT(false && "No room for more lines");
			break;
		}
		__m256i contains_complex = _mm256_setzero_si256();

		while (file_offset + 32 < buf->file.size) {
			__m256i batch = _mm256_loadu_si256((__m256i *)((buf->file.buf) + file_offset));

			__m256i test_newline = _mm256_cmpeq_epi8(batch, newline_needle);
			__m256i test_complex = _mm256_and_si256(batch, complex_mask);

			uint32_t newline_detected = (uint32_t)_mm256_movemask_epi8(test_newline);

			if (newline_detected) {
				uint32_t first_newline_idx = 0;
				uint_ctz(newline_detected, &first_newline_idx);

				__m256i mask_complex = _mm256_loadu_si256((__m256i *)(overhang_mask + 32 - first_newline_idx));

				test_complex = _mm256_and_si256(test_complex, mask_complex);
				contains_complex = _mm256_or_si256(contains_complex, test_complex);

				line->contains_complex_chars |= (uint8_t)!_mm256_testz_si256(contains_complex, contains_complex);

				file_offset += first_newline_idx;

				break;
			}

			contains_complex = _mm256_or_si256(contains_complex, test_complex);

			file_offset += 32;
		}

		ASSERT(file_offset < buf->file.size);
		uint32_t is_newline = ((char *)(buf->file.buf))[file_offset] == '\n';
		uint32_t is_last_byte = file_offset + 1 == buf->file.size;
		if (is_newline || is_last_byte) {
			ASSERT(buf->lines_count < LINES_COUNT_MAX);

			line->file_offset = line_offset;
			line->size = file_offset ? file_offset - line_offset + 1 : 0;

			line_offset = file_offset + 1;

			if (is_newline && is_last_byte) {
				line = txtbuf_push_line(buf, tix);
				if (!line) {
					error = ERR_CODE_FAILED;
					ASSERT(false && "No room for more lines");
					break;
				}
				line->file_offset = line_offset;
				line->size = 0;
			}
		} else if (((char *)(buf->file.buf))[file_offset] < 0) {
			line->contains_complex_chars = 1U;
		}

		++file_offset;
	}

	return error;
}

static ErrCode txtbuf_read_file(TxtBuf *buf, Tix *tix, const char *const path)
{
	ErrCode error = sys_file_read(path, &tix->files_arena, &buf->file);

	if (!error) {
		error = sys_file_get_last_write_time(path, &tix->txtbuf.file_time);
	} else {
		LOG_ERROR("The file %s could not be opened", path);
		ASSERT(false && "Failed to read the file");
	}

	return error;
}

static void tix_init(Tix *tix)
{
	pool_init(&tix->lines_pool, tix->lines_buf, sizeof(tix->lines_buf), sizeof(Line));

	arena_init(&tix->files_arena, sizeof(tix->files_buf), tix->files_buf);
	arena_init(&tix->renderer_arena, sizeof(tix->renderer_buf), tix->renderer_buf);

	tix->view.buf = &tix->txtbuf;
}
