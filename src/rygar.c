#include <stdint.h>
#include <stdio.h>

#define CHIPS_IMPL
#define COMMON_IMPL

#include "chips/clk.h"
#include "chips/mem.h"
#include "chips/z80.h"
#include "clock.h"
#include "gfx.h"
#include "rygar-roms.h"
#include "sokol_app.h"
#include "tilemap.h"

#define BETWEEN(n, a, b) ((n >= a) && (n <= b))

#define CHAR_ROM_SIZE 0x8000
#define SPRITE_ROM_SIZE 0x20000
#define TILE_ROM_SIZE 0x20000

#define CHAR_RAM_START 0xd000
#define FG_RAM_START 0xd800
#define BG_RAM_START 0xdc00
#define SPRITE_RAM_START 0xe000

#define PALETTE_RAM_SIZE 0x800
#define PALETTE_RAM_START 0xe800
#define PALETTE_RAM_END (PALETTE_RAM_START + PALETTE_RAM_SIZE - 1)

#define RAM_SIZE 0x3000
#define RAM_START 0xc000
#define RAM_END (RAM_START + RAM_SIZE - 1)

#define BANK_SIZE 0x8000
#define BANK_WINDOW_SIZE 0x800
#define BANK_WINDOW_START 0xf000
#define BANK_WINDOW_END (BANK_WINDOW_START + BANK_WINDOW_SIZE - 1)

// write
#define FG_SCROLL_START 0xf800
#define FG_SCROLL_END 0xf802
#define BG_SCROLL_START 0xf803
#define BG_SCROLL_END 0xf805
#define SOUND_LATCH 0xf807
#define FLIP_SCREEN 0xf807
#define BANK_SWITCH 0xf808

#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 256

// The tilemap horizontal scroll values are offset by a fixed value, due to
// hardware timing constraints, etc. We don't need an adjusted scroll value, so
// we need to correct it.
#define SCROLL_OFFSET 48

#define VSYNC_PERIOD_4MHZ (4000000 / 60)
#define VBLANK_DURATION_4MHZ (((4000000 / 60) / 525) * (525 - 483))

typedef struct {
  z80_t cpu;
  clk_t clk;
  int vsync_count;
  int vblank_count;
  mem_t mem;
  uint8_t current_bank;
  uint8_t fg_scroll[3]; // XXY
  uint8_t bg_scroll[3]; // XXY
} mainboard_t;

typedef struct {
  mainboard_t main;
  uint8_t main_ram[RAM_SIZE];
  uint8_t main_bank[BANK_SIZE];
  uint8_t char_rom[CHAR_ROM_SIZE];
  uint8_t sprite_rom[SPRITE_ROM_SIZE];
  uint8_t tile_rom_1[TILE_ROM_SIZE];
  uint8_t tile_rom_2[TILE_ROM_SIZE];
  /* 32-bit RGBA color palette cache */
  uint32_t palette_cache[1024];
} rygar_t;

rygar_t rygar;

/*
 * Updates the color palette cache with 32-bit colors, this is called for CPU
 * writes to the palette RAM area.
 *
 * The hardware palette contains 1024 entries of 16-bit big-endian color values
 * (xxxxBBBBRRRRGGGG). This function keeps a palette cache with 32-bit colors
 * up to date, so that the 32-bit colors don't need to be computed for each
 * pixel in the video decoding code.
 */
static inline void rygar_update_palette_cache(uint16_t addr, uint8_t data) {
  assert((addr >= PALETTE_RAM_START) && (addr <= PALETTE_RAM_END));

  int pal_index = (addr - PALETTE_RAM_START) / 2;
  uint32_t c = rygar.palette_cache[pal_index];

  if (addr & 1) {
    /* odd addresses are the RRRRGGGG part */
    uint8_t r = (data & 0xf0) | (data>>4 & 0x0f);
    uint8_t g = (data & 0x0f) | (data<<4 & 0xf0);
    c = 0xff000000 | (c & 0x00ff0000) | g<<8 | r;
  } else {
    /* even addresses are the xxxxBBBB part */
    uint8_t b = (data & 0x0f) | (data<<4 & 0xf0);
    c = 0xff000000 | (c & 0x0000ffff) | b << 16;
  }

  rygar.palette_cache[pal_index] = c;
}

