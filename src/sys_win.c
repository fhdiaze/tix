#include <dwmapi.h>
#include <immintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "app.h"
#include "lib.h"
#include "sys.h"

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL

#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif

#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

#define LINES_PER_NOTCH 3
#define POINTS_PER_INCH 72

#define BG_COLOR 0x00202230U
#define FG_COLOR 0x00FFFF00U
#define ALPHA_MASK 0xFF000000U
#define RED_MASK 0x00FF0000U
#define GREEN_MASK 0x0000FF00U
#define BLUE_MASK 0x000000FFU
#define ALPHA_SHIFT 24U
#define RED_SHIFT 16U
#define GREEN_SHIFT 8U
#define BLUE_SHIFT 0U

#define ALPHA_BITS(c) ((c & ALPHA_MASK) >> ALPHA_SHIFT)
#define RED_BITS(c) ((c & RED_MASK) >> RED_SHIFT)
#define GREEN_BITS(c) ((c & GREEN_MASK) >> GREEN_SHIFT)
#define BLUE_BITS(c) ((c & BLUE_MASK) >> BLUE_SHIFT)

/**
 * @brief (0,0) is on the top left corner. Top-To-Bottom.
 * The byte order in a register (little endian) is AA RR GG BB
 */
typedef struct Bitmap {
	void *buf;
	size_t buf_size_byte;

	unsigned width_px;
	unsigned height_px;

	uint8_t pixel_size_byte;
} Bitmap;

typedef struct WinState {
	size_t buf_size_byte;
	void *buf;

	uint8_t is_running;
} WinState;

// TODO(fredy):  - remove this global
static unsigned long g_render_thread_id = 0;

static void bitmap_draw_border(Bitmap *bitmap, float min_x_px_f, float min_y_px_f, float width_px_f, float height_px_f,
                               uint32_t color_argb)
{
	assert(width_px_f > 1.0F);
	assert(height_px_f > 1.0F);
	assert(min_x_px_f >= 0.0F);
	assert(min_y_px_f >= 0.0F);
	assert(min_x_px_f < (float)bitmap->width_px);
	assert(min_y_px_f < (float)bitmap->height_px);
	assert(min_x_px_f + width_px_f <= (float)bitmap->width_px);
	assert(min_y_px_f + height_px_f <= (float)bitmap->height_px);

	unsigned min_x_px = (unsigned)floorf(min_x_px_f);
	unsigned min_y_px = (unsigned)floorf(min_y_px_f);
	size_t width_px = (unsigned)ceilf(width_px_f);
	size_t height_px = (unsigned)ceilf(height_px_f);

	size_t backbuf_pitch_size_byte = (size_t)bitmap->width_px * bitmap->pixel_size_byte;

	unsigned char *pixel_first_byte = (unsigned char *)bitmap->buf + (size_t)(min_x_px * bitmap->pixel_size_byte) +
	                                  min_y_px * backbuf_pitch_size_byte;
	unsigned char *last_pixel_first_byte = (unsigned char *)bitmap->buf + (size_t)(min_x_px * bitmap->pixel_size_byte) +
	                                       min_y_px * backbuf_pitch_size_byte + width_px * bitmap->pixel_size_byte +
	                                       height_px * backbuf_pitch_size_byte;

	// size_t pixel_first_byte = 0 + (size_t)(min_x_px * bitmap->pixel_size_byte) + min_y_px * backbuf_pitch_size_byte;
	// size_t last_pixel_first_byte = 0 + (size_t)(min_x_px * bitmap->pixel_size_byte) +
	//                                min_y_px * backbuf_pitch_size_byte + width_px * bitmap->pixel_size_byte +
	//                                height_px * backbuf_pitch_size_byte;

	uint32_t *pixel = nullptr;
	unsigned x = 0;
	unsigned y = 0;
	while (pixel_first_byte <= last_pixel_first_byte) {
		pixel = (uint32_t *)pixel_first_byte;
		*pixel = color_argb;

		if (y == 0) {
			if (x < width_px - 1) {
				pixel_first_byte += bitmap->pixel_size_byte;
				++x;
			} else {
				pixel_first_byte += backbuf_pitch_size_byte - (width_px - 1) * bitmap->pixel_size_byte;
				x = 0;
				++y;
			}
		} else if (y == height_px - 1) {
			if (x < width_px - 1) {
				pixel_first_byte += bitmap->pixel_size_byte;
				++x;
			} else {
				break;
			}
		} else {
			if (x == 0) {
				pixel_first_byte += (width_px - 1) * bitmap->pixel_size_byte;
				x += width_px - 1;
			} else {
				pixel_first_byte += backbuf_pitch_size_byte - (width_px - 1) * bitmap->pixel_size_byte;
				x = 0;
				++y;
			}
		}
	}
}

