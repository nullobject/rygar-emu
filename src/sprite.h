#include "mem.h"

// The number of bitplanes that define the pixel data for the 8x8 tiles.
#define NUM_BITPLANES 4

// A single 8x8 tile takes up one byte per bitplane, per row.
#define TILE_SIZE (8*NUM_BITPLANES)

// Display width/height in pixels.
#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 256

#define NUM_SPRITES 256

// sprite size in bytes
#define SPRITE_SIZE 8

#define SPRITE_RAM_SIZE 0x800

// tile offsets
static const uint8_t layout[8][8] = {
  { 0, 1, 4, 5, 16, 17, 20, 21 },
  { 2, 3, 6, 7, 18, 19, 22, 23 },
  { 8, 9, 12, 13, 24, 25, 28, 29 },
  { 10, 11, 14, 15, 26, 27, 30, 31 },
  { 32, 33, 36, 37, 48, 49, 52, 53 },
  { 34, 35, 38, 39, 50, 51, 54, 55 },
  { 40, 41, 44, 45, 56, 57, 60, 61 },
  { 42, 43, 46, 47, 58, 59, 62, 63 }
};

static void sprite_draw_8x8_tile(uint32_t* buffer, uint32_t* palette, mem_t* rom, uint16_t code, uint8_t color, uint8_t flipx, uint8_t flipy, uint16_t sx, uint16_t sy) {
  uint32_t* dst = buffer + sy*DISPLAY_WIDTH + sx;

  for (int y = 0; y < 8; y++) {
    int base_addr = code*TILE_SIZE + y*NUM_BITPLANES;
    uint32_t* ptr = dst + y*DISPLAY_WIDTH;

    if (!flipx) {
      for (int x = 0; (x < 4) && (sx + x < DISPLAY_WIDTH); x++) {
        uint8_t data = mem_rd(rom, base_addr + x);
        uint8_t hi = data>>4 & 0xf;
        uint8_t lo = data & 0xf;
        if (hi) { *ptr = palette[color<<4|hi]; }
        *ptr++;
        if (lo) { *ptr = palette[color<<4|lo]; }
        *ptr++;
      }
    } else {
      for (int x = 3; (x >= 0) && (x + sx < DISPLAY_WIDTH); x--) {
        uint8_t data = mem_rd(rom, base_addr + x);
        uint8_t hi = data>>4 & 0xf;
        uint8_t lo = data & 0xf;
        if (lo) { *ptr = palette[color<<4|lo]; }
        *ptr++;
        if (hi) { *ptr = palette[color<<4|hi]; }
        *ptr++;
      }
    }
  }
}

/**
 * Draws the sprites to the given buffer.
 *
 * The sprites are stored in the following format:
 *
 *  byte     bit        description
 * --------+-76543210-+----------------
 *       0 | xxxx---- | bank
 *         | -----x-- | visible
 *         | ------x- | flip y
 *         | -------x | flip x
 *       1 | xxxxxxxx | tile code
 *       2 | ------xx | size
 *       3 | xx-------| priority
 *         | --x----- | upper y co-ord
 *         | ---x---- | upper x co-ord
 *         | ----xxxx | colour
 *       4 | xxxxxxxx | ypos
 *       5 | xxxxxxxx | xpos
 *       6 | -------- |
 *       7 | -------- |
 */
void sprite_draw(uint32_t* dst, uint32_t* palette, uint8_t* ram, mem_t* rom) {
  for (int addr = SPRITE_RAM_SIZE - SPRITE_SIZE; addr >= 0; addr -= SPRITE_SIZE) {
    bool visible = ram[addr] & 0x04;

		if (visible) {
      uint8_t bank = ram[addr];
      uint16_t code = ram[addr+1];
      uint8_t size = ram[addr+2] & 0x03;

      code |= (bank & 0xf0) << 4;

      // Ensure the lower bits are masked out for the diffrent sizes (8x8,
      // 16x16, 32x32, 64x64). This is required because we add a tile code
      // offset for each tile in the different sizes.
      code &= ~((1 << (size*2)) - 1);

      size = 1 << size;

      uint8_t flags = ram[addr+3];
			uint16_t xpos = ram[addr+5] - ((flags & 0x10) << 0x04);
			uint16_t ypos = ram[addr+4] - ((flags & 0x20) << 0x03);

			uint8_t flipx = bank & 0x01;
			uint8_t flipy = bank & 0x02;
      uint8_t color = flags & 0x0f;

      for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
          int sx = xpos + 8*(flipx?(size-1-x):x);
          int sy = ypos + 8*(flipy?(size-1-y):y);
          sprite_draw_8x8_tile(dst, palette, rom, code + layout[y][x], color, flipx, flipy, sx, sy);
        }
      }
    }
  }
}
