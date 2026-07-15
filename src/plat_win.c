#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "app.h"
#include "lib.h"

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
		case WM_KEYDOWN:
		case WM_CHAR: {
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

static unsigned long WINAPI render_run(void *param)
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

	Tix tix = {
		.is_running = 1U,
		.context_path = "",
	};
	tix.storage.perm_base_address = VirtualAlloc(nullptr, GB_TO_BYTES(2), flAllocationType, DWORD flProtect);

	while (tix.is_running) {
		render_process_messages(&tix);

		WindowDimensions win_dim = window_get_dimensions(win_handle);

		window_display_offscreen_buffer(dc_handle, nullptr, win_dim.width, win_dim.height);
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
		.lpfnWndProc = window_procedure,
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

	CreateThread(nullptr, 0, render_run, win_handle, 0, &g_render_thread_id);

	for (;;) {
		MSG msg = {};
		GetMessageA(&msg, nullptr, 0, 0);
		TranslateMessage(&msg);

		if (msg.message == WM_CHAR || msg.message == WM_KEYDOWN || msg.message == WM_QUIT ||
		    msg.message == WM_SIZE) {
			PostThreadMessageA(g_render_thread_id, msg.message, msg.wParam, msg.lParam);
		} else {
			DispatchMessageA(&msg);
		}
	}

	return EXIT_SUCCESS;
}
