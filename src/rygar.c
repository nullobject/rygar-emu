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

#define BETWEEN(n, a, b) ((n >= a) && (n <= b))

#define CHAR_ROM_SIZE (0x8000)
#define SPRITE_ROM_SIZE (0x20000)
#define TILE_ROM_SIZE (0x20000)

#define TX_RAM_START (0xd000)
#define FG_RAM_START (0xd800)
#define BG_RAM_START (0xdc00)
#define SPRITE_RAM_START (0xe000)
#define PALETTE_RAM_START (0xe800)

#define RAM_SIZE (0x3000)
#define RAM_START (0xc000)
#define RAM_END (RAM_START + RAM_SIZE - 1)

#define BANK_SIZE (0x8000)
#define BANK_WINDOW_SIZE (0x800)
#define BANK_WINDOW_START (0xf000)
#define BANK_WINDOW_END (BANK_WINDOW_START + BANK_WINDOW_SIZE - 1)

#define FLIP_SCREEN 0xf807
#define BANK_SWITCH 0xf808

#define NUM_BITPLANES (4)
#define TILE_SIZE (32) // bytes

#define DISPLAY_WIDTH (256)
#define DISPLAY_HEIGHT (256)

#define VSYNC_PERIOD_4MHZ (4000000 / 60)
#define VBLANK_DURATION_4MHZ (((4000000 / 60) / 525) * (525 - 483))

typedef struct {
  z80_t cpu;
  clk_t clk;
  int vsync_count;
  int vblank_count;
  mem_t mem;
  uint8_t current_bank;
} mainboard_t;

typedef struct {
  mainboard_t main;
  uint8_t main_ram[RAM_SIZE];
  uint8_t main_bank[BANK_SIZE];
  uint8_t char_rom[CHAR_ROM_SIZE];
  uint8_t sprite_rom[SPRITE_ROM_SIZE];
  uint8_t tile_rom_1[TILE_ROM_SIZE];
  uint8_t tile_rom_2[TILE_ROM_SIZE];
} rygar_t;

rygar_t rygar;

/**
 * Handle IO
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
      /* printf("W 0x%04x 0x%02x \n", addr, data); */
      /* if ((addr >= RAM_START) && (addr <= RAM_END)) { */
      if (BETWEEN(addr, RAM_START, RAM_END)) {
        mem_wr(&rygar.main.mem, addr, data);
      } else if (addr == BANK_SWITCH) {
        rygar.main.current_bank = data >> 3; // bank addressed by DO3-DO6 in schematic
      }
    } else if (pins & Z80_RD) {
      /* printf("R 0x%04x\n", addr); */
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

  clk_init(&rygar.main.clk, 6000000);
  z80_init(&rygar.main.cpu, &(z80_desc_t) { .tick_cb = rygar_tick_main });

  // 0000-bfff ROM
  // c000-cfff WORK RAM
  // d000-d7ff TX VIDEO RAM
  // d800-dbff FG VIDEO RAM
  // dc00-dfff BG VIDEO RAM
  // e000-e7ff SPRITE RAM
  // e800-efff PALETTE
  // f000-f7ff WINDOW FOR BANKED ROM
  // f800-ffff ?
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
 * Draws a single 8x8 tile.
 */
static void draw_tile(uint32_t* buffer, uint16_t index, uint8_t color) {
  for (int y = 0; y < 8; y++) {
    int tile_addr = index*TILE_SIZE + y*NUM_BITPLANES;
    uint32_t* ptr = buffer + y*DISPLAY_WIDTH;

    for (int x = 0; x < 4; x++) {
      // The bytes in the char ROM are divided into high and low pixels for
      // each bitplane.
      uint8_t hi = (rygar.char_rom[tile_addr + x] >> 4) & 0xf;
      uint8_t lo = rygar.char_rom[tile_addr + x] & 0xf;

      // FIXME: Lookup color from palette RAM.
      *ptr++ = 0xff000000 | (hi << 21) | (hi << 13) | (hi << 5);
      *ptr++ = 0xff000000 | (lo << 21) | (lo << 13) | (lo << 5);
    }
  }
}

/**
 * Draws 32x32 char tiles.
 */
static void rygar_draw_tx_tiles(uint32_t* buffer) {
  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 32; x++) {
      int addr = TX_RAM_START - RAM_START + y*32 + x;
      uint8_t color = rygar.main_ram[addr + 0x400];
      // 10-bit tile index (8-bits from tile value, with the 9th and 10th bits
      // borrowed from the color value).
      uint16_t index = rygar.main_ram[addr] + ((color & 0x03) << 8);
      uint32_t* ptr = buffer + y*DISPLAY_WIDTH*8 + x*8;
      draw_tile(ptr, index, color >> 4);
    }
  }
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

  uint32_t* buffer = gfx_framebuffer();
  rygar_draw_tx_tiles(buffer);
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
