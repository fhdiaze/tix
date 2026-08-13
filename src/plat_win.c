#include <dwmapi.h>
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

/**
 * @brief (0,0) is on the top left corner.
 * The byte order in a register (little endian) is AA RR GG BB
 */
typedef struct Bitmap {
	void *buf;
	size_t buf_size_byte;

	unsigned width_px;
	unsigned height_px;

	uint8_t pixel_size_byte;
} Bitmap;

typedef struct ReadFileResult {
	size_t size_byte;
	void *base_address;
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
			result.base_address = VirtualAlloc(nullptr, file_size_byte, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.base_address) {
				DWORD read_size_byte = 0;
				if (ReadFile(handle, result.base_address, file_size_byte, &read_size_byte, nullptr) ||
				    read_size_byte == file_size_byte) {
					result.size_byte = file_size_byte;
				} else {
					LOG_ERROR("failed to read the file: %s", path);

					file_free_memory(result.base_address);

					result.base_address = nullptr;
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
			.visible_lines = 10,
			.lines_count = 20,
			.storage = { .perm_size_byte = MB_TO_BYTES(64ULL), .trans_size_byte = GB_TO_BYTES(1ULL), },
		};

		Bitmap backbuffer = {
			.pixel_size_byte = 4,
		};
		BITMAPINFO bitmap_info = { .bmiHeader = {
									   .biSize = sizeof(BITMAPINFOHEADER),
									   .biPlanes = 1,
									   .biBitCount = CHAR_BIT * backbuffer.pixel_size_byte,
									   .biCompression = BI_RGB,
								   } };
		tix.storage.perm_base_address = VirtualAlloc(MEMORY_BASE_ADDRESS,
		                                             tix.storage.perm_size_byte + tix.storage.trans_size_byte,
		                                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		tix.storage.trans_base_address = tix.storage.perm_base_address + tix.storage.perm_size_byte;

		const char *file_path = "./test.txt";
		ReadFileResult file = file_read(file_path);

		while (tix.is_running) {
			// =============================================================================
			// Window
			// =============================================================================
			RECT client_rect;
			GetClientRect(window, &client_rect);

			assert(client_rect.right - client_rect.left >= 0);
			assert(client_rect.bottom - client_rect.top >= 0);

			unsigned new_width_px = (unsigned)(client_rect.right - client_rect.left);
			unsigned new_height_px = (unsigned)(client_rect.bottom - client_rect.top);

			if (new_width_px != backbuffer.width_px || new_height_px != backbuffer.height_px) {
				size_t new_buf_size_byte =
					(size_t)new_width_px * (size_t)new_height_px * (size_t)backbuffer.pixel_size_byte;
				void *new_buf = VirtualAlloc(nullptr, new_buf_size_byte, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

				if (new_buf) {
					if (backbuffer.buf && !VirtualFree(backbuffer.buf, 0, MEM_RELEASE)) {
						LOG_ERROR("unable to deallocate memory of the previous backbuffer");
						assert(false && "unable to deallocate memory of the previous backbuffer");
					}

					backbuffer.buf = new_buf;
					backbuffer.buf_size_byte = new_buf_size_byte;
					backbuffer.width_px = new_width_px;
					backbuffer.height_px = new_height_px;
				} else {
					LOG_ERROR("unable to allocate %zu bytes for the backbuffer", new_buf_size_byte);
					assert(false && "unable to allocate memory for the backbuffer");
				}
			}

			// =============================================================================
			// Input
			// =============================================================================

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
					int notches = delta / WHEEL_DELTA;
					if (notches > 0) {
						tix.scroll_offset -= (unsigned)notches;
					} else if (notches < 0) {
						tix.scroll_offset += (unsigned)-notches;
					}

					tix.scroll_offset = max(tix.scroll_offset, 0);
					tix.scroll_offset = min(tix.scroll_offset, tix.scroll_offset < tix.lines_count - tix.visible_lines);
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
			// Segmentation
			// =============================================================================

			// =============================================================================
			// Shaping
			// =============================================================================

			// =============================================================================
			// Rasterization
			// =============================================================================

			// Store atlas tiles as 8-bit coverage/alpha, not pre-colored RGB. Same reasoning as the GPU shader:
			// one grayscale glyph tile serves any foreground color, computed at blend time
			// (out = bg + coverage * (fg - bg)), rather than re-rasterizing per color.

			// =============================================================================
			// Layout
			// =============================================================================
			bitmap_draw_rectangle(&backbuffer, 0.0F, 0.0F, (float)backbuffer.width_px, (float)backbuffer.height_px,
			                      32.0F / 255.0F, 34.0F / 255.0F, 48.0F / 255.0F);

			if (file.size_byte) {
				unsigned cell_width_px = 10U;
				unsigned cell_height_px = 20U;

				size_t line_idx = 0;
				char *line_start = file.base_address;
				char *file_end_plus_one = (char *)file.base_address + file.size_byte;
				size_t last_visible_line_idx = tix.scroll_offset + tix.visible_lines;

				for (char *p = file.base_address; p <= file_end_plus_one; ++p) {
					if (p == file_end_plus_one || *p == '\n') {
						size_t line_length = (size_t)(p - line_start);
						if (line_length > 0 && line_start[line_length - 1] == '\r') {
							--line_length;
						}

						if (tix.scroll_offset <= line_idx && line_idx <= last_visible_line_idx) {
							size_t relative_line_idx = line_idx - tix.scroll_offset;
							float min_y_px = (float)cell_height_px * (float)relative_line_idx;
							float max_y_px = min_y_px + (float)cell_height_px;

							for (uint32_t glyph_idx = 0; glyph_idx < line_length; ++glyph_idx) {
								float min_x_px = (float)glyph_idx * (float)cell_width_px;
								float max_x_px = min_x_px + (float)cell_width_px;
								float red = 1.0F;
								float green = 1.0F;
								float blue = 1.0F;

								if (glyph_idx == line_length - 1 &&
								    max_x_px > (float)cell_width_px * (float)line_length) {
									assert(false);
								}

								if (min_x_px < (float)backbuffer.width_px && min_y_px < (float)backbuffer.height_px) {
									max_x_px = min(max_x_px, (float)backbuffer.width_px);
									max_y_px = min(max_y_px, (float)backbuffer.height_px);
									bitmap_draw_rectangle(&backbuffer, min_x_px, min_y_px, max_x_px, max_y_px, red,
									                      green, blue);
								}
							}
						}

						++line_idx;
						line_start = p + 1;
					}
				}
			} else {
				LOG_FATAL("The file %s could not be open\n", file_path);
			}

			// =============================================================================
			// Present
			// =============================================================================

			if (backbuffer.buf) {
				bitmap_info.bmiHeader.biWidth = (long)backbuffer.width_px;
				bitmap_info.bmiHeader.biHeight = -(long)backbuffer.height_px;
				SetDIBitsToDevice(dc_handle, 0, 0, backbuffer.width_px, backbuffer.height_px, 0, 0, 0,
				                  backbuffer.height_px, backbuffer.buf, &bitmap_info, DIB_RGB_COLORS);
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
