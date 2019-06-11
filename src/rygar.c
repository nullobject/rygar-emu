#include <stdint.h>
#include <stdio.h>

#define CHIPS_IMPL
#define COMMON_IMPL

#include "chips/clk.h"
#include "chips/z80.h"
#include "clock.h"
#include "gfx.h"
#include "mem.h"
#include "rygar-roms.h"
#include "sokol_app.h"
#include "tilemap.h"

#define BETWEEN(n, a, b) ((n >= a) && (n <= b))

#define TX_ROM_SIZE 0x8000
#define FG_ROM_SIZE 0x20000
#define BG_ROM_SIZE 0x20000
#define SPRITE_ROM_SIZE 0x20000

#define TX_RAM_SIZE 0x800
#define TX_RAM_START 0xd000

#define FG_RAM_SIZE 0x400
#define FG_RAM_START 0xd800

#define BG_RAM_SIZE 0x400
#define BG_RAM_START 0xdc00

#define SPRITE_RAM_SIZE 0x800
#define SPRITE_RAM_START 0xe000

#define PALETTE_RAM_SIZE 0x800
#define PALETTE_RAM_START 0xe800
#define PALETTE_RAM_END (PALETTE_RAM_START + PALETTE_RAM_SIZE - 1)

#define WORK_RAM_SIZE 0x1000
#define WORK_RAM_START 0xc000

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

#define CPU_FREQ 4000000
#define VSYNC_PERIOD_4MHZ (4000000 / 60)
#define VBLANK_DURATION_4MHZ (((4000000 / 60) / 525) * (525 - 483))

typedef struct {
  clk_t clk;
  z80_t cpu;
  mem_t mem;

  // ram
  uint8_t work_ram[WORK_RAM_SIZE];
  uint8_t tx_ram[TX_RAM_SIZE];
  uint8_t fg_ram[FG_RAM_SIZE];
  uint8_t bg_ram[BG_RAM_SIZE];
  uint8_t sprite_ram[SPRITE_RAM_SIZE];
  uint8_t palette_ram[PALETTE_RAM_SIZE];

  // bank switched rom
  uint8_t banked_rom[BANK_SIZE];
  uint8_t current_bank;

  // gfx roms
  mem_t tx_rom;
  mem_t fg_rom;
  mem_t bg_rom;
  mem_t sprite_rom;

  // tilemap scroll offset registers
  uint8_t fg_scroll[3];
  uint8_t bg_scroll[3];
} mainboard_t;

typedef struct {
  mainboard_t main;

  // tilemaps
  tilemap_t tx_tilemap;
  tilemap_t fg_tilemap;
  tilemap_t bg_tilemap;

  // 32-bit RGBA color palette cache
  uint32_t palette_cache[1024];

  // counters
  int vsync_count;
  int vblank_count;
} rygar_t;

rygar_t rygar;

/**
 * Updates the color palette cache with 32-bit colors, this is called for CPU
 * writes to the palette RAM area.
 *
 * The hardware palette contains 1024 entries of 16-bit big-endian color values
 * (xxxxBBBBRRRRGGGG). This function keeps a palette cache with 32-bit colors
 * up to date, so that the 32-bit colors don't need to be computed for each
 * pixel in the video decoding code.
 */
static inline void rygar_update_palette_cache(uint16_t addr, uint8_t data) {
  uint16_t pal_index = addr>>1;
  uint32_t c = rygar.palette_cache[pal_index];

  if (addr & 1) {
    // odd addresses are the RRRRGGGG part
    uint8_t r = (data & 0xf0) | (data>>4 & 0x0f);
    uint8_t g = (data & 0x0f) | (data<<4 & 0xf0);
    c = 0xff000000 | (c & 0x00ff0000) | g<<8 | r;
  } else {
    // even addresses are the xxxxBBBB part
    uint8_t b = (data & 0x0f) | (data<<4 & 0xf0);
    c = 0xff000000 | (c & 0x0000ffff) | b << 16;
  }

  rygar.palette_cache[pal_index] = c;
}

/**
 * This callback function is called for every CPU tick.
 */
