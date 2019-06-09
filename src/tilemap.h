#pragma once

// The number of bitplanes that define the pixel data for the 8x8 tiles.
#define NUM_BITPLANES 4

// Tile width/height in pixels.
#define TILE_WIDTH 8
#define TILE_HEIGHT 8

// The tile size in bytes.
//
// A single 8x8 tile takes up one byte per bitplane, per row.
#define TILE_SIZE (TILE_HEIGHT*NUM_BITPLANES)

// Display width/height in pixels.
#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 256

static inline void draw_pixel(
  uint32_t* dst,
  uint32_t* palette,
  uint16_t palette_offset,
  uint32_t data,
  uint8_t color
) {
  // The low and high nibbles represent the bitplane values for each pixel.
  uint8_t hi = (data >> 4) & 0xf;
  uint8_t lo = data & 0xf;

  if (hi != 0) { *dst = palette[palette_offset | (color << 4) | hi]; }
  dst++;
  if (lo != 0) { *dst = palette[palette_offset | (color << 4) | lo]; }
  dst++;
}

static void draw_8x8_tile(
  uint32_t* dst,
  uint32_t* palette,
  uint16_t palette_offset,
  uint8_t* rom,
  uint16_t index,
  uint8_t color
) {
  for (int y = 0; y < TILE_HEIGHT; y++) {
    int addr = index*TILE_SIZE + y*NUM_BITPLANES;
    uint32_t* ptr = dst + y*DISPLAY_WIDTH;

    for (int x = 0; x < TILE_WIDTH/2; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x];
      draw_pixel(ptr, palette, palette_offset, data, color);
      ptr+=2;
    }
  }
}

static void draw_16x16_tile(
  uint32_t* dst,
  uint32_t* palette,
  uint16_t palette_offset,
  uint8_t* rom,
  uint16_t index,
  uint8_t color
) {
  for (int y = 0; y < TILE_HEIGHT; y++) {
    int addr = index*TILE_SIZE*4 + y*NUM_BITPLANES;
    uint32_t* ptr = dst + y*DISPLAY_WIDTH;

    for (int x = 0; x < TILE_WIDTH/2; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x];
      draw_pixel(ptr, palette, palette_offset, data, color);
      ptr+=2;
    }

    for (int x = 0; x < TILE_WIDTH/2; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x + TILE_SIZE];
      draw_pixel(ptr, palette, palette_offset, data, color);
      ptr+=2;
    }
  }

  for (int y = 0; y < TILE_HEIGHT; y++) {
    int addr = index*TILE_SIZE*4 + y*NUM_BITPLANES + TILE_SIZE*2;
    uint32_t* ptr = dst + (y+8)*DISPLAY_WIDTH;

    for (int x = 0; x < TILE_WIDTH/2; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x];
      draw_pixel(ptr, palette, palette_offset, data, color);
      ptr+=2;
    }

    for (int x = 0; x < TILE_WIDTH/2; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x + TILE_SIZE];
      draw_pixel(ptr, palette, palette_offset, data, color);
      ptr+=2;
    }
  }
}

/**
 * Draws a 16x16 tilemap.
 *
 * The tilemap is made up of 16x16 pixel tiles.
 */
void draw_16x16_tilemap(
  uint32_t* dst,
  uint32_t* palette,
  uint16_t palette_offset,
  uint8_t* rom,
  uint8_t* ram
) {
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      uint32_t* ptr = dst + y*DISPLAY_WIDTH*16 + x*16;

      int addr = y * TILE_SIZE + x;
      uint8_t lo = ram[addr];
      uint8_t hi = ram[addr + 0x200];

      // The tile index is a 10-bit value, represented by the low byte and the
      // three LSBs of the high byte.
      uint16_t index = ((hi & 0x07) << 8) | lo;

      // The four MSBs of the high byte represent the color value.
      uint8_t color = hi>>4;

      draw_16x16_tile(ptr, palette, palette_offset, rom, index, color);
    }
  }
}

/**
 * Draws a 32x32 tilemap.
 *
 * The tilemap is made up of 8x8 pixel tiles.
 */
void draw_32x32_tilemap(
  uint32_t* dst,
  uint32_t* palette,
  uint16_t palette_offset,
  uint8_t* rom,
  uint8_t* ram
) {
  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 32; x++) {
      uint32_t* ptr = dst + y*DISPLAY_WIDTH*8 + x*8;

      int addr = y * TILE_SIZE + x;
      uint8_t lo = ram[addr];
      uint8_t hi = ram[addr + 0x400];

      // The tile index is a 10-bit value, represented by the low byte and the
      // two LSBs of the high byte.
      uint16_t index = ((hi & 0x03) << 8) | lo;

      // The four MSBs of the high byte represent the color value.
      uint8_t color = hi>>4;

      draw_8x8_tile(ptr, palette, palette_offset, rom, index, color);
    }
  }
}
