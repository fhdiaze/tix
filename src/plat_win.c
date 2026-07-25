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

typedef struct WindowDimensions {
	long width;
	long height;
} WindowDimensions;

/**
 * @brief (0,0) is on the top left corner.
 * The byte order in a register (little endian) is AA RR GG BB
 */
typedef struct WinBitmap {
	unsigned width;
	unsigned height;
	unsigned pitch_bytes; // size of a row in bytes
	unsigned bytes_per_pixel;
	void *top_left_px;
	BITMAPINFO info;
} WinBitmap;

static DWORD g_render_thread_id = 0;

static WindowDimensions window_get_dimensions(HWND winhandle)
{
	WindowDimensions result;

	RECT client_rec;
	GetClientRect(winhandle, &client_rec);

	result.width = client_rec.right - client_rec.left;
	result.height = client_rec.bottom - client_rec.top;

	return result;
}

static void window_display_bitmap(HDC dc_handle, WinBitmap *bitmap, long win_width,
                                            long win_height)
{
	PatBlt(dc_handle, 0, 0, win_width, win_height, BLACKNESS);
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

/**
 * @brief Process POSTED messages
 *
 * @param tix_state
 */
static void render_process_messages(Tix *tix)
{
	MSG msg;
	// TODO(fredy): limit the iterations of this loop
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		switch (msg.message) {
		case WM_QUIT: {
			// The WM_QUIT message is not associated with a window and therefore will never be received through a
			// window's window procedure. It is retrieved only by the GetMessage or PeekMessage functions
			tix->is_running = 0U;
		} break;
		case WM_MOUSEWHEEL: {
			int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
			// A "notch" refers to one discrete click/detent of a physical mouse wheel
			int notches = delta / WHEEL_DELTA;
			if (notches > 0) {
				tix->scroll_offset -= (unsigned)notches;
			} else if (notches < 0) {
				tix->scroll_offset += (unsigned)-notches;
			}

			tix->scroll_offset = max(tix->scroll_offset, 0);
			tix->scroll_offset =
				min(tix->scroll_offset, tix->scroll_offset < tix->lines_count - tix->visible_lines);
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
}

// =============================================================================
// File management
// =============================================================================

typedef struct ReadFileResult {
	size_t size_byte;
	void *base_address;
} ReadFileResult;

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

static void window_render_lines(HDC dc_handle, ReadFileResult *file, size_t start_line_idx, size_t lines_count)
{
	unsigned line_height_px = 20U;
	unsigned y = 0;
	SetBkMode(dc_handle, TRANSPARENT);
	SetTextColor(dc_handle, RGB(220, 220, 220));

	size_t current_line_number = 0;
	char *line_start = file->base_address;
	char *file_end_plus_one = (char *)file->base_address + file->size_byte;
	size_t end_line_idx = start_line_idx + lines_count;

	for (char *p = file->base_address; p <= file_end_plus_one; ++p) {
		if (p == file_end_plus_one || *p == '\n') {
			unsigned line_length = (unsigned)(p - line_start);
			if (line_length > 0 && line_start[line_length - 1] == '\r') {
				--line_length;
			}

			if (start_line_idx <= current_line_number && current_line_number <= end_line_idx) {
				TextOutA(dc_handle, 0, (int)y, line_start, (int)line_length);
			}

			++current_line_number;
			line_start = p + 1;
			y += line_height_px;
		}
	}
}

static unsigned long WINAPI render_run(void *param)
{
	HWND win_handle = (HWND)param;

	HDC dc_handle = GetDC(win_handle);
	if (dc_handle) {
		Tix tix = {
			.is_running = 1U,
			.context_path = "",
			.visible_lines = 10,
			.lines_count = 20,
			.storage = { .perm_size_byte = MB_TO_BYTES(64ULL), .trans_size_byte = GB_TO_BYTES(1ULL), },
		};

		tix.storage.perm_base_address = VirtualAlloc(MEMORY_BASE_ADDRESS,
		                                             tix.storage.perm_size_byte + tix.storage.trans_size_byte,
		                                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		tix.storage.trans_base_address = tix.storage.perm_base_address + tix.storage.perm_size_byte;

		const char *file_path = "./test.txt";
		ReadFileResult file = file_read(file_path);

		while (tix.is_running) {
			// Input
			render_process_messages(&tix);

			// Segmentation

			// Shaping

			// Rasterization

			// Store atlas tiles as 8-bit coverage/alpha, not pre-colored RGB. Same reasoning as the GPU shader:
			// one grayscale glyph tile serves any foreground color, computed at blend time
			// (out = bg + coverage * (fg - bg)), rather than re-rasterizing per color.

			// Layout

			// Render
			if (file.size_byte) {
				window_render_lines(dc_handle, &file, tix.scroll_offset, tix.visible_lines);
			} else {
				LOG_FATAL("The file %s could not be open\n", file_path);
			}

			// WindowDimensions win_dim = window_get_dimensions(win_handle);

			// window_display_offscreen_buffer(dc_handle, nullptr, win_dim.width, win_dim.height);
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
