## Plan

Since this mirrors refterm's architecture, here's an iterative roadmap from simplest to refterm-level performance:

## Stage 0: Naive (get something on screen)
- Read the whole txt file into memory as one big string/byte array
- Use any immediate-mode text API (GDI `TextOut`, or a simple font library) to draw each line directly to a window
- No caching, no cell grid — just loop over lines and call a text-drawing function
- **Goal**: prove the pipeline (file → window) works. Don't worry about performance yet.

## Stage 1: Cell grid (CPU-side)
- Define a `cell` struct: `{ character/codepoint, foreground color, background color, flags }`
- Allocate a 2D grid `DimX × DimY` matching your window size in character cells (like refterm's `renderer_cell` / `AllocateTerminalBuffer`)
- Parse the file into lines (track offsets, like refterm's `example_line` with `FirstP`/`OnePastLastP`)
- A "layout" step converts visible lines → fills the cell grid (like `LayoutLines`/`ParseLineIntoGlyphs`)
- Still render each cell via CPU (e.g., blit pre-rasterized bitmap glyphs to a backbuffer) — no GPU yet

## Stage 2: Glyph atlas + GPU compositing
- Rasterize each unique character once into an atlas texture (a big texture divided into fixed-size tiles), like refterm's `GlyphTexture`/`TransferTile`
- Give each glyph a `GlyphIndex` (tile X/Y packed into a uint)
- Upload your cell grid as a `StructuredBuffer` to the GPU each frame
- Write a shader almost identical to `ComputeOutputColor()`: given a pixel, find its cell, look up `GlyphIndex`, sample the atlas, blend fg/bg colors
- This is the big performance jump — GPU does all pixel work in parallel instead of CPU blitting

## Stage 3: Editor-specific additions
- Cursor rendering (blinking, like refterm's `BlinkModulate`)
- Selection highlighting (extra flag/color per cell, similar to reverse-video)
- Line wrapping / scrolling (viewport offset into your line array, like `ViewingLineOffset`)
- Syntax highlighting = just setting different `Foreground` per cell based on a tokenizer pass

## Stage 4: Handle large files efficiently
- Don't reparse the whole file every frame — only reparse edited regions (refterm reparses only newly appended data via `ParseLines`)
- Only fill the cell grid for visible lines + a small buffer above/below (refterm does `2×DimY` lines in `LayoutLines`)
- If files get huge, consider memory-mapping the file instead of loading fully into RAM

## Stage 5: SIMD scanning (optional, only if profiling shows it's needed)
- Use SSE/AVX to scan for newlines and non-ASCII bytes in bulk, like refterm's `ParseLines` (`_mm_cmpeq_epi8` on 16-byte chunks)
- Only worth doing once you've profiled and confirmed line-scanning is a bottleneck

**Key principle**: Stage 0-1 gets you a working editor fast with pure CPU rendering. Stage 2 (GPU cell buffer + glyph atlas) is the point where you adopt refterm's actual technique and get its performance characteristics. Don't jump straight to GPU shaders before you have the CPU-side data model (cell grid, line indexing) working correctly — it's much easier to debug logic on the CPU first.


Updated todo list

Here's Stage 0 broken down in full detail — a minimal Win32 text file viewer using GDI (no GPU, no cell grid, no glyph caching):

## 1. Window Setup

Create a standard Win32 window (same as refterm's approach in refterm_example_terminal.c):

```c
LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch(Message)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            // We'll do our drawing here
            break;
    }
    return DefWindowProcA(Window, Message, WParam, LParam);
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE Prev, LPSTR CmdLine, int Show)
{
    WNDCLASSA WindowClass = {0};
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "TextEditorClass";
    RegisterClassA(&WindowClass);

    HWND Window = CreateWindowExA(0, "TextEditorClass", "Simple Editor",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        0, 0, Instance, 0);

    // message loop goes here (step 5)
}
```

## 2. Load the File

Read the whole file into memory as a single buffer (simplest possible I/O):

```c
char *LoadFile(char *Path, size_t *OutSize)
{
    HANDLE File = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, 0,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    DWORD Size = GetFileSize(File, 0);
    char *Buffer = malloc(Size + 1);
    DWORD BytesRead;
    ReadFile(File, Buffer, Size, &BytesRead, 0);
    Buffer[Size] = 0; // null terminate
    CloseHandle(File);
    *OutSize = Size;
    return Buffer;
}
```

## 3. Split Into Lines

Simplest possible line index — just an array of pointers/offsets, split on `\n`:

```c
typedef struct { char *Start; size_t Length; } line;

line *Lines = malloc(sizeof(line) * MaxLines);
int LineCount = 0;

char *P = FileBuffer;
char *LineStart = P;
while(*P)
{
    if(*P == '\n')
    {
        Lines[LineCount].Start = LineStart;
        Lines[LineCount].Length = P - LineStart;
        LineCount++;
        LineStart = P + 1;
    }
    P++;
}
// handle last line if file doesn't end with \n
```

This is your data model equivalent of refterm's `example_line` array — just far simpler (no scroll-back circular buffer, no escape-code parsing).

## 4. Draw Text with GDI (no GPU yet)

In `WM_PAINT`, use `TextOutA`/`DrawTextA` to draw each visible line directly:

```c
case WM_PAINT:
{
    PAINTSTRUCT Paint;
    HDC DC = BeginPaint(Window, &Paint);

    HFONT Font = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH, "Consolas");
    HFONT OldFont = SelectObject(DC, Font);

    SetTextColor(DC, RGB(200,200,200));
    SetBkColor(DC, RGB(20,20,20));

    int LineHeight = 20; // pixels per line, matches font size roughly
    int FirstVisibleLine = ScrollOffset; // which line is at top of screen

    for(int i = 0; i < VisibleLineCount; i++)
    {
        int LineIndex = FirstVisibleLine + i;
        if(LineIndex >= LineCount) break;

        TextOutA(DC, 0, i * LineHeight,
                 Lines[LineIndex].Start, (int)Lines[LineIndex].Length);
    }

    SelectObject(DC, OldFont);
    DeleteObject(Font);
    EndPaint(Window, &Paint);
} break;
```

**Key point**: GDI rasterizes and draws each character glyph itself on every `WM_PAINT` — no caching, no GPU texture, no cell buffer. It's slow compared to refterm's approach, but it's the simplest thing that gets text on screen.

## 5. Message Loop

```c
MSG Message;
while(GetMessage(&Message, 0, 0, 0))
{
    TranslateMessage(&Message);
    DispatchMessage(&Message);
}
```

## 6. Basic Scrolling (optional but useful even at Stage 0)

Handle `WM_MOUSEWHEEL` or arrow keys to change `ScrollOffset`, then call `InvalidateRect(Window, 0, TRUE)` to trigger a repaint:

```c
case WM_MOUSEWHEEL:
{
    int Delta = GET_WHEEL_DELTA_WPARAM(WParam);
    ScrollOffset -= (Delta / WHEEL_DELTA) * 3; // scroll 3 lines per notch
    if(ScrollOffset < 0) ScrollOffset = 0;
    InvalidateRect(Window, 0, TRUE);
} break;
```

## What Stage 0 Deliberately Skips

- No Unicode/UTF-8 decoding (assume ASCII for now)
- No syntax highlighting
- No editing (read-only viewer first — add cursor/typing in Stage 3)
- No glyph caching — GDI redraws every character every frame, which is fine at this stage because file sizes are small and you're not editing yet
- No cell grid data structure — lines are drawn directly as strings

## Why Start Here

This gets you:
- A window
- A file loaded into memory
- Text visibly on screen
- Basic scrolling

...in under ~150 lines, with zero GPU/shader complexity. Once this works, Stage 1 replaces `TextOutA` calls with a `cell` grid + `LayoutLines`-style function, which sets up the exact data model refterm uses — making the jump to GPU rendering (Stage 2) straightforward since you'll already have `GlyphIndex`/`Foreground`/`Background` per cell ready to upload.