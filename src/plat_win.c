#include <dwmapi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "app.h"
#include "lib.h"

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL

#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif

#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

#define BACKGROUND_COLOR_R 32
#define BACKGROUND_COLOR_G 34
#define BACKGROUND_COLOR_B 48

/**
 * @brief (0,0) is on the top left corner. Top-To-Bottom.
 * The byte order in a register (little endian) is AA RR GG BB
 */
typedef struct Bitmap {
	unsigned char *buf;
	size_t buf_size_byte;

	unsigned width_px;
	unsigned height_px;

	uint8_t pixel_size_byte;
} Bitmap;

typedef struct ReadFileResult {
	size_t size_byte;
	void *buf;
} ReadFileResult;

// TODO(fredy):  - remove this global
static unsigned long g_render_thread_id = 0;

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

inline static void keyboard_process_message(KeyState *key_state, uint32_t is_down)
{
	if (key_state->ended_down != is_down) {
		key_state->ended_down = (uint8_t)is_down;
		++key_state->half_transition_count;
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

static uint32_t file_free_memory(void *base_address)
{
	uint32_t result = 0U;

	if (base_address) {
		result = (uint32_t)VirtualFree(base_address, 0, MEM_RELEASE);
	}

	return result;
}

static ReadFileResult file_read(const char *const path)
{
	ReadFileResult result = {};

	HANDLE handle =
		CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER filesize_struct;
		if (GetFileSizeEx(handle, &filesize_struct)) {
			uint32_t file_size_byte = (uint32_t)(filesize_struct.QuadPart);
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

static unsigned long WINAPI render_run(void *param)
{
	HWND window = (HWND)param;

	HDC dc_handle = GetDC(window);
	if (dc_handle) {
		Tix tix = {
			.is_running = 1U,
			.context_path = "",
			.lines_count = 20,
			.storage = { .perm_size_byte = MB_TO_BYTE(64ULL), .trans_size_byte = GB_TO_BYTE(1ULL), },
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

		// NOLINTNEXTLINE(performance-no-int-to-ptr): fixed base address for deterministic pointers across runs
		tix.storage.perm_base_address = VirtualAlloc(MEMORY_BASE_ADDRESS,
		                                             tix.storage.perm_size_byte + tix.storage.trans_size_byte,
		                                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (tix.storage.perm_base_address) {
			tix.storage.trans_base_address = tix.storage.perm_base_address + tix.storage.perm_size_byte;
		}

		Arena perm_arena = {};
		arena_init(&perm_arena, tix.storage.perm_size_byte, tix.storage.perm_base_address);

		Arena trans_arena = {};
		arena_init(&trans_arena, tix.storage.trans_size_byte, tix.storage.trans_base_address);

		assert(trans_arena.buf);

		const char *file_path = "./test.txt";
		ReadFileResult file = {};
		FILETIME file_previous_write_time = {};

		HDC font_dc = CreateCompatibleDC(nullptr);
		HFONT font = CreateFontA(-128, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		                         ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
		SelectObject(font_dc, font);
		TEXTMETRICA tm;
		GetTextMetricsA(font_dc, &tm);

		unsigned cell_width_px = (unsigned)abs(tm.tmMaxCharWidth);
		unsigned cell_height_px = (unsigned)abs(tm.tmHeight);

		while (tix.is_running) {
			// =============================================================================
			// Input
			// =============================================================================
			int notches = 0;

			// Process POSTED messages
			MSG msg;
			// TODO(fredy): limit the iterations of this loop
			// TODO(fredy): deal with WM_DPICHANGED and WM_GETDPISCALEDSIZE
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				switch (msg.message) {
				case WM_QUIT: {
					// The WM_QUIT message is not associated with a window and therefore will never be received through a
					// window's window procedure. It is retrieved only by the GetMessage or PeekMessage functions
					tix.is_running = 0U;
				} break;
				case WM_MOUSEWHEEL: {
					int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
					// A "notch" refers to one discrete click/detent of a physical mouse wheel
					notches = delta / WHEEL_DELTA;
				} break;
				case WM_KEYDOWN:
				case WM_CHAR: {
					LOG_TRACE("A char arrived");
				} break;
				case WM_SIZE: {
					// No-op while the loop spins on PeekMessage; it only wakes the thread once the spin is
					// replaced by a blocking wait (GetMessage / MsgWaitForMultipleObjectsEx)
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

			if (backbuf.buf) {
				// =============================================================================
				// Update
				// =============================================================================
				unsigned visible_lines = (unsigned)floorf((float)backbuf.height_px / (float)cell_height_px);

				if (notches > 0) {
					if (tix.scroll_offset > (unsigned)notches) {
						tix.scroll_offset -= (unsigned)notches;
					} else {
						tix.scroll_offset = 0;
					}
				} else if (notches < 0) {
					tix.scroll_offset += (unsigned)-notches;
					size_t max_scroll_offset = tix.lines_count > visible_lines ? tix.lines_count - visible_lines : 0;
					if (tix.scroll_offset > max_scroll_offset) {
						tix.scroll_offset = max_scroll_offset;
					}
				}

				tix.scroll_offset = max(tix.scroll_offset, 0);
				tix.scroll_offset = min(tix.scroll_offset, tix.lines_count - visible_lines);

				FILETIME current_write_time = {};
				if (!file.buf) {
					file = file_read(file_path);
					if (file.buf) {
						file_get_last_write_time(file_path, &file_previous_write_time);
					} else {
						LOG_ERROR("The file %s could not be open\n", file_path);
					}
				} else if (file_get_last_write_time(file_path, &current_write_time)) {
					if (CompareFileTime(&current_write_time, &file_previous_write_time) > 0) {
						file_free_memory(file.buf);

						file = file_read(file_path);

						if (file.buf) {
							file_previous_write_time = current_write_time;
						} else {
							LOG_ERROR("The file %s could not be open\n", file_path);
						}
					}
				}

				// =============================================================================
				// Segmentation
				// =============================================================================

				// =============================================================================
				// Shaping
				// =============================================================================

				// =============================================================================
				// Layout
				// =============================================================================
				bitmap_draw_rectangle(&backbuf, 0.0F, 0.0F, (float)backbuf.width_px, (float)backbuf.height_px,
				                      BACKGROUND_COLOR_R / 255.0F, BACKGROUND_COLOR_G / 255.0F,
				                      BACKGROUND_COLOR_B / 255.0F);

				if (file.buf) {
					size_t line_idx = 0;
					size_t column_idx = 0;
					char *line = file.buf;
					char *file_end_plus_one = (char *)file.buf + file.size_byte;
					size_t last_visible_line_idx_plus_one = tix.scroll_offset + visible_lines;

					unsigned t = 0;
					for (char *p = file.buf; p < file_end_plus_one && t < 10; ++p) {
						if (line_idx >= tix.scroll_offset) {
							if (line_idx < last_visible_line_idx_plus_one) {
								if (*p == '\n') {
									++line_idx;
									line = p + 1;

									column_idx = 0;
								} else if (*p != '\r') {
									// rotation, shear, scale: { WORD fract; short value; }
									static const MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };

									void *glyph_buf = nullptr;
									GLYPHMETRICS glyph_metrics;
									DWORD glyph_size_byte = GetGlyphOutlineA(font_dc, (UINT)*p, GGO_GRAY8_BITMAP,
									                                         &glyph_metrics, 0, nullptr, &identity);
									if (glyph_size_byte != GDI_ERROR && glyph_size_byte &&
									    trans_arena.offset_byte + glyph_size_byte <= trans_arena.buf_size_byte) {
										glyph_buf = arena_push_zero(&trans_arena, glyph_size_byte);
										glyph_size_byte = GetGlyphOutlineA(font_dc, (UINT)(unsigned char)*p,
										                                   GGO_GRAY8_BITMAP, &glyph_metrics,
										                                   glyph_size_byte, glyph_buf, &identity);
									}

									if (glyph_size_byte != GDI_ERROR && glyph_size_byte) {
										size_t relative_line_idx = line_idx - tix.scroll_offset;

										float min_y_px = (float)cell_height_px * (float)relative_line_idx;
										float max_y_px = min_y_px + (float)cell_height_px;
										float min_x_px = (float)column_idx * (float)cell_width_px;
										float max_x_px = min_x_px + (float)cell_width_px;

										unsigned glyph_width_byte = glyph_metrics.gmBlackBoxX;
										unsigned glyph_height_byte = glyph_metrics.gmBlackBoxY;

										uint32_t row_padding_byte =
											(sizeof(DWORD) - (size_t)glyph_width_byte % sizeof(DWORD)) % sizeof(DWORD);
										uint32_t glyph_pitch_byte = glyph_width_byte + row_padding_byte;

										size_t backbuffer_pitch = (size_t)backbuf.width_px * backbuf.pixel_size_byte;

										// in memory: BB GG RR AA
										uint8_t *dst_px_ptr = backbuf.buf +
										                      (size_t)floorf(min_x_px) * backbuf.pixel_size_byte +
										                      backbuffer_pitch * (size_t)floorf(min_y_px);
										unsigned char *coverage_ptr = (unsigned char *)glyph_buf;

										for (size_t y = 0; y < glyph_height_byte; ++y) {
											for (size_t x = 0; x < glyph_width_byte; ++x) {
												uint8_t blend_factor = (*coverage_ptr * 255U) / 64U;

												// x/255 ~ x/256 + x/256² = (x + x/256) / 256

												// blue
												uint32_t blended =
													0x00U * blend_factor + *dst_px_ptr * (255 - blend_factor);
												*dst_px_ptr = (uint8_t)((blended + 1U + (blended >> 8U)) >> 8U);

												// green
												++dst_px_ptr;
												blended = 0xFFU * blend_factor + *dst_px_ptr * (255 - blend_factor);
												*dst_px_ptr = (uint8_t)((blended + 1U + (blended >> 8U)) >> 8U);

												// red
												++dst_px_ptr;
												blended = 0xFFU * blend_factor + *dst_px_ptr * (255 - blend_factor);
												*dst_px_ptr = (uint8_t)((blended + 1U + (blended >> 8U)) >> 8U);

												// alpha
												++dst_px_ptr;

												++dst_px_ptr;
												++coverage_ptr;
											}

											dst_px_ptr +=
												backbuffer_pitch - (size_t)glyph_width_byte * backbuf.pixel_size_byte;
											coverage_ptr += glyph_pitch_byte - glyph_width_byte;
										}

										// float red = 1.0F;
										// float green = 1.0F;
										// float blue = 1.0F;

										// if (min_x_px < (float)backbuffer.width_px &&
										//     min_y_px < (float)backbuffer.height_px) {
										// 	max_x_px = min(max_x_px, (float)backbuffer.width_px);
										// 	max_y_px = min(max_y_px, (float)backbuffer.height_px);

										// 	if (line[column_idx] == ' ') {
										// 		red = BACKGROUND_COLOR_R / 255.0F;
										// 		green = BACKGROUND_COLOR_G / 255.0F;
										// 		blue = BACKGROUND_COLOR_B / 255.0F;
										// 	}
										// 	bitmap_draw_rectangle(&backbuffer, min_x_px, min_y_px, max_x_px, max_y_px,
										// 	                      red, green, blue);
										// }
									}

									arena_reset(&trans_arena);

									++column_idx;
								}
							} else {
								break;
							}
						}

						++t;
					}

					tix.lines_count = line_idx;
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
				bitmap_info.bmiHeader.biWidth = (long)backbuf.width_px;
				bitmap_info.bmiHeader.biHeight = -(long)backbuf.height_px;
				SetDIBitsToDevice(dc_handle, 0, 0, backbuf.width_px, backbuf.height_px, 0, 0, 0, backbuf.height_px,
				                  backbuf.buf, &bitmap_info, DIB_RGB_COLORS);
			}
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

		if (msg.message == WM_CHAR || msg.message == WM_KEYDOWN || msg.message == WM_QUIT || msg.message == WM_SIZE ||
		    msg.message == WM_MOUSEWHEEL) {
			PostThreadMessageA(g_render_thread_id, msg.message, msg.wParam, msg.lParam);
		} else {
			DispatchMessageA(&msg);
		}
	}

	return EXIT_SUCCESS;
}
