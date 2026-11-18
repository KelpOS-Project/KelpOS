# stb_image for KelpOS

Vendored copy of **stb_image v2.32** (public domain / MIT, see the
license block at the end of `stb_image.h`):

- Upstream: <https://github.com/nothings/stb>
- File: `stb_image.h` (unchanged)
- Wrapper: `stb_image_impl.c` (KelpOS build configuration)
- API for the rest of the kernel: `kernel/gfx/image.h`

## Build configuration

`stb_image_impl.c` defines:

- `STBI_NO_STDIO` — images are decoded from memory only; the kernel has
  no `FILE`.
- `STBI_NO_SIMD` — scalar decoder. `kernel/gfx` is SSE-enabled, but the
  SIMD paths are not needed for wallpaper-sized images and keep the
  code smaller.
- `STBI_NO_THREAD_LOCALS` — no TLS in the kernel.
- `STBI_REALLOC_SIZED` — a copying wrapper. klibc's `realloc()` cannot
  copy (the bump allocator does not know the old size), and stb_image
  relies on `REALLOC_SIZED` preserving the zlib buffers.
- `STBI_ASSERT` — the KelpOS graphics `assert()` (prints and traps).

## Usage

`image_load_rgb32()` decodes PNG, JPEG, BMP, GIF, TGA, ... from a
memory buffer and returns packed `0xAARRGGBB` pixels, which is the
format `console_blit_rgb32()` expects. The desktop uses it for
`wallpaper.bmp`; any supported format can replace that boot module
without code changes.
