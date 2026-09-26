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

typedef struct WinState {
	Storage storage;

	uint8_t is_running;
} WinState;

// TODO(fredy):  - remove this global
static unsigned long g_render_thread_id = 0;

static void bitmap_draw_border(Bitmap *bitmap, float rect_x_px, float rect_y_px, float rect_width_px,
                               float rect_height_px, uint32_t color_argb)
{
	ASSERT(rect_width_px > 1.0F);
	ASSERT(rect_height_px > 1.0F);
	ASSERT(rect_x_px >= 0.0F);
	ASSERT(rect_y_px >= 0.0F);
	ASSERT(rect_x_px < (float)bitmap->width_px);
	ASSERT(rect_y_px < (float)bitmap->height_px);
	ASSERT(rect_x_px + rect_width_px <= (float)bitmap->width_px);
	ASSERT(rect_y_px + rect_height_px <= (float)bitmap->height_px);

	unsigned rect_x_idx_min = (unsigned)floorf(rect_x_px);
	unsigned rect_y_idx_min = (unsigned)floorf(rect_y_px);
	size_t x_count = (unsigned)ceilf(rect_width_px);
	size_t y_count = (unsigned)ceilf(rect_height_px);

	size_t bitmap_pitch_size = (size_t)bitmap->width_px * PIXEL_SIZE;

	unsigned char *pixel_offset =
		(unsigned char *)bitmap->buf + (size_t)(rect_x_idx_min * PIXEL_SIZE) + rect_y_idx_min * bitmap_pitch_size;
	unsigned char *last_pixel_offset = (unsigned char *)bitmap->buf + (size_t)(rect_x_idx_min * PIXEL_SIZE) +
	                                   rect_y_idx_min * bitmap_pitch_size + x_count * PIXEL_SIZE +
	                                   y_count * bitmap_pitch_size;

	uint32_t *pixel = nullptr;
	unsigned x = 0;
	unsigned y = 0;
	while (pixel_offset <= last_pixel_offset) {
		pixel = (uint32_t *)pixel_offset;
		*pixel = color_argb;

		if (y == 0) {
			if (x < x_count - 1) {
				pixel_offset += PIXEL_SIZE;
				++x;
			} else {
				pixel_offset += bitmap_pitch_size - (x_count - 1) * PIXEL_SIZE;
				x = 0;
				++y;
			}
		} else if (y == y_count - 1) {
			if (x < x_count - 1) {
				pixel_offset += PIXEL_SIZE;
				++x;
			} else {
				break;
			}
		} else {
			if (x == 0) {
				pixel_offset += (x_count - 1) * PIXEL_SIZE;
				x += x_count - 1;
			} else {
				pixel_offset += bitmap_pitch_size - (x_count - 1) * PIXEL_SIZE;
				x = 0;
				++y;
			}
		}
	}
}

static uint32_t mem_copy_rect(unsigned char *src_buf, size_t src_size_x, size_t src_size_y, size_t src_pitch_size,
                              size_t src_offset_x, size_t src_offset_y, unsigned char *dst_buf, size_t dst_size_x,
                              size_t dst_size_y, size_t dst_pitch_size, size_t dst_offset_x, size_t dst_offset_y,
                              size_t blit_size_x, size_t blit_size_y)
{
	uint32_t error_code = 0U;

	ASSERT(src_offset_x + blit_size_x <= src_size_x);
	ASSERT(src_offset_y + blit_size_y <= src_size_y);

	ASSERT(dst_offset_x + blit_size_x <= dst_size_x);
	ASSERT(dst_offset_y + blit_size_y <= dst_size_y);

	unsigned char *dst_ptr = dst_buf + dst_offset_x + dst_pitch_size * dst_offset_y;
	unsigned char *src_ptr = src_buf + src_offset_x + src_pitch_size * src_offset_y;

	for (size_t y = 0; y < blit_size_y; ++y) {
		for (size_t x = 0; x < blit_size_x; ++x) {
			*dst_ptr = *src_ptr;

			++dst_ptr;
			++src_ptr;
		}

		dst_ptr += dst_pitch_size - blit_size_x;
		src_ptr += src_pitch_size - blit_size_x;
	}

	return error_code;
}

