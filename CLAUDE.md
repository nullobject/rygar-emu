# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

An emulator for the 1986 Tecmo arcade game Rygar, written in C with SDL3 for
video/input. It emulates the main board only: a Z80 CPU plus the tilemap and
sprite video hardware. There is no sound board — writes to `SOUND_LATCH`
(0xf806) are discarded, and player 2 inputs / most DIP switches read back as 0.

## Build and run

```
make          # cc -Wall -Werror -ggdb, links SDL3 via pkg-config
./rygar
make clean
```

Requires SDL3 development headers discoverable by `pkg-config sdl3`.

There are no tests, no linter config, and no CI. Verification is by running the
emulator and eyeballing the output; press `P` in-game to dump `sprite.png`,
`char.png`, `foreground.png`, and `background.png` — one PNG per graphics layer
— which is the practical way to check a rendering change.

### ROM headers

`src/rygar.c` includes `roms/rygar-roms.h`, which is generated and gitignored:
one `const uint8_t dump_*[]` array per ROM. `make` builds it with
`tools/dump.py`, which reads the file list in `src/roms/rygar-roms.yml` and
dumps each binary as a C array. Symbol names are `dump_` plus the filename with
its final extension stripped — `5.5p` → `dump_5`, `cpu_5m.bin` → `dump_cpu_5m`.

The script parses the manifest by hand rather than depending on PyYAML, and
`tools/dump.py --list <manifest>` prints the ROM paths, which is how the
Makefile derives the header's prerequisites. It replaces the fips `dump` code
generator that was dropped along with sokol in `bee100a`.

Adding or renaming a ROM means editing the manifest only — the Makefile
prerequisites and the generated symbol names both follow from it. Adding a
source file means adding it to `SRCS` in the Makefile.

## Architecture

`src/rygar.c` is the machine: memory map, CPU tick callback, ROM decoding, and
the SDL3 callback-style app (`SDL_MAIN_USE_CALLBACKS`; `SDL_AppInit` /
`SDL_AppEvent` / `SDL_AppIterate` / `SDL_AppQuit`, no `main()`). Everything else
is a reusable video-hardware primitive.

`src/chips/{z80,mem,clk}.h` are vendored single-header libraries from
floooh/chips — treat them as third-party and don't hand-edit. `CHIPS_IMPL` (and
`STB_IMAGE_WRITE_IMPLEMENTATION` for `stb_image_write.h`) is defined exactly
once, in `rygar.c`.

### Emulation loop

`SDL_AppIterate` measures wall-clock delta (clamped to 24ms), converts it to
CPU ticks, and runs `rygar_tick_main` that many times, then draws a frame.
`rygar_tick_main` decrements the vsync/vblank counters, raises `Z80_INT` for
the duration of vblank, ticks the CPU, and then decodes the resulting pin mask
by address range: RAM writes go through `mem_wr` and additionally mark tilemap
tiles dirty or update the palette cache; the 0xf8xx range is memory-mapped I/O
where reads are inputs (joystick/buttons/system) and writes are outputs
(scroll registers, bank switch, sound latch).

Note `CPU_FREQ` is 6MHz but the derived timing constants are still named
`VSYNC_PERIOD_4MHZ` / `VBLANK_DURATION_4MHZ` — the names are stale, the values
follow `CPU_FREQ`.

### Memory map (main board)

| Range | Contents |
| --- | --- |
| 0x0000–0xbfff | program ROM (`dump_5`, `dump_cpu_5m`) |
| 0xc000–0xcfff | work RAM |
| 0xd000–0xd7ff | char (text layer) RAM |
| 0xd800–0xdbff | foreground tilemap RAM |
| 0xdc00–0xdfff | background tilemap RAM |
| 0xe000–0xe7ff | sprite RAM |
| 0xe800–0xefff | palette RAM |
| 0xf000–0xf7ff | banked ROM window (`dump_cpu_5j`, bank from D3–D6 of the 0xf808 write) |
| 0xf800–0xf80f | I/O — inputs on read, scroll/bank/sound-latch on write |

### Graphics pipeline

Tile ROMs are 4bpp planar and are decoded once at startup by `tile_decode`
into one-byte-per-pixel buffers (`rygar_decode_tiles`), trading memory for a
sequential inner loop at draw time. Char tiles are 8x8; foreground, background,
and sprite tiles are 16x16 built from four 8x8 tiles.

`bitmap_t` is the shared render target: a `uint16_t` palette-index plane plus a
parallel `uint8_t` priority plane. Drawing order in `rygar_draw` is background,
foreground, char, sprites, each with a palette offset (0x300/0x200/0x100/0x0)
and a layer flag (`TILE_LAYER3`…`TILE_LAYER0`). `tile_draw_pixel` writes the
layer flag into the priority plane and refuses to overwrite a pixel whose
priority intersects the caller's `priority_mask`; sprites derive that mask from
bits 6–7 of sprite byte 3, which is how a sprite ends up behind a specific
layer.

Each `tilemap_t` owns an internal bitmap the size of the whole (32x32 or 32x16)
tilemap. Only tiles marked dirty by CPU writes are re-rendered into it; the
visible region is then `bitmap_copy`'d to the frame bitmap with wrapping at the
tilemap edges. Tilemaps are drawn with `TILE_OPAQUE` forced on so stale pixels
in the internal buffer aren't visible through transparent pens.

The hardware palette is 1024 entries of 16-bit big-endian `xxxxBBBBRRRRGGGG`.
`rygar_update_palette` maintains a 32-bit RGBA cache on every palette RAM
write, so per-pixel drawing only ever writes palette indices and `apply_palette`
does one lookup per pixel at the end of the frame.

The frame buffer is 256x256 but only 256x224 is displayed — `rygar_draw` skips
the first 16 scanlines. Tilemap horizontal scroll values are offset by
`SCROLL_OFFSET` (48) to compensate for the CRT back porch.

## Conventions

- 2-space indent, LF, trailing whitespace trimmed, final newline (`.editorconfig`).
- Formatting follows clang-format LLVM style at 80 columns; match the
  surrounding code.
- `/* ... */` block comments; doc comments in `/** ... */` above the declaration
  in headers.
- Every source file carries the NULLOBJECT ASCII-art MIT header — copy it into
  any new file.
- Builds are `-Werror`; unused variables and missing switch cases will fail the
  build, not warn.