/**
 * Handle IO
 *
 * 0000-bfff ROM
 * c000-cfff WORK RAM
 * d000-d7ff CHAR VIDEO RAM
 * d800-dbff FG VIDEO RAM
 * dc00-dfff BG VIDEO RAM
 * e000-e7ff SPRITE RAM
 * e800-efff PALETTE
 * f000-f7ff WINDOW FOR BANKED ROM
 * f800-ffff ?
 *
 * f800-f802 FG SCROLL
 * f803-f805 BG SCROLL
 * f806      SOUND LATCH
 * f807      FLIP SCREEN
 * f808      BANK SWITCH
 */
static uint64_t rygar_tick_main(int num_ticks, uint64_t pins, void* user_data) {
  rygar.main.vsync_count -= num_ticks;

  if (rygar.main.vsync_count <= 0) {
    rygar.main.vsync_count += VSYNC_PERIOD_4MHZ;
    rygar.main.vblank_count = VBLANK_DURATION_4MHZ;
  }

  if (rygar.main.vblank_count > 0) {
    rygar.main.vblank_count -= num_ticks;
    pins |= Z80_INT; // activate INT pin during VBLANK
  } else {
    rygar.main.vblank_count = 0;
  }

  uint16_t addr = Z80_GET_ADDR(pins);

  if (pins & Z80_MREQ) {
    if (pins & Z80_WR) {
      uint8_t data = Z80_GET_DATA(pins);

      if (BETWEEN(addr, RAM_START, RAM_END)) {
        mem_wr(&rygar.main.mem, addr, data);

        // Update the palette cache if we're writing to the palette RAM.
        if (BETWEEN(addr, PALETTE_RAM_START, PALETTE_RAM_END)) {
          rygar_update_palette_cache(addr, data);
        }
      } else if (BETWEEN(addr, FG_SCROLL_START, FG_SCROLL_END)) {
        uint8_t offset = addr - FG_SCROLL_START;
        rygar.main.fg_scroll[offset] = data;
      } else if (BETWEEN(addr, BG_SCROLL_START, BG_SCROLL_END)) {
        uint8_t offset = addr - BG_SCROLL_START;
        rygar.main.bg_scroll[offset] = data;
      } else if (addr == BANK_SWITCH) {
        rygar.main.current_bank = data >> 3; // bank addressed by DO3-DO6 in schematic
      }
    } else if (pins & Z80_RD) {
      if (addr <= RAM_END) {
        Z80_SET_DATA(pins, mem_rd(&rygar.main.mem, addr));
      } else if (BETWEEN(addr, BANK_WINDOW_START, BANK_WINDOW_END)) {
        uint16_t banked_addr = addr - BANK_WINDOW_START + (rygar.main.current_bank * BANK_WINDOW_SIZE);
        Z80_SET_DATA(pins, rygar.main_bank[banked_addr]);
      } else {
        Z80_SET_DATA(pins, 0x00);
      }
    }
  } else if ((pins & Z80_IORQ) && (pins & Z80_M1)) {
    // Clear interrupt.
    pins &= ~Z80_INT;
  }

  return pins;
}

/**
 * Initialise the rygar arcade hardware.
 */