static uint32_t bitmap_copy_rect(unsigned char *src_buf, size_t src_width, size_t src_height, size_t src_pitch,
                                 size_t src_offset_x, size_t src_offset_y, unsigned char *dst_buf, size_t dst_width,
                                 size_t dst_height, size_t dst_pitch, size_t dst_offset_x, size_t dst_offset_y,
                                 size_t blit_width, size_t blit_height)
{
	uint32_t error_code = 0U;

	assert(src_offset_x + blit_width <= src_width);
	assert(src_offset_y + blit_height <= src_height);

	assert(dst_offset_x + blit_width <= dst_width);
	assert(dst_offset_y + blit_height <= dst_height);

	unsigned char *dst_ptr = dst_buf + dst_offset_x + dst_pitch * dst_offset_y;
	unsigned char *src_ptr = src_buf + src_offset_x + src_pitch * src_offset_y;

	for (size_t y = 0; y < blit_height; ++y) {
		for (size_t x = 0; x < blit_width; ++x) {
			*dst_ptr = *src_ptr;
			// *dst_ptr = 255;

			++dst_ptr;
			++src_ptr;
		}

		dst_ptr += dst_pitch - blit_width;
		src_ptr += src_pitch - blit_width;
	}

	return error_code;
}

/**
 * @brief Rasterizes a glyph, allocating its bitmap from the given arena.
 *
 * @return uint32_t 0 on success. Non-zero on failure, e.g. if the allocation fails.
 */
static uint32_t glyph_rasterize(HDC font_dc, uint32_t code, Arena *arena, unsigned ascent_y_byte,
                                unsigned char *dst_buf, size_t dst_width_byte, size_t dst_height_byte,
                                size_t dst_pitch_byte)
{
	uint32_t error_code = 0U;

	// rotation, shear, scale: { WORD fract; short value; }
	static const MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };

	unsigned char *glyph_buf = nullptr;
	GLYPHMETRICS glyph_metrics;
	DWORD glyph_size_byte = GetGlyphOutlineA(font_dc, code, GGO_GRAY8_BITMAP, &glyph_metrics, 0, nullptr, &identity);
	if (glyph_size_byte != GDI_ERROR && glyph_size_byte && glyph_size_byte <= dst_width_byte * dst_height_byte) {
		glyph_buf = arena_push_zero(arena, glyph_size_byte);

		if (glyph_buf) {
			glyph_size_byte = GetGlyphOutlineA(font_dc, code, GGO_GRAY8_BITMAP, &glyph_metrics, glyph_size_byte,
			                                   glyph_buf, &identity);
		} else {
			error_code = 1U;
		}
	}

	if (glyph_size_byte != GDI_ERROR) {
		if (glyph_size_byte) {
			assert(dst_width_byte >= glyph_metrics.gmBlackBoxX);
			assert(dst_height_byte >= glyph_metrics.gmBlackBoxY);
			if (dst_width_byte >= glyph_metrics.gmBlackBoxX && dst_height_byte >= glyph_metrics.gmBlackBoxY) {
				size_t dst_offset_x_byte = (dst_width_byte - glyph_metrics.gmBlackBoxX) / 2;
				size_t dst_offset_y_byte = (size_t)((long)ascent_y_byte - glyph_metrics.gmptGlyphOrigin.y);

				assert(dst_offset_y_byte < dst_height_byte);
				if (dst_offset_y_byte < dst_height_byte) {
					uint32_t glyph_row_padding_byte =
						(sizeof(DWORD) - (size_t)glyph_metrics.gmBlackBoxX % sizeof(DWORD)) % sizeof(DWORD);
					unsigned glyph_width_byte = glyph_metrics.gmBlackBoxX + glyph_row_padding_byte;

					unsigned glyph_offset_x_byte = 0;
					signed glyph_offset_y_byte = 0;

					assert(glyph_offset_y_byte >= 0);
					assert(glyph_offset_y_byte < (signed)dst_height_byte);

					if (glyph_offset_y_byte >= 0 && glyph_offset_y_byte < (signed)dst_height_byte) {
						bitmap_copy_rect(glyph_buf, glyph_width_byte, glyph_metrics.gmBlackBoxY, glyph_width_byte,
						                 glyph_offset_x_byte, (uint32_t)glyph_offset_y_byte, dst_buf, dst_width_byte,
						                 dst_height_byte, dst_pitch_byte, dst_offset_x_byte, dst_offset_y_byte,
						                 glyph_metrics.gmBlackBoxX, glyph_metrics.gmBlackBoxY);
						// bitmap_copy_rect(glyph_buf, 1, dst_height_byte, 1, 0, 0, dst_buf, dst_width_byte,
						//                  dst_height_byte, dst_pitch_byte, 0, 0, 1, dst_height_byte);

					} else {
						error_code = 1U;
					}
				} else {
					error_code = 1U;
				}
			} else {
				error_code = 1U;
			}
		} else {
			return 1U;
		}
	} else {
		error_code = 1U;
	}

	return error_code;
}

// static inline uint32_t bitmap_draw_raster()
// {
// 	unsigned glyph_width_px = glyph_metrics.gmBlackBoxX;
// 	unsigned glyph_height_px = glyph_metrics.gmBlackBoxY;

