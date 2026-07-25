# tix — Rendering Plan (CPU-first, GPU-second, shared-core architecture)

## Guiding principle

Split the system into **renderer-agnostic core** and **renderer-specific backends**.
The core — file loading, line indexing, the cell grid, glyph atlas generation,
and all editor logic (cursor, selection, scrolling) — is written *once* and
never touched when the GPU backend is added later. Only the final "cell grid →
pixels" compositing step is implemented twice: once on the CPU, once on the
GPU. Both consume exactly the same data.

```
   file bytes → line index → cell grid (codepoint, fg, bg, flags)
                                    │
                                    ▼
                 glyph atlas (unique glyphs rasterized once,
                 stored as coverage tiles — not pre-colored)
                                    │
                 cursor / selection / scroll — cell-grid
                 flag & color writes only, no render knowledge
                                    │
                       ── backend seam (init/resize/present) ──
                                    │
                     ┌──────────────┴──────────────┐
                     ▼                              ▼
              CPU backend                    GPU backend
           (own backbuffer,                (D3D11 compute
            scalar/SIMD blend)              shader, same math)
```

The atlas is stored as plain coverage — "how much ink" per pixel, not a
pre-colored image — so the same blend logic applies identically whether it's
running as a CPU loop or a GPU shader:

```
out_color = bg + coverage * (fg - bg)
```

That blend step is the *only* thing written twice; everything feeding into it
is single-source.

---

## Stage 0 — Window + owned backbuffer + present (no text yet)

**Goal:** prove the window → pixel buffer → screen pipeline works, using only
tools you already know (arrays, pointers, GDI blit). No D3D, no glyphs yet.

- Win32 window + standard message loop
- Own backbuffer: a plain allocated block of memory sized to the window (in
  pixels), described as a top-down bitmap so row 0 is the top row on screen
- Reallocate the backbuffer whenever the window resizes
- Present every frame by copying that memory straight to the window via
  StretchDIBits — no intermediate GDI bitmap object needed
- Sanity content: fill with a solid color or gradient, just to prove writes to
  the buffer show up on screen correctly, right-side up, right-sized

**Presentation choice:** own-memory-plus-StretchDIBits, not
SetDIBitsToDevice or BitBlt-off-a-cached-DIB-section. This is a deliberate
pick, not a placeholder to revisit later — all three do the same class of copy
into GDI's redirection surface, so there's no meaningful throughput difference
between them. Don't spend time trying variants of this; see the note below for
where the actual presentation-layer gain lives.

**Exit criteria:** resizing the window resizes the buffer and repaints
correctly, no flicker, no artifacts.

---

## Note: what StretchDIBits actually costs, and when to stop paying it

Worth knowing up front so the optimization order in Stages 4-5 makes sense:

- **GDI is CPU-only on modern Windows.** Hardware GDI acceleration was
  deprecated starting with Windows 8 — StretchDIBits runs in software,
  not on the GPU, same as your blend loop.
- **A GDI-drawn window gets composited through an extra copy.** Windows keeps
  a separate "redirection surface" for GDI windows; StretchDIBits copies your
  buffer into that surface, and DWM composites the surface onto the desktop
  separately. That's two full-frame copies per present, not one, and the
  second one is invisible to your code.
- **This is not this project's near-term bottleneck.** For a terminal-sized
  window, that extra copy is a few megabytes of memcpy — real, but dwarfed by
  an unoptimized blend loop. Optimizing it now would be tuning the wrong
  layer first.
- **It goes away for free later, not by tuning GDI harder.** Presenting
  through a DXGI flip-model swap chain instead — which Stage 5 sets up anyway,
  for the GPU backend — lets DWM scan the surface out directly in the common
  case, collapsing the two copies into one, and adds real vsync control
  (`Present(1, 0)`) that GDI has no equivalent for. That's the point at which
  the CPU backend also gets folded onto the swap chain (see Stage 5) — not
  because it needs a GPU, but because that's where this specific copy is
  actually eliminated.

**Rule of thumb for this project:** optimize the blend loop and the parsing
pipeline first (Stage 4); let the presentation-layer copy get eliminated as a
side effect of standing up the swap chain in Stage 5, rather than chasing it
on the GDI side.

---

