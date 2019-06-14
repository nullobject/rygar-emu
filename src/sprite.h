#pragma once

#include "bitmap.h"
#include "tile.h"

// Pixels drawn with this pen will be marked as transparent.
#define TRANSPARENT_PEN 0

#define NUM_SPRITES 256

// sprite size in bytes
#define SPRITE_SIZE 8

#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define SPRITE_RAM_SIZE 0x800

// There are four possible sprite sizes: 8x8, 16x16, 32x32, and 64x64. All
// sprites are composed of a number of 8x8 tiles. This lookup table allows us
// to easily find the offsets of the tiles which make up a sprite.
//
// We store the offset values of the tiles for the different sprite sizes. For
// example, a 8x8 sprite contains only a single tile with an offset value of
// zero. A 16x16 sprite contains four tiles, with offset values 0, 1, 2, and 3.
static const uint8_t sprite_tile_offset_table[8][8] = {
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
void sprite_draw(bitmap_t* bitmap, uint8_t* ram, uint8_t* rom, uint8_t flags) {
  for (int addr = SPRITE_RAM_SIZE - SPRITE_SIZE; addr >= 0; addr -= SPRITE_SIZE) {
    bool visible = ram[addr] & 0x04;

		if (visible) {
      uint8_t bank = ram[addr];
      uint16_t code = ram[addr+1] | (bank & 0xf0)<<4;
      int size = ram[addr+2] & 0x03;

      // Ensure the lower sprite code bits are masked. This is required because
      // we add the tile code offset from the layout lookup table for the
      // different sprite sizes.
      code &= ~((1 << (size*2)) - 1);

      // The size is specified in 8x8 tiles (8x8, 16x16, 32x32, 64x64).
      size = 1 << size;

      uint8_t b3 = ram[addr+3];
			int xpos = ram[addr+5] - ((b3 & 0x10) << 4);
			int ypos = ram[addr+4] - ((b3 & 0x20) << 3);

			bool flipx = bank & 0x01;
			bool flipy = bank & 0x02;
      uint8_t color = b3 & 0x0f;
      uint8_t priority_mask;

			switch (b3>>6) {
				default:
				case 0x0: priority_mask = 0; break;
				case 0x1: priority_mask = 0x1; break; // obscured by text layer
				case 0x2: priority_mask = 0x1|0x2; break; // obscured by foreground
				case 0x3: priority_mask = 0x1|0x2|0x4; break; // obscured by foreground and background
			}

      for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
          int sx = xpos + 8*(flipx?(size-1-x):x);
          int sy = ypos + 8*(flipy?(size-1-y):y);

          tile_draw(
            bitmap,
            rom,
            code + sprite_tile_offset_table[y][x],
            color,
            0, // palette offset
            flipx, flipy,
            sx, sy,
            TILE_WIDTH, TILE_HEIGHT,
            priority_mask,
            flags
          );
        }
      }
    }
  }
}