// 	unsigned min_y_px = (unsigned)((signed)baseline_y_px - glyph_metrics.gmptGlyphOrigin.y);
// 	unsigned min_x_px = (unsigned)((signed)tile_min_x_px + glyph_metrics.gmptGlyphOrigin.x);

// 	unsigned glyph_blit_width_px = min(tile_width_px, glyph_width_px);
// 	unsigned glyph_blit_height_px = min(tile_height_px, glyph_height_px);

// 	uint32_t row_padding_byte = (sizeof(DWORD) - (size_t)glyph_width_px % sizeof(DWORD)) % sizeof(DWORD);
// 	uint32_t glyph_pitch_byte = glyph_width_px + row_padding_byte;

// 	// in memory: BB GG RR AA
// 	uint8_t *dst_px_ptr = (unsigned char *)backbuf.buf + (size_t)(min_x_px * backbuf.pixel_size_byte) +
// 	                      backbuf_pitch_size_byte * min_y_px;
// 	unsigned char *coverage_ptr = glyph_buf;

// 	for (size_t y = 0; y < glyph_blit_height_px; ++y) {
// 		for (size_t x = 0; x < glyph_blit_width_px; ++x) {
// 			uint8_t blend_factor = (*coverage_ptr * 255U) / 64U;

// 			// x/255 ~ x/256 + x/256² = (x + x/256) / 256

// 			// blue
// 			uint32_t blended = 0x00U * blend_factor + *dst_px_ptr * (255 - blend_factor);
// 			*dst_px_ptr = (uint8_t)((blended + 1U + (blended >> 8U)) >> 8U);

// 			// green
// 			++dst_px_ptr;
// 			blended = 0xFFU * blend_factor + *dst_px_ptr * (255 - blend_factor);
// 			*dst_px_ptr = (uint8_t)((blended + 1U + (blended >> 8U)) >> 8U);

// 			// red
// 			++dst_px_ptr;
// 			blended = 0xFFU * blend_factor + *dst_px_ptr * (255 - blend_factor);
// 			*dst_px_ptr = (uint8_t)((blended + 1U + (blended >> 8U)) >> 8U);

// 			// alpha
// 			++dst_px_ptr;

// 			++dst_px_ptr;
// 			++coverage_ptr;
// 		}

// 		dst_px_ptr += backbuf_pitch_size_byte - (size_t)glyph_blit_width_px * backbuf.pixel_size_byte;
// 		coverage_ptr += glyph_pitch_byte - glyph_blit_width_px;
// 	}
// }

/**
 * @brief max_x_px_f and max_y_px_f are not included
 *
 * @param bitmap
 * @param min_x_px_f
 * @param min_y_px_f
 * @param max_x_px_f
 * @param max_y_px_f
 * @param red
 * @param green
 * @param blue
 */
static void bitmap_draw_rectangle(Bitmap *bitmap, float min_x_px_f, float min_y_px_f, float max_x_px_f,
                                  float max_y_px_f, float red, float green, float blue)
{
	assert(min_x_px_f < max_x_px_f);
	assert(min_y_px_f < max_y_px_f);
	assert(min_x_px_f >= 0.0F);
	assert(min_y_px_f >= 0.0F);
	assert(min_x_px_f < (float)bitmap->width_px);
	assert(min_y_px_f < (float)bitmap->height_px);
	assert(max_x_px_f <= (float)bitmap->width_px);
	assert(max_y_px_f <= (float)bitmap->height_px);

	unsigned min_x_px = (unsigned)floorf(min_x_px_f);
	unsigned min_y_px = (unsigned)floorf(min_y_px_f);
	unsigned max_x_px = (unsigned)ceilf(max_x_px_f);
	unsigned max_y_px = (unsigned)ceilf(max_y_px_f);

	uint32_t red_bits = (uint32_t)roundf(red * 255.0F);
	uint32_t green_bits = (uint32_t)roundf(green * 255.0F);
	uint32_t blue_bits = (uint32_t)roundf(blue * 255.0F);
	uint32_t rgb_color = red_bits << 16UL | green_bits << 8UL | blue_bits;

	uint32_t pitch_size_byte = bitmap->width_px * bitmap->pixel_size_byte;

	unsigned char *pixel_first_byte = (unsigned char *)bitmap->buf + (size_t)(min_x_px * bitmap->pixel_size_byte) +
	                                  (size_t)(min_y_px * pitch_size_byte);
	uint32_t *pixel = nullptr;
	for (unsigned y = min_y_px; y < max_y_px; ++y) {
		for (unsigned x = min_x_px; x < max_x_px; ++x) {
			pixel = (uint32_t *)pixel_first_byte;
			*pixel = rgb_color;
			pixel_first_byte += bitmap->pixel_size_byte;
		}

		pixel_first_byte += pitch_size_byte - (max_x_px - min_x_px) * bitmap->pixel_size_byte;
	}
}

/**
 * @brief Handles window lifecycle events
 *
 * @param window
 * @param msg
 * @param wparam
 * @param lparam
 * @return
 */
