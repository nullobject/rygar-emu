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

#define DISPLAY_WIDTH (256)
#define DISPLAY_HEIGHT (256)

#define VSYNC_PERIOD_4MHZ (4000000 / 60)
#define VBLANK_DURATION_4MHZ (((4000000 / 60) / 525) * (525 - 483))
#define VSYNC_PERIOD_3MHZ (3000000 / 60)

typedef struct {
  z80_t cpu;
  clk_t clk;
  int vsync_count;
  int vblank_count;
  mem_t mem;
} mainboard_t;

typedef struct {
  mainboard_t main;
  uint8_t ram[0x1000];
  uint8_t tx_video_ram[0x800];
  uint8_t fg_video_ram[0x400];
  uint8_t bg_video_ram[0x400];
  uint8_t sprite_ram[0x800];
  uint8_t palette[0x800];
} rygar_t;

rygar_t rygar;

static uint64_t rygar_tick_main(int num_ticks, uint64_t pins, void* user_data) {
  return pins;
}

/**
 * Initialise the rygar arcade hardware.
 *
 * 0000-7fff ROM0 (5P)
 * 8000-bfff ROM1 (5M)
 * c000-cfff RAM
 * d000-d7ff TX VIDEO RAM
 * d800-dbff FG VIDEO RAM
 * dc00-dfff BG VIDEO RAM
 * e000-e7ff SPRITE RAM
 * e800-efff PALETTE
 * f000-f7ff ROM2 (5J)
 * f800-ffff ?
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
  mem_map_ram(&rygar.main.mem, 0, 0xc000, 0x1000, rygar.ram);
  mem_map_ram(&rygar.main.mem, 0, 0xd000, 0x800, rygar.tx_video_ram);
  mem_map_ram(&rygar.main.mem, 0, 0xd800, 0x400, rygar.fg_video_ram);
  mem_map_ram(&rygar.main.mem, 0, 0xdc00, 0x400, rygar.bg_video_ram);
  mem_map_ram(&rygar.main.mem, 0, 0xe000, 0x800, rygar.sprite_ram);
  mem_map_ram(&rygar.main.mem, 0, 0xe800, 0x800, rygar.palette);
  mem_map_rom(&rygar.main.mem, 0, 0xf000, 0x800, dump_cpu_5j);
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
}

static void app_init(void) {
  gfx_init(&(gfx_desc_t) {
    .aspect_x = 4,
    .aspect_y = 5,
    .rot90 = true
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
    .width = DISPLAY_WIDTH * 2,
    .height = DISPLAY_HEIGHT * 2,
    .window_title = "Rygar"
  };
}
