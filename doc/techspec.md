# Technical specification

## Guiding principle

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
running as a CPU loop or a GPU shader.

### Note: what the GDI present blit actually costs, and when to stop paying it

- **GDI is CPU-only on modern Windows.** Hardware GDI acceleration was
  deprecated starting with Windows 8 — `SetDIBitsToDevice` runs in software,
  not on the GPU, same as your blend loop.
- **A GDI-drawn window gets composited through an extra copy.** Windows keeps
  a separate "redirection surface" for GDI windows; the present blit copies
  your buffer into that surface, and DWM composites the surface onto the
  desktop separately. That's two full-frame copies per present, not one, and
  the second one is invisible to your code.
- **It goes away for free later, not by tuning GDI harder.** Presenting
  through a DXGI flip-model swap chain instead — which Stage 5 sets up anyway,
  for the GPU backend — lets DWM scan the surface out directly in the common
  case, collapsing the two copies into one, and adds real vsync control
  (`Present(1, 0)`) that GDI has no equivalent for.

## Definitions

- Structure:
    - tree
    - line
    - run
    - symbol
    - file
    - group
    - buffer
- primitives: bytes -> lines -> runs -> codePoints -> grapheme (char) -> glyph

## Rules

- It should be multi-threaded by default