static LRESULT CALLBACK window_procedure(HWND win_handle, [[__maybe_unused__]] UINT msg,
                                         [[__maybe_unused__]] WPARAM wparam, [[__maybe_unused__]] LPARAM lparam)
{
	LRESULT result = 0;

	switch (msg) {
	case WM_CLOSE:
	case WM_DESTROY: {
		PostQuitMessage(0);
	} break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_SIZE: {
		PostThreadMessageA(g_render_thread_id, msg, wparam, lparam);
	} break;
	default: {
		result = DefWindowProcA(win_handle, msg, wparam, lparam);
	} break;
	}

	return result;
}

static inline uint32_t file_get_last_write_time(const char *const file_path, FILETIME *result)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesExA(file_path, GetFileExInfoStandard, &data)) {
		LOG_ERROR("unable to check the timestamp of the file: %s", file_path);
		return 0U;
	}

	*result = data.ftLastWriteTime;

	return 1U;
}

static uint32_t file_free_memory(void *buf)
{
	uint32_t result = 0U;

	if (buf) {
		result = (uint32_t)VirtualFree(buf, 0, MEM_RELEASE);
	}

	return result;
}

static ReadFileResult sys_file_read(const char *const path)
{
	ReadFileResult result = {};

	HANDLE handle =
		CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER filesize_struct;
		if (GetFileSizeEx(handle, &filesize_struct)) {
			uint32_t file_size_byte = (uint32_t)(filesize_struct.QuadPart);

			// TODO(fredy): if the file is too big, use file mapping?
			result.buf = VirtualAlloc(nullptr, file_size_byte, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.buf) {
				DWORD read_size_byte = 0;
				if (ReadFile(handle, result.buf, file_size_byte, &read_size_byte, nullptr) ||
				    read_size_byte == file_size_byte) {
					result.size_byte = file_size_byte;
				} else {
					LOG_ERROR("failed to read the file: %s", path);

					file_free_memory(result.buf);

					result.buf = nullptr;
					result.size_byte = 0;
				}
			} else {
				LOG_ERROR("failed to allocate memory for the content of file: %s", path);
			}
		} else {
			LOG_ERROR("failed to get the size of the file: %s", path);
		}

		CloseHandle(handle);
	} else {
		LOG_ERROR("failed to open the file: %s", path);
	}

	return result;
}

inline static void keyboard_process_message(KeyState *key_state, uint32_t is_down)
{
	if (key_state->ended_down != is_down) {
		key_state->ended_down = (uint8_t)is_down;
		++key_state->half_transition_count;
	}
}