static uint64_t rygar_tick_main(int num_ticks, uint64_t pins, void* user_data) {
  rygar.vsync_count -= num_ticks;

  if (rygar.vsync_count <= 0) {
    rygar.vsync_count += VSYNC_PERIOD_4MHZ;
    rygar.vblank_count = VBLANK_DURATION_4MHZ;
  }

  if (rygar.vblank_count > 0) {
    rygar.vblank_count -= num_ticks;
    pins |= Z80_INT; // activate INT pin during VBLANK
  } else {
    rygar.vblank_count = 0;
  }

  uint16_t addr = Z80_GET_ADDR(pins);

  if (pins & Z80_MREQ) {
    if (pins & Z80_WR) {
      uint8_t data = Z80_GET_DATA(pins);

      if (BETWEEN(addr, RAM_START, RAM_END)) {
        mem_wr(&rygar.main.mem, addr, data);

        if (BETWEEN(addr, TX_RAM_START, TX_RAM_START + 0x400)) {
          tilemap_mark_tile_dirty(&rygar.tx_tilemap, addr - TX_RAM_START);
        } else if (BETWEEN(addr, FG_RAM_START, FG_RAM_START + 0x200)) {
          tilemap_mark_tile_dirty(&rygar.fg_tilemap, addr - FG_RAM_START);
        } else if (BETWEEN(addr, BG_RAM_START, BG_RAM_START + 0x200)) {
          tilemap_mark_tile_dirty(&rygar.bg_tilemap, addr - BG_RAM_START);
        } else if (BETWEEN(addr, PALETTE_RAM_START, PALETTE_RAM_END)) {
          rygar_update_palette_cache(addr - PALETTE_RAM_START, data);
        }
      } else if (BETWEEN(addr, FG_SCROLL_START, FG_SCROLL_END)) {
        uint8_t offset = addr - FG_SCROLL_START;
        rygar.main.fg_scroll[offset] = data;
        tilemap_set_scroll_x(&rygar.fg_tilemap, (rygar.main.fg_scroll[1]<<8 | rygar.main.fg_scroll[0]) + SCROLL_OFFSET);
      } else if (BETWEEN(addr, BG_SCROLL_START, BG_SCROLL_END)) {
        uint8_t offset = addr - BG_SCROLL_START;
        rygar.main.bg_scroll[offset] = data;
        tilemap_set_scroll_x(&rygar.bg_tilemap, (rygar.main.bg_scroll[1]<<8 | rygar.main.bg_scroll[0]) + SCROLL_OFFSET);
      } else if (addr == BANK_SWITCH) {
        rygar.main.current_bank = data >> 3; // bank addressed by DO3-DO6 in schematic
      }
    } else if (pins & Z80_RD) {
      if (addr <= RAM_END) {
        Z80_SET_DATA(pins, mem_rd(&rygar.main.mem, addr));
      } else if (BETWEEN(addr, BANK_WINDOW_START, BANK_WINDOW_END)) {
        uint16_t banked_addr = addr - BANK_WINDOW_START + (rygar.main.current_bank * BANK_WINDOW_SIZE);
        Z80_SET_DATA(pins, rygar.main.banked_rom[banked_addr]);
      } else {
        Z80_SET_DATA(pins, 0);
      }
    }
  } else if ((pins & Z80_IORQ) && (pins & Z80_M1)) {
    // Clear interrupt.
    pins &= ~Z80_INT;
  }

  return pins;
}

static void tx_tile_info(tile_t* tile, uint16_t index) {
  uint8_t lo = rygar.main.tx_ram[index];
  uint8_t hi = rygar.main.tx_ram[index + 0x400];

  // The tile code is a 10-bit value, represented by the low byte and the two
  // LSBs of the high byte.
  tile->code = (hi & 0x03)<<8 | lo;

  // The four MSBs of the high byte represent the color value.
  tile->color = hi>>4;
}

static void fg_tile_info(tile_t* tile, uint16_t index) {
  uint8_t lo = rygar.main.fg_ram[index];
  uint8_t hi = rygar.main.fg_ram[index + 0x200];

  // The tile code is a 11-bit value, represented by the low byte and the three
  // LSBs of the high byte.
  tile->code = (hi & 0x07)<<8 | lo;

  // The four MSBs of the high byte represent the color value.
  tile->color = hi>>4;
}

static void bg_tile_info(tile_t* tile, uint16_t index) {
  uint8_t lo = rygar.main.bg_ram[index];
  uint8_t hi = rygar.main.bg_ram[index + 0x200];

  // The tile code is a 11-bit value, represented by the low byte and the three
  // LSBs of the high byte.
  tile->code = (hi & 0x07)<<8 | lo;

  // The four MSBs of the high byte represent the color value.
  tile->color = hi>>4;
}

/**
 * Initialises the Rygar arcade hardware.
 */
