# TinyGL for KelpOS — porting notes

Vendored source of **TinyGL 0.4** by Fabrice Bellard, from the maintained
fork by ska80:

- Upstream: <https://github.com/ska80/tinygl>
- Commit: `f42881c53401c49879f33690d61e5edced67cdbb`
- License: MIT, see `LICENSE` in this directory
- Upstream files kept unchanged wherever possible

TinyGL is a small software implementation of a large subset of OpenGL
1.1: immediate mode, matrices, Z-buffered triangle/line rasterisation,
flat and Gouraud shading, lighting, textures, display lists, vertex
arrays, selection and polygon offset.

## Layout

```
kernel/gfx/tinygl/
├── LICENSE            upstream MIT license (Fabrice Bellard)
├── PORTING.md         this file
├── include/GL/gl.h    upstream public GL header
├── include/GL/glu.h   GLU subset (upstream examples/glu.h)
└── src/               upstream src/*.c and headers, plus examples/glu.c
```

`kernel/gfx/tgl_surface.c` is the KelpOS platform layer: it owns one
`ZBuffer` per GL window and converts a finished frame to RGB32 with
`ZB_copyFrameBuffer()`. `kernel/gfx/apps/` contains the demo shells.

## Not ported

The platform/windowing layers are intentionally absent:

- `src/glx.c`, `src/nglx.c` (X11/GLX)
- `src/oscontext.c` (BeOS/OSMesa-style context)
- `BeOS/` (BGLView)

KelpOS replaces them with `tgl_surface_open()/present()`.

## Changes against upstream

Compile/link changes needed in a freestanding kernel:

- `src/zgl.h`: `dprintf()` is a no-op (no `stderr`, no `fprintf`).
- `src/error.c`: `gl_fatal_error()` prints with `kprintf()` and traps
  instead of `vfprintf(stderr) + exit(1)`.
- `src/get.c`: unknown-pname warning uses `kprintf()`.
- `src/matrix.c`: the debug matrix printer is a no-op.
- `src/list.c`: the unused display-list dumper (`gl_print_op`, used
  `FILE *`) and its `op_table_str` table were removed; `gl_add_op()`
  no longer calls them.
- `src/zbuffer.c`: the RGB32 conversion sets the alpha byte to `0xFF`
  so the output matches KelpOS' `0xAARRGGBB` framebuffer format.
- `src/glu.c` + `include/GL/glu.h`: the GLU subset from
  `examples/glu.{c,h}` moved into the library, with an include guard and
  `<GL/gl.h>` include.
- `src/msghandling.c`: unchanged; it compiles its debug output away
  because upstream defines `NDEBUG` there.

The renderer itself (rasteriser, lighting, textures, display lists,
matrix math) is untouched upstream code.

## Build integration

- `kernel/gfx/**` is compiled with SSE (`-msse2 -mfpmath=sse`) because
  x86_64 returns floats in XMM registers. `fpu_init()` runs early in
  `kmain()`.
- The rest of the kernel stays `-mno-sse`; interrupt handlers never
  touch XMM.
- The GL window entry points take **integer** milliseconds, not
  `float`: calling a float-parameter function from a `-mno-sse` object
  (the desktop) would not pass the argument in XMM0.
- `klibc` gained `calloc()`; TinyGL and stb_image allocate through the
  bump allocator (`free` is a no-op until PMM/VMM).
- The boot stack was raised to 64 KiB for the deeper graphics/stb
  call chains.
- The vendored code is built with `-Wno-unused-parameter`,
  `-Wno-sign-compare` and `-Wno-implicit-fallthrough`; it is not
  warning-clean under `-Wextra`.

## Updating

Re-copy `src/`, `include/GL/gl.h` and `LICENSE`, then re-apply the
changes above. Keep `PORTING.md` in sync and update the commit hash.