static unsigned long WINAPI render_run(void *param)
{
	HWND window = (HWND)param;

	HDC dc_handle = GetDC(window);
	if (dc_handle) {
		WinState win_state = {
			.is_running = 1U,
		};

		Storage storage = {
			.buf_size_byte = MB_TO_BYTE(128ULL),
		};

		Bitmap backbuf = {
			.pixel_size_byte = 4,
		};
		BITMAPINFO bitmap_info = { .bmiHeader = {
									   .biSize = sizeof(BITMAPINFOHEADER),
									   .biPlanes = 1,
									   .biBitCount = CHAR_BIT * backbuf.pixel_size_byte,
									   .biCompression = BI_RGB,
								   } };

		win_state.buf_size_byte = storage.buf_size_byte;
		win_state.buf =
			// NOLINTNEXTLINE(performance-no-int-to-ptr): fixed base address for deterministic pointers across runs
			VirtualAlloc(MEMORY_BASE_ADDRESS, win_state.buf_size_byte, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (win_state.buf) {
			storage.buf = (unsigned char *)win_state.buf;
		}

		Tix *tix = (Tix *)storage.buf;
		tix->caret_mode = CARET_MODE_NORMAL;
		tix->caret_pos.row = 0;
		tix->caret_pos.col = 0;

		Arena *arena = &tix->arena;

		if (!storage.is_initialized) {
			arena_init(arena, storage.buf_size_byte - sizeof(Tix), (unsigned char *)storage.buf + sizeof(Tix));

			assert(arena->buf);

			storage.is_initialized = 1U;
		}

		// pt: physical unit - 1 point = 1/72 inch
		int font_size_pt = 16;
		int dpi_y = GetDeviceCaps(dc_handle, LOGPIXELSY);
		int font_size_px = MulDiv(font_size_pt, dpi_y, POINTS_PER_INCH);
		HDC font_dc = CreateCompatibleDC(nullptr);
		HFONT font = CreateFontA(-font_size_px, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS,
		                         CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
		SelectObject(font_dc, font);
		TEXTMETRICA text_metrics;
		GetTextMetricsA(font_dc, &text_metrics);

		unsigned tile_width_px = (unsigned)abs(text_metrics.tmAveCharWidth);
		unsigned tile_height_px = (unsigned)abs(text_metrics.tmHeight) + (unsigned)abs(text_metrics.tmExternalLeading);
		unsigned tile_ascent_px = (unsigned)abs(text_metrics.tmAscent); // Includes the internal leading

		SIZE size;
		GetTextExtentPoint32W(dc_handle, L"@", 1, &size);
		tile_width_px = (unsigned)max((long)tile_width_px, size.cx);
		tile_height_px = (unsigned)max((long)tile_height_px, size.cy);

		GetTextExtentPoint32W(dc_handle, L"M", 1, &size);
		tile_width_px = (unsigned)max((long)tile_width_px, size.cx);
		tile_height_px = (unsigned)max((long)tile_height_px, size.cy);

		GetTextExtentPoint32W(dc_handle, L"g", 1, &size);
		tile_width_px = (unsigned)max((long)tile_width_px, size.cx);
		tile_height_px = (unsigned)max((long)tile_height_px, size.cy);

		unsigned tile_size_byte = tile_width_px * tile_height_px;

		// TODO(fredy): what is the correct size for this?
		size_t font_arena_size_byte = MB_TO_BYTE(16ULL);
		void *font_arena_buf = arena_push(arena, font_arena_size_byte);
		Arena font_arena;
		if (font_arena_buf) {
			arena_init(&font_arena, font_arena_size_byte, font_arena_buf);
		}

		ArenaMark init_mark = arena_mark(arena);

		unsigned char *glyph_atlas = nullptr;
		size_t glyph_atlas_size_byte = (size_t)DIRECT_CODE_POINTS_COUNT * tile_width_px * tile_height_px;
		if (font_arena.buf) {
			glyph_atlas = arena_push_zero(&font_arena, glyph_atlas_size_byte);
			if (glyph_atlas) {
				for (char p = MIN_DIRECT_CODE_POINT; p <= MAX_DIRECT_CODE_POINT; ++p) {
					GlyphIdx glyph_idx = { .value = (uint32_t)p - MIN_DIRECT_CODE_POINT };
					glyph_rasterize(font_dc, (uint32_t)p, arena, tile_ascent_px,
					                glyph_atlas + (size_t)(glyph_idx.value * tile_size_byte), tile_width_px,
					                tile_height_px, tile_width_px);
				}
			}
		}

		arena_rewind(&init_mark);

		const char *file_path = "./test.txt";

		ReadFileResult file = {};
		FILETIME file_previous_write_time = {};

		constexpr uint32_t max_lines = 1000000;
		Line *file_lines = ARENA_PUSH_ARRAY(arena, Line, max_lines);
		size_t file_lines_count = 0;

		LARGE_INTEGER performance_frequency;
		QueryPerformanceFrequency(&performance_frequency);

		TixInput tix_input = {};

		while (win_state.is_running) {
			LARGE_INTEGER wall_clock_at_start;
			QueryPerformanceCounter(&wall_clock_at_start);

			// =============================================================================
			// Input
			// =============================================================================
			tix_input.mouse_notches = 0;

			// Process POSTED messages
			MSG msg;
			// TODO(fredy): limit the iterations of this loop
			// TODO(fredy): deal with WM_DPICHANGED and WM_GETDPISCALEDSIZE
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				switch (msg.message) {
				case WM_QUIT: {
					// The WM_QUIT message is not associated with a window and therefore will never be received through a
					// window's window procedure. It is retrieved only by the GetMessage or PeekMessage functions
					win_state.is_running = 0U;
				} break;
				case WM_MOUSEWHEEL: {
					int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
					tix_input.mouse_notches += delta / WHEEL_DELTA;
				} break;
				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
				case WM_KEYDOWN:
				case WM_KEYUP: {
					size_t vk_code = (size_t)msg.wParam;
					size_t key_stroke_info = (size_t)msg.lParam;
					uint32_t was_down = (key_stroke_info & (1U << 30U)) != 0;
					uint32_t is_down = (key_stroke_info & (1UL << 31UL)) == 0;
					if (was_down != is_down) {
						if (vk_code == 'J') {
							keyboard_process_message(&tix_input.move_down, is_down);
						} else if (vk_code == 'K') {
							keyboard_process_message(&tix_input.move_up, is_down);
						} else if (vk_code == 'H') {
							keyboard_process_message(&tix_input.move_left, is_down);
						} else if (vk_code == 'L') {
							keyboard_process_message(&tix_input.move_right, is_down);
						}
					}
				} break;
				case WM_CHAR: {
					(void)0;
				} break;
				case WM_SIZE: {
					// Why do we send this msg from the window thread if we are not doing anything?
					// Ans: this is a wake up signal. It is not useful while the render loop spins
					// on PeekMessage; it only wakes the render thread once the spin is replaced by
					// a blocking wait (GetMessage / MsgWaitForMultipleObjectsEx)

					// Event-driven file watch: use ReadDirectoryChangesW (overlapped, with an event HANDLE)
					// or FindFirstChangeNotificationA on the file's directory, and add that handle to the
					// array passed to MsgWaitForMultipleObjectsEx. Then the thread only wakes for an actual
					// change — no polling at all, but more plumbing (need to re-arm the watch after each
					// notification, handle the directory vs. file distinction, etc.)
				} break;
				default: {
					assert(false && "unexpected message arrived to the render thread");
				} break;
				}
			}

			// =============================================================================
			// Window
			// =============================================================================
			RECT client_rect;
			GetClientRect(window, &client_rect);

			assert(client_rect.right - client_rect.left >= 0);
			assert(client_rect.bottom - client_rect.top >= 0);

			unsigned new_width_px = (unsigned)(client_rect.right - client_rect.left);
			unsigned new_height_px = (unsigned)(client_rect.bottom - client_rect.top);

			if (new_width_px != backbuf.width_px || new_height_px != backbuf.height_px) {
				void *new_buf = nullptr;
				size_t new_buf_size_byte =
					(size_t)new_width_px * (size_t)new_height_px * (size_t)backbuf.pixel_size_byte;
				if (new_buf_size_byte > 0) {
					new_buf = VirtualAlloc(nullptr, new_buf_size_byte, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
					if (!new_buf) {
						LOG_ERROR("unable to allocate %zu bytes for the backbuffer", new_buf_size_byte);
						assert(false && "unable to allocate memory for the backbuffer");
					}
				}

				if (new_buf || new_buf_size_byte == 0) {
					if (backbuf.buf && !VirtualFree(backbuf.buf, 0, MEM_RELEASE)) {
						LOG_ERROR("unable to deallocate memory of the previous backbuffer");
						assert(false && "unable to deallocate memory of the previous backbuffer");
					}

					backbuf.buf = new_buf;
					backbuf.buf_size_byte = new_buf_size_byte;
					backbuf.width_px = new_width_px;
					backbuf.height_px = new_height_px;
				} else {
					LOG_ERROR("unable to allocate %zu bytes for the backbuffer", new_buf_size_byte);
					assert(false && "unable to allocate memory for the backbuffer");
				}
			}

			// =============================================================================
			// Update
			// =============================================================================
			size_t backbuf_pitch_size_byte = (size_t)backbuf.width_px * backbuf.pixel_size_byte;

			if (backbuf.buf) {
				uint32_t was_file_updated = 0U;
				FILETIME current_write_time = {};
				if (!file.buf) {
					file = sys_file_read(file_path);
					if (file.buf) {
						file_get_last_write_time(file_path, &file_previous_write_time);
						was_file_updated = 1U;
					} else {
						LOG_ERROR("The file %s could not be open\n", file_path);
					}
				} else if (file_get_last_write_time(file_path, &current_write_time)) {
					if (CompareFileTime(&current_write_time, &file_previous_write_time) > 0) {
						file_free_memory(file.buf);

						file = sys_file_read(file_path);

						if (file.buf) {
							was_file_updated = 1U;
							file_previous_write_time = current_write_time;
						} else {
							LOG_ERROR("The file %s could not be open\n", file_path);
						}
					}
				}

				if (was_file_updated) {
					file_lines_count = 0;

					// TODO(fredy): should it be a circular buffer?
					static uint8_t overhang_mask[64] = {
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
						0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
						0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
					};
					char *buf = (char *)file.buf;
					size_t remaining_byte_count = file.size_byte;

					__m256i newline_needle = _mm256_set1_epi8('\n');
					__m256i complex_mask = _mm256_set1_epi8((char)0x80);
					size_t line_start_idx = 0;
					size_t last_byte_idx = 0;

					while (file_lines_count < max_lines && remaining_byte_count) {
						__m256i contains_complex = _mm256_setzero_si256();

						while (remaining_byte_count > 32) {
							__m256i batch = _mm256_loadu_si256((__m256i *)buf);

							__m256i test_newline = _mm256_cmpeq_epi8(batch, newline_needle);
							__m256i test_complex = _mm256_and_si256(batch, complex_mask);

							uint32_t newline_detected = (uint32_t)_mm256_movemask_epi8(test_newline);

							if (newline_detected) {
								uint32_t first_newline_idx = 0;
								uint_ctz(newline_detected, &first_newline_idx);

								__m256i mask_complex =
									_mm256_loadu_si256((__m256i *)(overhang_mask + 32 - first_newline_idx));

								test_complex = _mm256_and_si256(test_complex, mask_complex);
								contains_complex = _mm256_or_si256(contains_complex, test_complex);

								file_lines[file_lines_count].contains_complex_chars |=
									(uint8_t)!_mm256_testz_si256(contains_complex, contains_complex);

								buf += first_newline_idx;
								remaining_byte_count -= first_newline_idx;

								break;
							}

							contains_complex = _mm256_or_si256(contains_complex, test_complex);

							buf += 32;
							remaining_byte_count -= 32;
						}

						if (buf[0] == '\n' || remaining_byte_count == 1) {
							last_byte_idx = (size_t)(buf - (char *)file.buf);

							file_lines[file_lines_count].newline_idx = last_byte_idx;
							file_lines[file_lines_count].start_idx = line_start_idx;

							line_start_idx = last_byte_idx + 1;

							++file_lines_count;
						} else if (buf[0] < 0) {
							file_lines[file_lines_count].contains_complex_chars = 1U;
						}

						++buf;
						--remaining_byte_count;
					}
				}

				size_t grid_height_tile = (size_t)floorf((float)backbuf.height_px / (float)tile_height_px);
				size_t grid_width_tile = backbuf.width_px / tile_width_px;

				// Move cursor
				uint32_t was_caret_moved = 0U;
				if (tix_input.move_up.ended_down && tix->caret_pos.row > 0) {
					--tix->caret_pos.row;
					was_caret_moved = 1U;
				}

				if (tix_input.move_down.ended_down && tix->caret_pos.row + 1 < file_lines_count) {
					++tix->caret_pos.row;
					was_caret_moved = 1U;
				}

				size_t new_line_col =
					file_lines[tix->caret_pos.row].newline_idx - file_lines[tix->caret_pos.row].start_idx;
				if (new_line_col > 0 &&
				    *((unsigned char *)file.buf + file_lines[tix->caret_pos.row].newline_idx - 1) == '\r') {
					--new_line_col;
				}

				if (tix_input.move_left.ended_down && tix->caret_pos.col > 0) {
					--tix->caret_pos.col;
					was_caret_moved = 1U;
				}

				if (tix_input.move_right.ended_down && tix->caret_pos.col + 1 < new_line_col) {
					++tix->caret_pos.col;
					was_caret_moved = 1U;
				}

				size_t caret_col = tix->caret_pos.col;
				if (caret_col >= new_line_col) {
					if (new_line_col > 0) {
						caret_col = new_line_col - 1;
					} else {
						caret_col = new_line_col;
					}
				}

				if (was_caret_moved && tix->caret_pos.row < tix->scroll_offset) {
					tix->scroll_offset = tix->caret_pos.row;
				}

				if (was_caret_moved && tix->caret_pos.row >= tix->scroll_offset + grid_height_tile) {
					tix->scroll_offset += tix->scroll_offset + grid_height_tile - tix->caret_pos.row + 1;
				}

				// Process mouse wheel
				int64_t new_scroll_offset = (int64_t)tix->scroll_offset;
				new_scroll_offset -= LINES_PER_NOTCH * (int64_t)tix_input.mouse_notches;
				new_scroll_offset = min(new_scroll_offset, (int64_t)file_lines_count - 1);
				new_scroll_offset = max(new_scroll_offset, 0);

				assert(new_scroll_offset >= 0);

				tix->scroll_offset = (size_t)new_scroll_offset;

				// =============================================================================
				// Segmentation
				// =============================================================================

				// =============================================================================
				// Layout
				// =============================================================================
				bitmap_draw_rectangle(&backbuf, 0.0F, 0.0F, (float)backbuf.width_px, (float)backbuf.height_px,
				                      RED_BITS(BG_COLOR) / 255.0F, GREEN_BITS(BG_COLOR) / 255.0F,
				                      BLUE_BITS(BG_COLOR) / 255.0F);
				unsigned tile_row = 0;
				for (size_t line_idx = tix->scroll_offset; line_idx < file_lines_count && tile_row < grid_height_tile;
				     ++line_idx) {
					// =============================================================================
					// Shaping
					// =============================================================================

					// Sometimes multiple codepoints are merged into one glyph

					unsigned tile_col = 0;
					unsigned tile_min_y_px = tile_height_px * tile_row;
					char *p = (char *)file.buf + file_lines[line_idx].start_idx;
					GlyphIdx glyph_idx = {};
					unsigned char *glyph_buf = nullptr;
					uint32_t bg_color = BG_COLOR;
					uint32_t fg_color = FG_COLOR;
					while (p <= (char *)file.buf + file_lines[line_idx].newline_idx && tile_col < grid_width_tile) {
						unsigned tile_min_x_px = tile_col * tile_width_px;
						char c = *p;

						if (tile_col == caret_col && line_idx == tix->caret_pos.row) {
							fg_color = BG_COLOR;
							bg_color = FG_COLOR;

							if (c == '\r' || c == '\n') {
								c = ' ';
							}
						} else {
							fg_color = FG_COLOR;
							bg_color = BG_COLOR;
						}

						if (c >= MIN_DIRECT_CODE_POINT && c <= MAX_DIRECT_CODE_POINT) {
							glyph_idx.value = (unsigned char)c - MIN_DIRECT_CODE_POINT;
							glyph_buf = glyph_atlas + (size_t)glyph_idx.value * tile_size_byte;

							// TODO(fredy): what happen with width 1.5F?

							// in memory: BB GG RR AA
							uint8_t *dst_px_ptr = (unsigned char *)backbuf.buf +
							                      (size_t)(tile_min_x_px * backbuf.pixel_size_byte) +
							                      backbuf_pitch_size_byte * tile_min_y_px;
							unsigned char *coverage_ptr = glyph_buf;

							// TODO(fredy): should I use SIMD here?
							for (size_t y = 0; y < tile_height_px; ++y) {
								for (size_t x = 0; x < tile_width_px; ++x) {
									float blend_factor = (float)(*coverage_ptr) / 64.0F;

									// blue
									float blended = (float)BLUE_BITS(fg_color) * blend_factor +
									                (float)BLUE_BITS(bg_color) * (1.0F - blend_factor);
									*dst_px_ptr = (uint8_t)(blended + 0.5F);

									// green
									++dst_px_ptr;
									blended = (float)GREEN_BITS(fg_color) * blend_factor +
									          (float)GREEN_BITS(bg_color) * (1.0F - blend_factor);
									*dst_px_ptr = (uint8_t)(blended + 0.5F);

									// red
									++dst_px_ptr;
									blended = (float)RED_BITS(fg_color) * blend_factor +
									          (float)RED_BITS(bg_color) * (1.0F - blend_factor);
									*dst_px_ptr = (uint8_t)(blended + 0.5F);

									// alpha
									++dst_px_ptr;

									++dst_px_ptr;
									++coverage_ptr;
								}

								dst_px_ptr += backbuf_pitch_size_byte - (size_t)tile_width_px * backbuf.pixel_size_byte;

								// bitmap_draw_border(&backbuf, (float)cell_min_x_px, (float)cell_min_y_px,
								//                    (float)cell_blit_width_px, (float)cell_blit_height_px,
								//                    0xFFFFFFU);
							}
						}

						++tile_col;
						++p;
					}

					++tile_row;
				}
			}

			// =============================================================================
			// Rasterization
			// =============================================================================
			// Store atlas tiles as 8-bit coverage/alpha, not pre-coloured RGB. Same reasoning as the GPU shader:
			// one grayscale glyph tile serves any foreground color, computed at blend time
			// (out = bg + coverage * (fg - bg)), rather than re-rasterizing per color.

			// =============================================================================
			// Composition
			// =============================================================================

			// =============================================================================
			// Present
			// =============================================================================
			LARGE_INTEGER wall_clock_at_end;
			QueryPerformanceCounter(&wall_clock_at_end);

			float frame_time_s = (float)(wall_clock_at_end.QuadPart - wall_clock_at_start.QuadPart) /
			                     (float)performance_frequency.QuadPart;

			char window_title[256];
			(void)snprintf(window_title, sizeof(window_title), "tix - ft: %fms, fps: %f",
			               (double)(1000.0F * frame_time_s), 1.0 / (double)frame_time_s);

			SetWindowTextA(window, window_title);

			// TODO(fredy): Get the swap chain's back buffer (IDXGISwapChain::GetBuffer), and copy backbuf.buf into it.
			bitmap_info.bmiHeader.biWidth = (long)backbuf.width_px;
			bitmap_info.bmiHeader.biHeight = -(long)backbuf.height_px;
			SetDIBitsToDevice(dc_handle, 0, 0, backbuf.width_px, backbuf.height_px, 0, 0, 0, backbuf.height_px,
			                  backbuf.buf, &bitmap_info, DIB_RGB_COLORS);
		}
	} else {
		LOG_ERROR("error getting the device context");
	}

	ExitProcess(0);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	LOG_INFO("%p", (void *)hInstance);
	LOG_INFO("%p", (void *)hPrevInstance);
	LOG_INFO("%p", (void *)lpCmdLine);
	LOG_INFO("%d", nShowCmd);

	LOG_INFO("Starting the editor\n");

	COLORREF text_color = RGB(220, 220, 220);
	COLORREF background_color = RGB(32, 34, 48);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	WNDCLASSA win_class = {
		.style = CS_OWNDC,
		.hInstance = hInstance,
		.lpszClassName = "tix",
		.lpfnWndProc = window_procedure,
	};
	if (!RegisterClassA(&win_class)) {
		LOG_ERROR("error registering the window class");
		return EXIT_FAILURE;
	}

	HWND win_handle = CreateWindowExA(0, win_class.lpszClassName, "tix", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
	                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance,
	                                  nullptr);
	if (!win_handle) {
		LOG_ERROR("error creating the window");
		return EXIT_FAILURE;
	}

	DwmSetWindowAttribute(win_handle, DWMWA_CAPTION_COLOR, &background_color, sizeof(background_color));
	DwmSetWindowAttribute(win_handle, DWMWA_TEXT_COLOR, &text_color, sizeof(text_color));

	ShowWindow(win_handle, nShowCmd);

	CreateThread(nullptr, 0, render_run, win_handle, 0, &g_render_thread_id);

	for (;;) {
		MSG msg = {};
		GetMessageA(&msg, nullptr, 0, 0);
		TranslateMessage(&msg);

		if (msg.message == WM_CHAR || msg.message == WM_KEYDOWN || msg.message == WM_KEYUP || msg.message == WM_QUIT ||
		    msg.message == WM_SIZE || msg.message == WM_MOUSEWHEEL) {
			PostThreadMessageA(g_render_thread_id, msg.message, msg.wParam, msg.lParam);
		} else {
			// Sends the msg to WinProc
			DispatchMessageA(&msg);
		}
	}

	return EXIT_SUCCESS;
}
