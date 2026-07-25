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
typedef struct WinBitmap {
	unsigned width_px;
	unsigned height_px;
	unsigned pitch_bytes; // size of a row in bytes
	BITMAPINFO info;
	void *top_left_px;
	unsigned short bytes_per_pixel;
} WinBitmap;

typedef struct ReadFileResult {
	size_t size_byte;
	void *base_address;
} ReadFileResult;

// TODO(fredy):  - remove this global
static unsigned long g_render_thread_id = 0;

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
			result.base_address =
				VirtualAlloc(nullptr, file_size_byte, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
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

		WinBitmap backbuffer = {};

		tix.storage.perm_base_address = VirtualAlloc(MEMORY_BASE_ADDRESS,
		                                             tix.storage.perm_size_byte + tix.storage.trans_size_byte,
		                                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		tix.storage.trans_base_address = tix.storage.perm_base_address + tix.storage.perm_size_byte;

		const char *file_path = "./test.txt";
		ReadFileResult file = file_read(file_path);

		while (tix.is_running) {
			RECT client_rec;
			GetClientRect(window, &client_rec);

			assert(client_rec.right - client_rec.left >= 0);
			assert(client_rec.bottom - client_rec.top >= 0);

			unsigned window_width_px = (unsigned)(client_rec.right - client_rec.left);
			unsigned window_height_px = (unsigned)(client_rec.bottom - client_rec.top);

			if (window_width_px * window_height_px > backbuffer.width_px * backbuffer.height_px) {
				if (backbuffer.top_left_px) {
					VirtualFree(backbuffer.top_left_px, 0, MEM_RELEASE);
				}

				size_t bitmap_memory_size = (size_t)(backbuffer.width_px) *
				                            (size_t)(backbuffer.height_px) *
				                            (size_t)(backbuffer.bytes_per_pixel);

				backbuffer.top_left_px = VirtualAlloc(nullptr, bitmap_memory_size,
				                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			}

			backbuffer.width_px = window_width_px;
			backbuffer.height_px = window_height_px;
			backbuffer.bytes_per_pixel = 4;
			backbuffer.info.bmiHeader.biSize = sizeof(backbuffer.info.bmiHeader);
			backbuffer.info.bmiHeader.biWidth = (long)backbuffer.width_px;
			backbuffer.info.bmiHeader.biHeight = -(long)backbuffer.height_px;
			backbuffer.info.bmiHeader.biPlanes = 1;
			backbuffer.info.bmiHeader.biBitCount = CHAR_BIT * backbuffer.bytes_per_pixel;
			backbuffer.info.bmiHeader.biCompression = BI_RGB;

			backbuffer.pitch_bytes = backbuffer.width_px * backbuffer.bytes_per_pixel;

			// =============================================================================
			// Input
			// =============================================================================

			// Process POSTED messages
			MSG msg;
			// TODO(fredy): limit the iterations of this loop
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
					tix.scroll_offset =
						min(tix.scroll_offset,
					            tix.scroll_offset < tix.lines_count - tix.visible_lines);
				} break;
				case WM_KEYDOWN:
				case WM_CHAR: {
					LOG_TRACE("A char arrived");
				} break;
				case WM_SIZE: {
					// Keep it for awake
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

			if (file.size_byte) {
				unsigned line_height_px = 20U;
				unsigned y = 0;
				SetBkMode(dc_handle, TRANSPARENT);
				SetTextColor(dc_handle, RGB(220, 220, 220));

				size_t current_line_number = 0;
				char *line_start = file.base_address;
				char *file_end_plus_one = (char *)file.base_address + file.size_byte;
				size_t last_visible_line_idx = tix.scroll_offset + tix.visible_lines;

				for (char *p = file.base_address; p <= file_end_plus_one; ++p) {
					if (p == file_end_plus_one || *p == '\n') {
						unsigned line_length = (unsigned)(p - line_start);
						if (line_length > 0 && line_start[line_length - 1] == '\r') {
							--line_length;
						}

						if (tix.scroll_offset <= current_line_number &&
						    current_line_number <= last_visible_line_idx) {
							TextOutA(dc_handle, 0, (int)y, line_start, (int)line_length);
						}

						++current_line_number;
						line_start = p + 1;
						y += line_height_px;
					}
				}
			} else {
				LOG_FATAL("The file %s could not be open\n", file_path);
			}

			// =============================================================================
			// Present
			// =============================================================================

			// SetDIBitsToDevice(dc_handle, 0, 0, window_width_px, window_height_px, 0, 0, 0, window_height_px,
			//                   backbuffer.top_left_px, &backbuffer.info, DIB_RGB_COLORS);
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

	WNDCLASSA win_class = {
		.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.hInstance = hInstance,
		.lpszClassName = "tix",
		.lpfnWndProc = window_procedure,
		.hbrBackground = CreateSolidBrush(background_color),
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

		if (msg.message == WM_CHAR || msg.message == WM_KEYDOWN || msg.message == WM_QUIT ||
		    msg.message == WM_SIZE || msg.message == WM_MOUSEWHEEL) {
			PostThreadMessageA(g_render_thread_id, msg.message, msg.wParam, msg.lParam);
		} else {
			DispatchMessageA(&msg);
		}
	}

	return EXIT_SUCCESS;
}
