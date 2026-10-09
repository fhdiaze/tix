# TODO

Working checklist. this file tracks *what's next* against the current state of the code.

Legend: `[ ]` todo · `[~]` in progress · `[x]` done

---
- [x] vim: $ command
- [x] vim: _ command
- [ ] vim: w command
- [ ] vim: b command
- [ ] vim: f command
- [ ] vim: F command
- [ ] vim: dd command
- [ ] vim: ctrl+o command
- [ ] vim: ctrl+i command
- [ ] Memory allocation strategy:
        * perm: Tix(atlas, backbuf, TileGrid)
        * renderer: scratch
        * buffers:
            * blocks of lines, like a pool allocator. size=sizeof(Line)*count where count is power of 2. Or, we can push four times the size of the file: half for the file content half for lines.
            * we can have two arenas: one for the file contents, other for the lines like a link list or similar
        * scratch
- [ ] Define the `tile` struct (glyph_idx, fg, bg, flags).
- [ ] Allocate the tile grid sized in columns/rows (not pixels).
- [ ] Improve input management.
- [ ] Selection highlight (per-cell flag/color override).
- [ ] line wrapping
- [ ] Syntax highlighting (tokenizer sets each cell's fg).
- [ ] SIMD the blend loop.
- [ ] Dirty-cell/row tracking.
- [ ] Lay out visible lines + small margin only.
- [ ] Memory-map or partial file loading once large files matter.
- [ ] Render non-ascii unicode code points
- [ ] `render_process_messages`: bound the `PeekMessage` loop
      (`TODO(fredy)` in `sys_win.c`).




## GPU backend

- [ ] D3D11 device + flip-model swap chain (WARP fallback).
- [ ] Re-express Stage 2 compositing as a compute shader.
- [ ] Upload cell grid + atlas each frame.
- [ ] Fold the CPU backend onto the swap chain (retires `SetDIBitsToDevice`).
- [ ] Runtime CPU/GPU toggle behind the `renderer_backend` seam.
- [ ] Dirty-rect-aware dispatch.
- [ ] Multiple atlas pages for heavy Unicode / CJK.

## Stage 5 — GPU backend (compute shader, same data, same math)

Nothing above this line changes. The work here is entirely: stand up D3D11,
and re-express the Stage 2 compositing loop as a compute shader.

**This is also where the Stage 0 presentation note gets resolved.** Standing
up the swap chain isn't purely a GPU-backend need — once it exists, the CPU
backend's finished buffer can be copied into it too (see the toggle section
below), which is what actually collapses the GDI double-copy into one and
adds real vsync control. That's a genuine win for the CPU path as well, not
just infrastructure for the GPU one.

- Device and swap chain, set up so the back buffer can be written to directly
  from a compute shader (no intermediate copy/blit)
- Fall back to Microsoft's software D3D11 driver (WARP) if hardware device
  creation fails — covers machines without a capable GPU, see below
- Upload the same cell grid to the GPU each frame
- Upload the same atlas bitmap built in Stage 2, unmodified, as a texture
- Compute shader: per pixel, find its cell, look up the glyph tile, sample the
  atlas, apply the **same** background-toward-foreground blend by coverage, and
  write the result directly into the back buffer
- Run the shader, then present — no CPU-side copy needed if the shader writes
  directly to the presentable surface

**Optional: runtime CPU/GPU toggle.** Since both backends will exist side by
side, wire a key to switch which one composites each frame. To keep a single
present path for both, have the CPU backend copy its finished pixel buffer into
a GPU-side texture each frame, then into the same back buffer the GPU path
writes to. That keeps the window/device/swap-chain/present code — the part
that shouldn't be rewritten — genuinely identical for both backends; only the
fill step differs. This gives a direct, visible A/B comparison, and a
correctness check any time the shader changes.

**Exit criteria:** GPU output is pixel-identical (or near enough — float rounding
aside) to the CPU backend on the same content, and is measurably faster.

---

## Stage 6 — Further GPU-specific optimization (optional / stretch)

Only pursue if profiling motivates it:

- Dirty-rect-aware dispatch (scissor to changed region instead of full-screen
  every frame)
- Multiple glyph atlas pages / larger atlas if many unique glyphs are in play
  (CJK, heavy Unicode use)

---

## Renderer-agnostic backend interface (the seam)

To keep the main loop and toggle logic ignorant of which backend is active,
give both backends the same shape:

```c
typedef struct {
    void (*Init)(HWND Window);
    void (*Resize)(uint32_t Width, uint32_t Height);
    void (*Present)(cell *Grid, uint32_t DimX, uint32_t DimY, glyph_atlas *Atlas);
} renderer_backend;
```

The main loop only ever calls through this shape — switching which backend is
active on a keypress becomes a single pointer swap (`Active = &CpuBackend` or
`Active = &GpuBackend`), with no branching anywhere else in the codebase.

---

## Why WARP means you don't need a hand-rolled CPU fallback for shipping

Separate from the "build CPU first to learn" reasoning above: Windows itself
ships WARP, a full software implementation of D3D11 (compute shaders included)
since Windows 7. Falling back to it when hardware device creation fails means
the same shader runs correctly, just slower, on machines without a capable GPU
— no separate rendering code required for that case. The CPU backend in this
plan exists for learning, debugging, and comparison — not because WARP leaves
a gap in device coverage.