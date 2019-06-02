#include <stdint.h>
#include <stdio.h>

#define CHIPS_IMPL
#include "chips/clk.h"
#include "chips/mem.h"
#include "chips/z80.h"
#include "rygar-roms.h"

typedef struct {
  z80_t cpu;
  clk_t clk;
  mem_t mem;
} mainboard_t;

typedef struct {
  mainboard_t main;
  uint8_t main_ram[0x1C00];
} rygar_t;

rygar_t rygar;

static uint64_t rygar_tick_main(int num_ticks, uint64_t pins, void* user_data) {
  return pins;
}

static void rygar_init(void) {
  memset(&rygar, 0, sizeof(rygar));

  clk_init(&rygar.main.clk, 4000000);

  // FIXME: Why can't we use the inline version?
  /* z80_init(&rygar.main.cpu, &(z80_desc_t) { .tick_cb = rygar_tick_main }); */
  z80_desc_t main_cpu = { .tick_cb = rygar_tick_main };
  z80_init(&rygar.main.cpu, &main_cpu);

  // FIXME: What is the real memory map?
  mem_init(&rygar.main.mem);
  mem_map_rom(&rygar.main.mem, 0, 0x0000, 0x2000, dump_5);
  mem_map_rom(&rygar.main.mem, 0, 0x2000, 0x2000, dump_cpu_5j);
  mem_map_rom(&rygar.main.mem, 0, 0x4000, 0x2000, dump_cpu_5m);
}

int main(int argc, const char** argv) {
  rygar_init();
  printf("Hello World!\n");
  return 0;
}