## Stage 1 — File → lines → cell grid (shared core, part 1)

- Read the whole file into memory to start (memory-mapping comes in Stage 4
  once large files matter)
- Line index: scan for line breaks, store a start/end offset per line
  (mirrors refterm's line-indexing approach)
- Define a cell:

  ```c
  typedef struct {
      uint32_t Codepoint;
      uint32_t Foreground;
      uint32_t Background;
      uint32_t Flags; // cursor/selection/etc., added in Stage 3
  } cell;
  ```
- Allocate a grid of cells sized to the window in character columns/rows, not
  pixels
- Layout step: visible lines fill the cell grid (mirrors refterm's line-layout
  pass)
- **Rendering for this stage:** draw one solid-colored rectangle per cell,
  using only the background color, ignoring glyphs entirely. This proves the
  cell-grid-to-pixel-region mapping is correct *before* glyph rasterization is
  in the mix — one less variable to debug at once.

**Exit criteria:** grid of colored rectangles matches window dimensions exactly,
resizes cleanly, no off-by-one seams between cells.

---

## Stage 2 — Glyph atlas + real text (shared core, part 2 + CPU backend)

This is the first stage that's genuinely shared forever — the atlas-generation
code written here is reused unchanged by the GPU backend in Stage 5.

- Rasterize each unique codepoint once, via IDWriteBitmapRenderTarget, into a
  fixed-size tile inside one large CPU-side atlas bitmap (mirrors refterm's
  glyph texture/tile-transfer approach)
- Store atlas tiles as coverage only — discard color info from DirectWrite's
  output, keep only "how much ink" per pixel
- Maintain a lookup from codepoint to tile position (mirrors refterm's glyph
  table)
- **CPU compositing step:** for each cell, look up its glyph's tile, walk the
  tile's pixels, blend background toward foreground in proportion to coverage,
  and write the result into the backbuffer at that cell's position
- Present the same way as Stage 0/1 — nothing about presentation changes here

**Exit criteria:** real text from the file renders correctly, correct colors,
correct glyph shapes, no atlas cache misses/corruption on repeated characters.

---

## Stage 3 — Editor-specific features (shared core, part 3)

All of these are cell-grid manipulations only — zero renderer-specific code,
by construction:

- Cursor rendering (a flag on one cell, plus a blink timer, mirrors refterm's
  blink modulation)
- Selection highlighting (a flag/color override per cell, like reverse-video)
- Line wrapping / scrolling (an offset into the line array controlling which
  lines are visible, mirrors refterm's viewing-offset approach)
- Syntax highlighting (a tokenizer pass that sets each cell's foreground color)

**Exit criteria:** all features work and are visually correct under the CPU
backend. Since this logic never touches pixels directly, it should need zero
changes when the GPU backend lands later.

---

## Stage 4 — CPU performance pass

Do this *before* starting the GPU backend — the point is to have a genuinely
fast, well-understood CPU renderer as the baseline/oracle, and to build
intuition for where the time actually goes before reaching for a GPU.

**In scope — these are where the time actually is right now:**

- SIMD the blend loop specifically — blending a row of pixels toward their
  background/foreground colors by coverage is a clean vectorization target —
  kept separate from any file-scanning SIMD work
- Dirty-cell/row tracking: only reblend cells that changed since the last frame
- Only reparse newly appended file data on edits/appends, not the whole file
  (mirrors refterm's incremental line parsing)
- Only fill the cell grid for visible lines plus a small margin above/below
  (mirrors refterm's approach of laying out slightly more than one screen's
  worth of lines)
- If files get large: switch file loading to memory-mapping instead of reading
  the whole file into memory up front
- SIMD line/newline scanning over the raw file bytes — only worth doing if
  profiling actually shows the scan is a bottleneck, not preemptively

**Deliberately out of scope:** the StretchDIBits/redirection-surface copy
described in the note after Stage 0. It's a fixed, small cost that's dwarfed
by the loop above until that loop is fast — and it gets eliminated as a side
effect of Stage 5's swap chain, not by further GDI tuning. Don't spend Stage 4
time on it.

**Exit criteria:** you have concrete before/after numbers for each optimization
(frame time or throughput), and a CPU renderer fast enough to be a legitimate
comparison point, not a strawman.

---

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