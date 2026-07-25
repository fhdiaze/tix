# tix — TODO

Working checklist. The "how" and "why" live in [plan.md](plan.md); this file
tracks *what's next* against the current state of the code.

Legend: `[ ]` todo · `[~]` in progress · `[x]` done

---

## Stage 0 — Window + owned backbuffer + present

Goal: own the pixel buffer and present it ourselves, replacing the `TextOutA`
spike. See plan.md → "Stage 0 → Target state" for the specific choices.

- [ ] **Allocate the backbuffer once, at a generous size.** `VirtualAlloc` a
      32bpp BGRA, top-down block sized to an upper bound (primary monitor or
      virtual-desktop resolution), not the current client area. Fill in the
      existing `WinBitmap` struct (`top_left_px`, `pitch_bytes = width * 4`,
      `info` with negative `biHeight`). Replace the `PatBlt(BLACKNESS)` stub in
      `window_display_bitmap`.
- [ ] **Present via `SetDIBitsToDevice`.** Blit the used `width × height`
      sub-region to the cached window DC each frame (`BI_RGB` top-down,
      `DIB_RGB_COLORS`, 1:1 / no scaling). No `StretchDIBits`,
      `CreateDIBSection`, or `BitBlt`.
- [ ] **Wire up resize (no per-`WM_SIZE` realloc).** The `WM_SIZE` case in
      `render_process_messages` is currently an empty `// Keep it for awake`.
      On resize, just update the logical `width`/`height`/stride used for
      rendering and presenting — do **not** `VirtualFree`/`VirtualAlloc` each
      time (drag-resize fires many `WM_SIZE`/sec and would stutter). Reallocate
      only if the new size exceeds the current allocation. Own this on the
      render thread only.
- [ ] **Sanity content.** Fill with a solid color / gradient to prove writes
      land right-side up and right-sized.
- [ ] **Drop the `TextOutA` path.** Remove `window_render_lines`'
      GDI-text-drawing once the backbuffer present works.
- [ ] Exit check: resize repaints correctly — no flicker, no artifacts.

## Stage 1 — File → lines → cell grid

- [ ] Line index: scan for line breaks, store start/end offset per line.
- [ ] Define the `cell` struct (codepoint, fg, bg, flags).
- [ ] Allocate the cell grid sized in columns/rows (not pixels).
- [ ] Layout pass: fill visible lines into the grid.
- [ ] Render one solid bg-colored rectangle per cell (no glyphs yet).

## Stage 2 — Glyph atlas + real text

- [ ] Rasterize each unique codepoint once via `IDWriteBitmapRenderTarget`
      into a fixed-size tile in one CPU-side atlas.
- [ ] Store tiles as 8-bit coverage only (discard DirectWrite color).
- [ ] Codepoint → tile lookup.
- [ ] CPU compositing: per cell, blend bg→fg by coverage into the backbuffer.

## Stage 3 — Editor features (cell-grid only)

- [ ] Cursor (flag on a cell + blink timer).
- [ ] Selection highlight (per-cell flag/color override).
- [ ] Scrolling / line wrapping via a viewing offset into the line array.
- [ ] Syntax highlighting (tokenizer sets each cell's fg).

## Stage 4 — CPU performance pass

- [ ] SIMD the blend loop.
- [ ] Dirty-cell/row tracking.
- [ ] Incremental reparse of appended data only.
- [ ] Lay out visible lines + small margin only.
- [ ] Memory-map file loading once large files matter.
- [ ] SIMD newline scan — only if profiling shows it's a bottleneck.

## Stage 5 — GPU backend + shared swap-chain present

- [ ] D3D11 device + flip-model swap chain (WARP fallback).
- [ ] Re-express Stage 2 compositing as a compute shader.
- [ ] Upload cell grid + atlas each frame.
- [ ] Fold the CPU backend onto the swap chain (retires `SetDIBitsToDevice`).
- [ ] Runtime CPU/GPU toggle behind the `renderer_backend` seam.

## Stage 6 — GPU-specific optimization (stretch)

- [ ] Dirty-rect-aware dispatch.
- [ ] Multiple atlas pages for heavy Unicode / CJK.

---

## Loose ends / cleanup (not stage-gated)

- [ ] `render_process_messages`: bound the `PeekMessage` loop
      (`TODO(fredy)` in `plat_win.c`).
- [ ] `WM_SIZE` is posted to the render thread from both `window_procedure`
      and the `WinMain` message loop — de-duplicate.
- [ ] Review the `scroll_offset` clamp in the `WM_MOUSEWHEEL` handler; the
      `min(...)` currently compares against a boolean expression.
