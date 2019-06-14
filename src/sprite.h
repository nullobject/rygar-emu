#pragma once

#include "bitmap.h"
#include "tile.h"

/* sprite size (in bytes) */
#define SPRITE_SIZE 8

/* sprite tile dimensions */
#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define SPRITE_RAM_SIZE 0x800

/* There are four possible sprite sizes: 8x8, 16x16, 32x32, and 64x64. All
 * sprites are composed of a number of 8x8 tiles. This lookup table allows us
 * to easily find the offsets of the tiles which make up a sprite.
 *
 * For example, a 8x8 sprite contains only a single tile with an offset value
 * of zero. A 16x16 sprite contains four tiles, with offset values 0, 1, 2, and
 * 3. */
static const uint8_t sprite_tile_offset_table[TILE_HEIGHT][TILE_WIDTH] = {
  {  0,  1,  4,  5, 16, 17, 20, 21 },
  {  2,  3,  6,  7, 18, 19, 22, 23 },
  {  8,  9, 12, 13, 24, 25, 28, 29 },
  { 10, 11, 14, 15, 26, 27, 30, 31 },
  { 32, 33, 36, 37, 48, 49, 52, 53 },
  { 34, 35, 38, 39, 50, 51, 54, 55 },
  { 40, 41, 44, 45, 56, 57, 60, 61 },
  { 42, 43, 46, 47, 58, 59, 62, 63 }
};

/**
 * Draws the sprites to the given bitmap.
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
void sprite_draw(bitmap_t *bitmap, uint8_t *ram, uint8_t *rom, uint16_t palette_offset, uint8_t flags) {
  /* sprites are sorted from highest to lowest priority, so we need to iterate
   * backwards to ensure that the sprites with the highest priority are drawn
   * last */
  for (int addr = SPRITE_RAM_SIZE - SPRITE_SIZE; addr >= 0; addr -= SPRITE_SIZE) {
    bool visible = ram[addr] & 0x04;

		if (visible) {
      uint8_t bank = ram[addr];
      uint16_t code = ram[addr + 1] | (bank & 0xf0)<<4;
      int size = ram[addr + 2] & 0x03;

      /* Ensure the lower sprite code bits are masked. This is required because
       * we add the tile code offset from the lookup table for the different
       * sprite sizes. */
      code &= ~((1 << (size * 2)) - 1);

      /* the size is the number of 8x8 tiles (8x8, 16x16, 32x32, 64x64) */
      size = 1 << size;

      uint8_t b3 = ram[addr + 3];
			int xpos = ram[addr + 5] - ((b3 & 0x10) << 4);
			int ypos = ram[addr + 4] - ((b3 & 0x20) << 3);

			bool flip_x = bank & 0x01;
			bool flip_y = bank & 0x02;
      uint8_t color = b3 & 0x0f;
      uint8_t priority_mask;

			switch (b3 >> 6) {
				default:
				case 0x0: priority_mask = TILE_LAYER0; break; /* obscured by other sprites */
				case 0x1: priority_mask = TILE_LAYER0 | TILE_LAYER1; break; /* obscured by text layer */
				case 0x2: priority_mask = TILE_LAYER0 | TILE_LAYER1 | TILE_LAYER2; break; /* obscured by foreground */
				case 0x3: priority_mask = TILE_LAYER0 | TILE_LAYER1 | TILE_LAYER2 | TILE_LAYER3; break; /* obscured by background */
			}

      for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
          int x = xpos + TILE_WIDTH * (flip_x ? (size - 1 - col) : col);
          int y = ypos + TILE_HEIGHT * (flip_y ? (size - 1 - row) : row);

          tile_draw(
            bitmap,
            rom,
            code + sprite_tile_offset_table[row][col],
            color,
            palette_offset,
            x, y,
            TILE_WIDTH, TILE_HEIGHT,
            flip_x, flip_y,
            priority_mask,
            flags
          );
        }
      }
    }
  }
}