static void rygar_init(void) {
  memset(&rygar, 0, sizeof(rygar));

  rygar.main.vsync_count = VSYNC_PERIOD_4MHZ;
  rygar.main.vblank_count = 0;

  clk_init(&rygar.main.clk, 4000000);
  z80_init(&rygar.main.cpu, &(z80_desc_t) { .tick_cb = rygar_tick_main });

  mem_init(&rygar.main.mem);
  mem_map_rom(&rygar.main.mem, 0, 0x0000, 0x8000, dump_5);
  mem_map_rom(&rygar.main.mem, 0, 0x8000, 0x4000, dump_cpu_5m);
  mem_map_ram(&rygar.main.mem, 0, RAM_START, RAM_SIZE, rygar.main_ram);

  // banked ROM at f000-f7ff
  memcpy(&rygar.main_bank[0x00000], dump_cpu_5j, 0x8000);

  // char ROM
  memcpy(&rygar.char_rom[0x00000], dump_cpu_8k, 0x8000);

  // sprite ROM
  memcpy(&rygar.sprite_rom[0x00000], dump_vid_6k, 0x8000);
  memcpy(&rygar.sprite_rom[0x08000], dump_vid_6j, 0x8000);
  memcpy(&rygar.sprite_rom[0x10000], dump_vid_6h, 0x8000);
  memcpy(&rygar.sprite_rom[0x18000], dump_vid_6g, 0x8000);

  // tile ROM #1
  memcpy(&rygar.tile_rom_1[0x00000], dump_vid_6p, 0x8000);
  memcpy(&rygar.tile_rom_1[0x08000], dump_vid_6o, 0x8000);
  memcpy(&rygar.tile_rom_1[0x10000], dump_vid_6n, 0x8000);
  memcpy(&rygar.tile_rom_1[0x18000], dump_vid_6l, 0x8000);

  // tile ROM #2
  memcpy(&rygar.tile_rom_2[0x00000], dump_vid_6f, 0x8000);
  memcpy(&rygar.tile_rom_2[0x08000], dump_vid_6e, 0x8000);
  memcpy(&rygar.tile_rom_2[0x10000], dump_vid_6c, 0x8000);
  memcpy(&rygar.tile_rom_2[0x18000], dump_vid_6b, 0x8000);
}

/**
 * Run the emulation for one frame.
 */
static void rygar_exec(uint32_t delta) {
  uint32_t ticks_to_run = clk_ticks_to_run(&rygar.main.clk, delta);
  uint32_t ticks_executed = 0;

  while (ticks_executed < ticks_to_run) {
    ticks_executed += z80_exec(&rygar.main.cpu, ticks_to_run);
  }
  clk_ticks_executed(&rygar.main.clk, ticks_executed);

  // Calculate scroll offsets.
  uint16_t fg_scroll_offset = (rygar.main.fg_scroll[1]<<8 | rygar.main.fg_scroll[0]) + SCROLL_OFFSET;
  uint16_t bg_scroll_offset = (rygar.main.bg_scroll[1]<<8 | rygar.main.bg_scroll[0]) + SCROLL_OFFSET;

  uint32_t* buffer = gfx_framebuffer();

  // Clear buffer.
  memset(buffer, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(buffer[0]));

  // Draw graphics layers.
  tilemap_draw_16x16(buffer, rygar.palette_cache + 0x300, rygar.tile_rom_2, rygar.main_ram + BG_RAM_START - RAM_START, 16, 16, 32, 16, bg_scroll_offset);
  tilemap_draw_16x16(buffer, rygar.palette_cache + 0x200, rygar.tile_rom_1, rygar.main_ram + FG_RAM_START - RAM_START, 16, 16, 32, 16, fg_scroll_offset);
  tilemap_draw_32x32(buffer, rygar.palette_cache + 0x100, rygar.char_rom, rygar.main_ram + CHAR_RAM_START - RAM_START, 8, 8, 32, 32);
}

static void app_init(void) {
  gfx_init(&(gfx_desc_t) {
    .aspect_x = 4,
    .aspect_y = 3,
    .rot90 = false
  });
  clock_init();
  rygar_init();
}

static void app_frame(void) {
  rygar_exec(clock_frame_time());
  gfx_draw(DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

static void app_input(const sapp_event* event) {
}

static void app_cleanup(void) {
  gfx_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
  return (sapp_desc) {
    .init_cb = app_init,
    .frame_cb = app_frame,
    .event_cb = app_input,
    .cleanup_cb = app_cleanup,
    .width = DISPLAY_WIDTH * 4,
    .height = DISPLAY_HEIGHT * 3,
    .window_title = "Rygar"
  };
}