static void rygar_init() {
  memset(&rygar, 0, sizeof(rygar));

  rygar.vsync_count = VSYNC_PERIOD_4MHZ;

  clk_init(&rygar.main.clk, CPU_FREQ);
  z80_init(&rygar.main.cpu, &(z80_desc_t) { .tick_cb = rygar_tick_main });
  mem_init(&rygar.main.mem);

  // main memory
  mem_map_rom(&rygar.main.mem, 0, 0x0000, 0x8000, dump_5);
  mem_map_rom(&rygar.main.mem, 0, 0x8000, 0x4000, dump_cpu_5m);
  mem_map_ram(&rygar.main.mem, 0, WORK_RAM_START, WORK_RAM_SIZE, rygar.main.work_ram);
  mem_map_ram(&rygar.main.mem, 0, TX_RAM_START, TX_RAM_SIZE, rygar.main.tx_ram);
  mem_map_ram(&rygar.main.mem, 0, FG_RAM_START, FG_RAM_SIZE, rygar.main.fg_ram);
  mem_map_ram(&rygar.main.mem, 0, BG_RAM_START, BG_RAM_SIZE, rygar.main.bg_ram);
  mem_map_ram(&rygar.main.mem, 0, SPRITE_RAM_START, SPRITE_RAM_SIZE, rygar.main.sprite_ram);
  mem_map_ram(&rygar.main.mem, 0, PALETTE_RAM_START, PALETTE_RAM_SIZE, rygar.main.palette_ram);

  // banked rom
  memcpy(&rygar.main.banked_rom[0x00000], dump_cpu_5j, 0x8000);

  // tx rom
  mem_map_rom(&rygar.main.tx_rom, 0, 0x00000, 0x8000, dump_cpu_8k);

  // fg rom
  mem_map_rom(&rygar.main.fg_rom, 0, 0x00000, 0x8000, dump_vid_6p);
  mem_map_rom(&rygar.main.fg_rom, 0, 0x08000, 0x8000, dump_vid_6o);
  mem_map_rom(&rygar.main.fg_rom, 0, 0x10000, 0x8000, dump_vid_6n);
  mem_map_rom(&rygar.main.fg_rom, 0, 0x18000, 0x8000, dump_vid_6l);

  // bg rom
  mem_map_rom(&rygar.main.bg_rom, 0, 0x00000, 0x8000, dump_vid_6f);
  mem_map_rom(&rygar.main.bg_rom, 0, 0x08000, 0x8000, dump_vid_6e);
  mem_map_rom(&rygar.main.bg_rom, 0, 0x10000, 0x8000, dump_vid_6c);
  mem_map_rom(&rygar.main.bg_rom, 0, 0x18000, 0x8000, dump_vid_6b);

  // sprite rom
  mem_map_rom(&rygar.main.sprite_rom, 0, 0x00000, 0x8000, dump_vid_6k);
  mem_map_rom(&rygar.main.sprite_rom, 0, 0x08000, 0x8000, dump_vid_6j);
  mem_map_rom(&rygar.main.sprite_rom, 0, 0x10000, 0x8000, dump_vid_6h);
  mem_map_rom(&rygar.main.sprite_rom, 0, 0x18000, 0x8000, dump_vid_6g);

  tilemap_init(&rygar.tx_tilemap, &(tilemap_desc_t) {
    .tile_cb = tx_tile_info,
    .rom = &rygar.main.tx_rom,
    .tile_width = 8,
    .tile_height = 8,
    .cols = 32,
    .rows = 32
  });

  tilemap_init(&rygar.fg_tilemap, &(tilemap_desc_t) {
    .tile_cb = fg_tile_info,
    .rom = &rygar.main.fg_rom,
    .tile_width = 16,
    .tile_height = 16,
    .cols = 32,
    .rows = 16
  });

  tilemap_init(&rygar.bg_tilemap, &(tilemap_desc_t) {
    .tile_cb = bg_tile_info,
    .rom = &rygar.main.bg_rom,
    .tile_width = 16,
    .tile_height = 16,
    .cols = 32,
    .rows = 16
  });
}

/**
 * Runs the emulation for one frame.
 */
static void rygar_exec(uint32_t delta) {
  uint32_t ticks_to_run = clk_ticks_to_run(&rygar.main.clk, delta);
  uint32_t ticks_executed = 0;

  while (ticks_executed < ticks_to_run) {
    ticks_executed += z80_exec(&rygar.main.cpu, ticks_to_run);
  }
  clk_ticks_executed(&rygar.main.clk, ticks_executed);

  uint32_t* buffer = gfx_framebuffer();

  // Clear buffer.
  memset(buffer, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(buffer[0]));

  // Draw graphics layers.
  tilemap_draw(&rygar.bg_tilemap, buffer, rygar.palette_cache + 0x300, 3);
  tilemap_draw(&rygar.fg_tilemap, buffer, rygar.palette_cache + 0x200, 2);
  tilemap_draw(&rygar.tx_tilemap, buffer, rygar.palette_cache + 0x100, 1);
}

static void app_init() {
  gfx_init(&(gfx_desc_t) {
    .aspect_x = 4,
    .aspect_y = 3,
    .rot90 = false
  });
  clock_init();
  rygar_init();
}

static void app_frame() {
  rygar_exec(clock_frame_time());
  gfx_draw(DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

static void app_input(const sapp_event* event) {
}

static void app_cleanup() {
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
