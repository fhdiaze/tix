#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "lib.h"
#include "tix.h"

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ALL

typedef struct WindowDimensions {
	long width;
	long height;
} WindowDimensions;

/**
 * @brief (0,0) is on the top left corner.
 * The byte order in a register (little endian) is AA RR GG BB
 */
typedef struct WinOffscreenBuffer {
	unsigned width;
	unsigned height;
	unsigned pitch_bytes; // size of a row in bytes
	unsigned bytes_per_pixel;
	void *top_left_px;
	BITMAPINFO info;
} WinOffscreenBuffer;

static uint32_t g_is_running = 1U;

static WindowDimensions window_get_dimensions(HWND winhandle)
{
	WindowDimensions result;
	RECT client_rec;

	GetClientRect(winhandle, &client_rec);

	result.width = client_rec.right - client_rec.left;
	result.height = client_rec.bottom - client_rec.top;

	return result;
}

static void window_display_offscreen_buffer(HDC dc_handle, WinOffscreenBuffer *offscreen_buffer, long win_width,
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
static LRESULT CALLBACK window_handle_callback(HWND win_handle, [[__maybe_unused__]] UINT msg,
                                               [[__maybe_unused__]] WPARAM wparam, [[__maybe_unused__]] LPARAM lparam)
{
	LRESULT result = 0;

	switch (msg) {
	case WM_CLOSE:
	case WM_DESTROY: {
		g_is_running = 0U;
	} break;
	case WM_CREATE: {
		// TODO(fredy): use user data from window
		// CREATESTRUCTA *cs = (CREATESTRUCTA*) lparam;
	} break;
	case WM_ACTIVATEAPP: {
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP: {
		assert(false && "We must be processing the keyboard in other place");
	} break;
	case WM_PAINT: {
		PAINTSTRUCT paint;
		HDC dc_handle = BeginPaint(win_handle, &paint);

		WindowDimensions win_dim = window_get_dimensions(win_handle);

		window_display_offscreen_buffer(dc_handle, nullptr, win_dim.width, win_dim.height);

		EndPaint(win_handle, &paint);
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
 * @param keyboard_state
 */
static void window_pump_messages(TixState *tix_state, KeyboardState *keyboard_state)
{
	MSG msg;
	// TODO(fredy): limit the iterations of this loop
	while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
		switch (msg.message) {
		case WM_QUIT: {
			// The WM_QUIT message is not associated with a window and therefore will never be received through a
			// window's window procedure. It is retrieved only by the GetMessage or PeekMessage functions
			tix_state->is_running = 0U;
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
					keyboard_process_message(&keyboard_state->move_up, is_down);
				} else if (vk_code == 'K') {
					keyboard_process_message(&keyboard_state->move_left, is_down);
				} else if (vk_code == 'H') {
					keyboard_process_message(&keyboard_state->move_down, is_down);
				} else if (vk_code == 'L') {
					keyboard_process_message(&keyboard_state->move_right, is_down);
				} else if (vk_code == VK_ESCAPE) {
					keyboard_process_message(&keyboard_state->scape, is_down);
				}
			}
		} break;
		default: {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		} break;
		}
	}
}

static DWORD WINAPI win_entry_point(LPVOID param)
{
	HWND win_handle = (HWND)param;

	HDC dc_handle = GetDC(win_handle);
	if (!dc_handle) {
		LOG_ERROR("error getting the device context");
		return EXIT_FAILURE;
	}

	const char *file_path = "./test.txt";
	FILE *file = fopen(file_path, "a+");
	if (file == nullptr) {
		LOG_FATAL("The file %s could not be open\n", file_path);
		return EXIT_FAILURE;
	}

	TixState tix_state = {
		.is_running = 1U,
		.tix_path = "",
	};
	KeyboardState keyboard_state = {};
	while (g_is_running) {
		window_pump_messages(&tix_state, &keyboard_state);

		WindowDimensions windim = window_get_dimensions(win_handle);

		window_display_offscreen_buffer(dc_handle, nullptr, windim.width, windim.height);
	}

	char line[1000];
	if (fgets(line, 1000, file) == nullptr) {
		LOG_FATAL("Failed to read from file %s\n", file_path);

		if (fclose(file) == EOF) {
			LOG_FATAL("Failed to close file %s\n", file_path);

			return EXIT_FAILURE;
		}

		return EXIT_FAILURE;
	}

	LOG_INFO("line read: %s", line);

	if (fclose(file) == EOF) {
		LOG_FATAL("error closing the file: '%s'\n", file_path);
		return EXIT_FAILURE;
	}

	constexpr int QUIT_CHAR = 'q';
	int c = getchar();

	while (c != EOF) {
		if (c == QUIT_CHAR) {
			break;
		}

		c = getchar();
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

	WNDCLASSA win_class = {
		.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.hInstance = hInstance,
		.lpszClassName = "tix",
		.lpfnWndProc = window_handle_callback,
	};

	if (!RegisterClassA(&win_class)) {
		LOG_ERROR("error registering the window class");
		return EXIT_FAILURE;
	}

	HWND win_handle = CreateWindowExA(0, win_class.lpszClassName, "tix", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr,
	                                  hInstance, nullptr);
	if (!win_handle) {
		LOG_ERROR("error creating the window");
		return EXIT_FAILURE;
	}

	CreateThread(nullptr, 0, win_entry_point, win_handle, 0, nullptr);

	for (;;) {
		MSG msg = {};
		GetMessageA(&msg, nullptr, 0, 0);
		TranslateMessage(&msg);

		if (msg.message == WM_CHAR || msg.message == WM_KEYDOWN || msg.message == WM_QUIT ||
		    msg.message == WM_SIZE) {
			PostThreadMessage(0, msg.message, msg.wParam, msg.lParam);
		} else {
			DispatchMessageA(&msg);
		}
	}

	return EXIT_SUCCESS;
}