/**
 * @brief Rasterizes a glyph, allocating its bitmap from the given arena.
 *
 * @return uint32_t 0 on success. Non-zero on failure, e.g. if the allocation fails.
 */
static uint32_t glyph_rasterize(HDC font_dc, uint32_t glyph_code, Arena *arena, unsigned ascent_size,
                                unsigned char *dst_buf, size_t dst_size_x, size_t dst_size_y, size_t dst_pitch_size)
{
	uint32_t error_code = 0U;

	// rotation, shear, scale: { WORD fract; short value; }
	static const MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };

	unsigned char *glyph_buf = nullptr;
	GLYPHMETRICS glyph_metrics;
	DWORD glyph_buf_byte_count = 0;
	DWORD glyph_buf_size =
		GetGlyphOutlineA(font_dc, glyph_code, GGO_GRAY8_BITMAP, &glyph_metrics, 0, nullptr, &identity);
	if (glyph_buf_size != GDI_ERROR && glyph_buf_size && glyph_buf_size <= dst_size_x * dst_size_y) {
		glyph_buf = arena_push_zero(arena, glyph_buf_size);
		ASSERT(glyph_buf);
		if (glyph_buf) {
			glyph_buf_byte_count = GetGlyphOutlineA(font_dc, glyph_code, GGO_GRAY8_BITMAP, &glyph_metrics,
			                                        glyph_buf_size, glyph_buf, &identity);
		} else {
			error_code = 1U;
		}
	}

	if (glyph_buf_byte_count != GDI_ERROR) {
		if (glyph_buf_byte_count) {
			ASSERT(dst_size_x >= glyph_metrics.gmBlackBoxX);
			ASSERT(dst_size_y >= glyph_metrics.gmBlackBoxY);
			if (dst_size_x >= glyph_metrics.gmBlackBoxX && dst_size_y >= glyph_metrics.gmBlackBoxY) {
				size_t dst_offset_x = (dst_size_x - glyph_metrics.gmBlackBoxX) / 2;
				size_t dst_offset_y = (size_t)((long)ascent_size - glyph_metrics.gmptGlyphOrigin.y);

				ASSERT(dst_offset_y < dst_size_y);
				if (dst_offset_y < dst_size_y) {
					uint32_t glyph_padding_x_size =
						(sizeof(DWORD) - (size_t)glyph_metrics.gmBlackBoxX % sizeof(DWORD)) % sizeof(DWORD);
					unsigned glyph_size_x = glyph_metrics.gmBlackBoxX + glyph_padding_x_size;

					unsigned glyph_offset_x = 0;
					signed glyph_offset_y = 0;

					ASSERT(glyph_offset_y >= 0);
					ASSERT(glyph_offset_y < (signed)dst_size_y);

					if (glyph_offset_y >= 0 && glyph_offset_y < (signed)dst_size_y) {
						mem_copy_rect(glyph_buf, glyph_size_x, glyph_metrics.gmBlackBoxY, glyph_size_x, glyph_offset_x,
						              (uint32_t)glyph_offset_y, dst_buf, dst_size_x, dst_size_y, dst_pitch_size,
						              dst_offset_x, dst_offset_y, glyph_metrics.gmBlackBoxX, glyph_metrics.gmBlackBoxY);

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
			error_code = 1U;
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
 * @brief Draws a rectangle with the specified color components
 *
 * @param buf
 * @param buf_width_px
 * @param buf_height_px
 * @param bounds_x_px_min included
 * @param bounds_y_px_min included
 * @param bounds_x_px_max not included
 * @param bounds_y_px_max not included
 * @param color_argb Color in Alpha,Red,Green,Blue format
 */
static void bitmap_draw_rectangle(void *buf, size_t buf_width_px, size_t buf_height_px, float bounds_x_px_min,
                                  float bounds_y_px_min, float bounds_x_px_max, float bounds_y_px_max,
                                  uint32_t color_argb)
{
	ASSERT(bounds_x_px_min < bounds_x_px_max);
	ASSERT(bounds_y_px_min < bounds_y_px_max);
	ASSERT(bounds_x_px_min >= 0.0F);
	ASSERT(bounds_y_px_min >= 0.0F);
	ASSERT(bounds_x_px_min < (float)buf_width_px);
	ASSERT(bounds_y_px_min < (float)buf_height_px);
	ASSERT(bounds_x_px_max <= (float)buf_width_px);
	ASSERT(bounds_y_px_max <= (float)buf_height_px);

	size_t bounds_x_idx_min = (unsigned)floorf(bounds_x_px_min);
	size_t bounds_y_idx_min = (unsigned)floorf(bounds_y_px_min);
	size_t bounds_x_idx_max = (unsigned)ceilf(bounds_x_px_max);
	size_t bounds_y_idx_max = (unsigned)ceilf(bounds_y_px_max);

	size_t pitch_size = buf_width_px * (size_t)PIXEL_SIZE;

	unsigned char *pixel_first_byte =
		(unsigned char *)buf + (bounds_x_idx_min * PIXEL_SIZE) + (bounds_y_idx_min * pitch_size);
	uint32_t *pixel = nullptr;
	for (size_t y = bounds_y_idx_min; y < bounds_y_idx_max; ++y) {
		for (size_t x = bounds_x_idx_min; x < bounds_x_idx_max; ++x) {
			pixel = (uint32_t *)pixel_first_byte;
			*pixel = color_argb;
			pixel_first_byte += PIXEL_SIZE;
		}

		pixel_first_byte += pitch_size - (bounds_x_idx_max - bounds_x_idx_min) * PIXEL_SIZE;
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
			uint32_t file_size = (uint32_t)(filesize_struct.QuadPart);

			// TODO(fredy): if the file is too big, use file mapping?
			result.buf = VirtualAlloc(nullptr, file_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.buf) {
				DWORD read_size = 0;
				if (ReadFile(handle, result.buf, file_size, &read_size, nullptr) || read_size == file_size) {
					result.size = file_size;
				} else {
					LOG_ERROR("failed to read the file: %s", path);

					file_free_memory(result.buf);

					result.buf = nullptr;
					result.size = 0;
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

inline static void keyboard_process_message(KeyState *key_state, uint32_t was_down, uint32_t is_down)
{
	key_state->ended_down = (uint8_t)is_down;
	if (was_down != is_down) {
		++key_state->half_transition_count;
	}
}

static unsigned long WINAPI render_run(void *param)
{
	HWND window = (HWND)param;

	HDC dc_handle = GetDC(window);
	if (!dc_handle) {
		LOG_ERROR("error getting the device context");
		goto END_ERROR;
	}

	// perm (Tix, ), renderer(atlas, tiles, scratch), buffers
	size_t renderer_buf_size = (size_t)ATLAS_TILE_SIZE_MAX * 20;
	size_t app_buf_size = sizeof(Tix) + renderer_buf_size + BUFFER_POOL_SIZE_MAX;

	BITMAPINFO bitmap_info = {
		.bmiHeader = {
			.biSize = sizeof(BITMAPINFOHEADER),
			.biPlanes = 1,
			.biBitCount = CHAR_BIT * PIXEL_SIZE,
			.biCompression = BI_RGB,
		},
	};

	void *app_buf =
		// NOLINTNEXTLINE(performance-no-int-to-ptr): fixed base address for deterministic pointers across runs
		VirtualAlloc(MEMORY_BASE_ADDRESS, app_buf_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!app_buf) {
		LOG_ERROR("unable to allocate %zu bytes for the app", app_buf_size);
		goto END_ERROR;
	}

	WinState win_state = {
		.storage = {
			.buf = app_buf,
			.buf_size = app_buf_size,
		},
		.is_running = 1U,
	};

	Tix *tix = (Tix *)win_state.storage.buf;
	tix->caret_mode = CARET_MODE_NORMAL;
	tix->caret.line_idx = 0;
	tix->caret.col_idx = 0;

	if (!win_state.storage.is_initialized) {
		arena_init(&tix->arena, win_state.storage.buf_size - sizeof(Tix),
		           (unsigned char *)win_state.storage.buf + sizeof(Tix));

		void *renderer_buf = arena_push(&tix->arena, renderer_buf_size);
		ASSERT(renderer_buf);
		arena_init(&tix->renderer_arena, renderer_buf_size, renderer_buf);

		void *buffers_arena_buf = arena_push(&tix->arena, BUFFER_POOL_SIZE_MAX);
		ASSERT(buffers_arena_buf);
		arena_init(&tix->buffers_arena, BUFFER_POOL_SIZE_MAX, buffers_arena_buf);

		win_state.storage.is_initialized = 1U;
	}

	// pt: physical unit - 1 point = 1/72 inch
	int font_size_pt = 16;
	int dpi_y = GetDeviceCaps(dc_handle, LOGPIXELSY);
	int font_size_px = MulDiv(font_size_pt, dpi_y, POINTS_PER_INCH);
	HDC font_dc = CreateCompatibleDC(nullptr);
	if (!font_dc) {
		LOG_ERROR("unable to get the font device context");
		goto END_ERROR;
	}
	const char *const font_name = "Consolas";
	HFONT font = CreateFontA(-font_size_px, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS,
	                         CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, font_name);
	if (!font) {
		LOG_ERROR("unable to load the '%s' font", font_name);
		goto END_ERROR;
	}
	SelectObject(font_dc, font);
	TEXTMETRICA text_metrics;
	GetTextMetricsA(font_dc, &text_metrics);

	tix->grid.tile_width_px = (unsigned)abs(text_metrics.tmAveCharWidth);
	tix->grid.tile_height_px = (unsigned)abs(text_metrics.tmHeight) + (unsigned)abs(text_metrics.tmExternalLeading);
	tix->grid.tile_ascent_px = (unsigned)abs(text_metrics.tmAscent); // Includes the internal leading

	SIZE size;
	GetTextExtentPoint32W(dc_handle, L"@", 1, &size);
	tix->grid.tile_width_px = (unsigned)max((long)tix->grid.tile_width_px, size.cx);
	tix->grid.tile_height_px = (unsigned)max((long)tix->grid.tile_height_px, size.cy);

	GetTextExtentPoint32W(dc_handle, L"M", 1, &size);
	tix->grid.tile_width_px = (unsigned)max((long)tix->grid.tile_width_px, size.cx);
	tix->grid.tile_height_px = (unsigned)max((long)tix->grid.tile_height_px, size.cy);

	GetTextExtentPoint32W(dc_handle, L"g", 1, &size);
	tix->grid.tile_width_px = (unsigned)max((long)tix->grid.tile_width_px, size.cx);
	tix->grid.tile_height_px = (unsigned)max((long)tix->grid.tile_height_px, size.cy);

	ASSERT(tix->grid.tile_width_px <= TILE_SIDE_PX_MAX);
	ASSERT(tix->grid.tile_height_px <= TILE_SIDE_PX_MAX);

	unsigned atlas_tile_size = tix->grid.tile_width_px * tix->grid.tile_height_px * ATLAS_PIXEL_SIZE;

	ASSERT(atlas_tile_size <= ATLAS_TILE_SIZE_MAX);

	tix->atlas.buf_size = (size_t)DIRECT_CODE_POINTS_COUNT * atlas_tile_size;
	ASSERT(tix->atlas.buf_size <= (size_t)ATLAS_BUF_SIZE_MAX);

	ArenaMark init_mark = arena_mark(&tix->renderer_arena);
	for (char p = DIRECT_CODE_POINT_MIN; p <= DIRECT_CODE_POINT_MAX; ++p) {
		GlyphIdx glyph_idx = { .value = (uint32_t)p - DIRECT_CODE_POINT_MIN };
		glyph_rasterize(font_dc, (uint32_t)p, &tix->renderer_arena, tix->grid.tile_ascent_px,
		                tix->atlas.buf + (size_t)(glyph_idx.value * atlas_tile_size), tix->grid.tile_width_px,
		                tix->grid.tile_height_px, tix->grid.tile_width_px);
		arena_rewind(&init_mark);
	}

	const char *file_path = "./test.txt";

	ReadFileResult file = {};
	FILETIME file_previous_write_time = {};

	constexpr uint32_t file_lines_count_max = 1000000;
	Line *file_lines = ARENA_PUSH_ARRAY(&tix->buffers_arena, Line, file_lines_count_max);
	size_t file_lines_count = 0;

	LARGE_INTEGER performance_frequency;
	QueryPerformanceFrequency(&performance_frequency);

	while (win_state.is_running) {
		LARGE_INTEGER wall_clock_at_start;
		QueryPerformanceCounter(&wall_clock_at_start);

		// =============================================================================
		// Input
		// =============================================================================
		TixInput tix_input = {};

		// Process POSTED messages
		MSG msg;
		// TODO(fredy): limit the iterations of this loop
		// TODO(fredy): deal with WM_DPICHANGED and WM_GETDPISCALEDSIZE
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			size_t key_stroke_info = (size_t)msg.lParam;
			uint32_t was_down = (key_stroke_info & (1U << 30U)) != 0;
			uint32_t is_down = (key_stroke_info & (1UL << 31UL)) == 0;

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

				if (vk_code == VK_CONTROL) {
					keyboard_process_message(&tix_input.keys[KEY_CTRL], was_down, is_down);
				}
			} break;
			case WM_CHAR: {
				char c = (char)msg.wParam;
				if (c == 'j') {
					keyboard_process_message(&tix_input.keys[KEY_J], was_down, is_down);
				} else if (c == 'k') {
					keyboard_process_message(&tix_input.keys[KEY_K], was_down, is_down);
				} else if (c == 'h') {
					keyboard_process_message(&tix_input.keys[KEY_H], was_down, is_down);
				} else if (c == 'l') {
					keyboard_process_message(&tix_input.keys[KEY_L], was_down, is_down);
				} else if (c == 'g') {
					keyboard_process_message(&tix_input.keys[KEY_G], was_down, is_down);
				} else if (c == 'd') {
					keyboard_process_message(&tix_input.keys[KEY_D], was_down, is_down);
				} else if (c == 'u') {
					keyboard_process_message(&tix_input.keys[KEY_U], was_down, is_down);
				} else if (c == 'G') {
					keyboard_process_message(&tix_input.keys[KEY_G], was_down, is_down);
					tix_input.keys[KEY_SHIFTED].ended_down = 1U;
				}
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
				ASSERT(false && "unexpected message arrived to the render thread");
			} break;
			}
		}

		// =============================================================================
		// Window
		// =============================================================================
		RECT client_rect;
		GetClientRect(window, &client_rect);

		uint32_t new_width_px = 0U;
		uint32_t new_height_px = 0U;

		if (client_rect.left < client_rect.right && client_rect.top < client_rect.bottom) {
			ASSERT(client_rect.right - client_rect.left > 0);
			ASSERT(client_rect.bottom - client_rect.top > 0);
			new_width_px = (uint32_t)(client_rect.right - client_rect.left);
			new_height_px = (uint32_t)(client_rect.bottom - client_rect.top);
		}

		if (new_width_px != tix->backbuf.width_px || new_height_px != tix->backbuf.height_px) {
			if (new_width_px * new_height_px * PIXEL_SIZE <= BACKBUF_SIZE_MAX) {
				tix->backbuf.width_px = new_width_px;
				tix->backbuf.height_px = new_height_px;
				tix->grid.tile_count_x = tix->backbuf.width_px / tix->grid.tile_width_px;
				tix->grid.tile_count_y =
					(uint32_t)floorf((float)tix->backbuf.height_px / (float)tix->grid.tile_height_px);
			} else {
				LOG_ERROR("unable to allocate %zu bytes for the backbuffer",
				          (size_t)new_width_px * new_height_px * PIXEL_SIZE);
				ASSERT(false && "unable to allocate memory for the backbuffer");
			}
		}

		ASSERT(tix->backbuf.width_px * tix->backbuf.height_px * PIXEL_SIZE <= BACKBUF_SIZE_MAX);

		if (tix->backbuf.width_px == 0 || tix->backbuf.height_px == 0) {
			continue;
		}

		// =============================================================================
		// Update
		// =============================================================================
		uint32_t backbuf_pitch_size = tix->backbuf.width_px * PIXEL_SIZE;

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
			size_t remaining_byte_count = file.size;

			__m256i newline_needle = _mm256_set1_epi8('\n');
			__m256i complex_mask = _mm256_set1_epi8((char)0x80);
			size_t line_start_idx = 0;
			size_t last_byte_idx = 0;

			while (file_lines_count < file_lines_count_max && remaining_byte_count) {
				__m256i contains_complex = _mm256_setzero_si256();

				while (remaining_byte_count > 32) {
					__m256i batch = _mm256_loadu_si256((__m256i *)buf);

					__m256i test_newline = _mm256_cmpeq_epi8(batch, newline_needle);
					__m256i test_complex = _mm256_and_si256(batch, complex_mask);

					uint32_t newline_detected = (uint32_t)_mm256_movemask_epi8(test_newline);

					if (newline_detected) {
						uint32_t first_newline_idx = 0;
						uint_ctz(newline_detected, &first_newline_idx);

						__m256i mask_complex = _mm256_loadu_si256((__m256i *)(overhang_mask + 32 - first_newline_idx));

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

		// Move cursor
		uint32_t was_caret_moved = 0U;
		if (tix_input.keys[KEY_K].ended_down && tix->caret.line_idx > 0) {
			--tix->caret.line_idx;
			was_caret_moved = 1U;
		}

		if (tix_input.keys[KEY_D].ended_down && tix_input.keys[KEY_CTRL].ended_down &&
		    !tix_input.keys[KEY_SHIFTED].ended_down) {
			tix->caret.line_idx += 10;
		}

		if (tix_input.keys[KEY_U].ended_down && tix_input.keys[KEY_CTRL].ended_down &&
		    tix_input.keys[KEY_SHIFTED].ended_down) {
			tix->caret.line_idx -= min(tix->caret.line_idx, 10);
		}

		if (tix_input.keys[KEY_G].ended_down) {
			if (tix_input.keys[KEY_G].half_transition_count > 1 ||
			    (tix->stack_key_count == 1 && tix->stack_key_codes[0] == KEY_G)) {
				tix->caret.line_idx = 0U;
				was_caret_moved = 1U;
				tix->stack_key_count = 0U;
			} else if (tix_input.keys[KEY_SHIFTED].ended_down) {
				tix->caret.line_idx = file_lines_count > 0 ? file_lines_count - 1 : 0;
				was_caret_moved = 1U;
			} else {
				tix->stack_key_codes[0] = KEY_G;
				tix->stack_key_count = 1U;
			}
		}

		if (tix_input.keys[KEY_J].ended_down && tix->caret.line_idx + 1 < file_lines_count) {
			++tix->caret.line_idx;
			was_caret_moved = 1U;
		}

		size_t new_line_col = file_lines[tix->caret.line_idx].newline_idx - file_lines[tix->caret.line_idx].start_idx;
		if (new_line_col > 0 &&
		    *((unsigned char *)file.buf + file_lines[tix->caret.line_idx].newline_idx - 1) == '\r') {
			--new_line_col;
		}

		if (tix_input.keys[KEY_H].ended_down && tix->caret.col_idx > 0) {
			--tix->caret.col_idx;
			was_caret_moved = 1U;
		}

		if (tix_input.keys[KEY_L].ended_down && tix->caret.col_idx + 1 < new_line_col) {
			++tix->caret.col_idx;
			was_caret_moved = 1U;
		}

		size_t caret_col = tix->caret.col_idx;
		if (caret_col >= new_line_col) {
			if (new_line_col > 0) {
				caret_col = new_line_col - 1;
			} else {
				caret_col = new_line_col;
			}
		}

		if (was_caret_moved && tix->caret.line_idx < tix->scroll_idx) {
			tix->scroll_idx = tix->caret.line_idx;
		}

		if (was_caret_moved && tix->caret.line_idx >= tix->scroll_idx + tix->grid.tile_count_y) {
			tix->scroll_idx = tix->caret.line_idx - tix->grid.tile_count_y + 1;
		}

		// Process mouse wheel
		if (tix_input.mouse_notches != 0) {
			int64_t new_scroll_offset = (int64_t)tix->scroll_idx;
			new_scroll_offset -= LINES_PER_NOTCH * (int64_t)tix_input.mouse_notches;
			new_scroll_offset = min(new_scroll_offset, (int64_t)file_lines_count - 1);
			new_scroll_offset = max(new_scroll_offset, 0);

			ASSERT(new_scroll_offset >= 0);

			tix->scroll_idx = (size_t)new_scroll_offset;
		}

		// =============================================================================
		// Segmentation
		// =============================================================================

		// =============================================================================
		// Layout
		// =============================================================================
		bitmap_draw_rectangle(&tix->backbuf.buf, tix->backbuf.width_px, tix->backbuf.height_px, 0.0F, 0.0F,
		                      (float)tix->backbuf.width_px, (float)tix->backbuf.height_px, BG_COLOR);
		unsigned tile_row = 0;
		for (size_t line_idx = tix->scroll_idx; line_idx < file_lines_count && tile_row < tix->grid.tile_count_y;
		     ++line_idx) {
			// =============================================================================
			// Shaping
			// =============================================================================

			// Sometimes multiple codepoints are merged into one glyph

			unsigned tile_col = 0;
			unsigned tile_min_y_px = tix->grid.tile_height_px * tile_row;
			char *p = (char *)file.buf + file_lines[line_idx].start_idx;
			GlyphIdx glyph_idx = {};
			unsigned char *glyph_buf = nullptr;
			uint32_t bg_color = BG_COLOR;
			uint32_t fg_color = FG_COLOR;
			while (p <= (char *)file.buf + file_lines[line_idx].newline_idx && tile_col < tix->grid.tile_count_x) {
				unsigned tile_min_x_px = tile_col * tix->grid.tile_width_px;
				char c = *p;

				if (tile_col == caret_col && line_idx == tix->caret.line_idx) {
					fg_color = BG_COLOR;
					bg_color = FG_COLOR;

					if (c == '\r' || c == '\n') {
						c = ' ';
					}
				} else {
					fg_color = FG_COLOR;
					bg_color = BG_COLOR;
				}

				if (c >= DIRECT_CODE_POINT_MIN && c <= DIRECT_CODE_POINT_MAX) {
					glyph_idx.value = (unsigned char)c - DIRECT_CODE_POINT_MIN;
					glyph_buf = tix->atlas.buf + (size_t)glyph_idx.value * atlas_tile_size;

					// TODO(fredy): what happen with width 1.5F?

					// in memory: BB GG RR AA
					uint8_t *dst_px_ptr = (unsigned char *)tix->backbuf.buf + (size_t)(tile_min_x_px * PIXEL_SIZE) +
					                      backbuf_pitch_size * tile_min_y_px;
					unsigned char *coverage_ptr = glyph_buf;

					// TODO(fredy): should I use SIMD here?
					for (size_t y = 0; y < tix->grid.tile_height_px; ++y) {
						for (size_t x = 0; x < tix->grid.tile_width_px; ++x) {
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

						dst_px_ptr += backbuf_pitch_size - (size_t)tix->grid.tile_width_px * PIXEL_SIZE;

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

		float frame_time_s =
			(float)(wall_clock_at_end.QuadPart - wall_clock_at_start.QuadPart) / (float)performance_frequency.QuadPart;

		char window_title[256];
		(void)snprintf(window_title, sizeof(window_title), "tix - ft: %fms, fps: %f", (double)(1000.0F * frame_time_s),
		               1.0 / (double)frame_time_s);

		SetWindowTextA(window, window_title);

		// TODO(fredy): Get the swap chain's back buffer (IDXGISwapChain::GetBuffer), and copy backbuf.buf into it.
		bitmap_info.bmiHeader.biWidth = (long)tix->backbuf.width_px;
		bitmap_info.bmiHeader.biHeight = -(long)tix->backbuf.height_px;
		SetDIBitsToDevice(dc_handle, 0, 0, tix->backbuf.width_px, tix->backbuf.height_px, 0, 0, 0,
		                  tix->backbuf.height_px, tix->backbuf.buf, &bitmap_info, DIB_RGB_COLORS);
	}

	ExitProcess(1);
END_ERROR:
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
	ASSERT(IsWindow(win_handle));
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
