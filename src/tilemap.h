#pragma once

// The number of bitplanes that define the pixel data for the 8x8 tiles.
#define NUM_BITPLANES 4

// The tile size in bytes.
//
// A single 8x8 tile takes up one byte per bitplane, per row.
#define TILE_SIZE (8*NUM_BITPLANES)

// Display width/height in pixels.
#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 256

#define TRANSPARENT_PEN 0

static inline void draw_pixel(
  uint32_t* dst,
  uint32_t* palette,
  uint32_t data,
  uint8_t color
) {
  // The low and high nibbles represent the bitplane values for each pixel.
  uint8_t hi = (data >> 4) & 0xf;
  uint8_t lo = data & 0xf;

  if (hi != TRANSPARENT_PEN) { *dst = palette[(color << 4) | hi]; }
  dst++;
  if (lo != TRANSPARENT_PEN) { *dst = palette[(color << 4) | lo]; }
  dst++;
}

static void draw_8x8_tile(
  uint32_t* dst,
  uint32_t* palette,
  uint8_t* rom,
  uint8_t tile_width,      // tile width in pixels
  uint8_t tile_height,     // tile height in pixels
  uint16_t tile_index,     // index of the tile in ROM
  uint8_t color
) {
  for (int y = 0; y < tile_height; y++) {
    int addr = tile_index*TILE_SIZE + y*NUM_BITPLANES;
    uint32_t* ptr = dst + y*DISPLAY_WIDTH;

    for (int x = 0; x < tile_width/2; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x];
      draw_pixel(ptr, palette, data, color);
      ptr+=2;
    }
  }
}

static void draw_16x16_tile(
  uint32_t* dst,
  uint32_t* palette,
  uint8_t* rom,
  uint8_t tile_width,      // tile width in pixels
  uint8_t tile_height,     // tile height in pixels
  uint16_t tile_index,     // index of the tile in ROM
  uint8_t color
) {
  for (int y = 0; y < tile_height/2; y++) {
    int addr = tile_index*TILE_SIZE*4 + y*NUM_BITPLANES;
    uint32_t* ptr = dst + y*DISPLAY_WIDTH;

    for (int x = 0; x < tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x];
      draw_pixel(ptr, palette, data, color);
      ptr+=2;
    }

    for (int x = 0; x < tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x + TILE_SIZE];
      draw_pixel(ptr, palette, data, color);
      ptr+=2;
    }
  }

  for (int y = 0; y < tile_height/2; y++) {
    int addr = tile_index*TILE_SIZE*4 + y*NUM_BITPLANES + TILE_SIZE*2;
    uint32_t* ptr = dst + (y+8)*DISPLAY_WIDTH;

    for (int x = 0; x < tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x];
      draw_pixel(ptr, palette, data, color);
      ptr+=2;
    }

    for (int x = 0; x < tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = rom[addr + x + TILE_SIZE];
      draw_pixel(ptr, palette, data, color);
      ptr+=2;
    }
  }
}

static void draw_16x16_tilemap_row(
  uint32_t* dst,
  uint32_t* palette,
  uint8_t* rom,
  uint8_t* ram,
  uint8_t tile_width,      // tile width in pixels
  uint8_t tile_height,     // tile height in pixels
  uint8_t from,
  uint8_t to
) {
  for (int x = from; x <= to; x++) {
    uint32_t* ptr = dst + x*tile_width;

    uint8_t lo = ram[x];
    uint8_t hi = ram[x + 0x200];

    // The tile index is a 10-bit value, represented by the low byte and the
    // three LSBs of the high byte.
    uint16_t tile_index = ((hi & 0x07) << 8) | lo;

    // The four MSBs of the high byte represent the color value.
    uint8_t color = hi>>4;

    draw_16x16_tile(ptr, palette, rom, tile_width, tile_height, tile_index, color);
  }
}

/**
 * Draws a 16x16 scrolling tilemap.
 *
 * The tilemap is made up of 16x16 pixel tiles.
 */
void draw_16x16_tilemap(
  uint32_t* dst,
  uint32_t* palette,
  uint8_t* rom,
  uint8_t* ram,
  uint8_t tile_width,      // tile width in pixels
  uint8_t tile_height,     // tile height in pixels
  uint8_t cols,            // number of cols in the tilemap
  uint8_t rows,            // number of rows in the tilemap
  uint16_t scroll_offset   // scroll offset of the tilemap
) {
  uint32_t* ptr;
  uint16_t tilemap_width = tile_width * cols;
  uint16_t first_col = (scroll_offset % tilemap_width) >> 4;
  uint16_t last_col = ((scroll_offset + DISPLAY_WIDTH - tile_width) % tilemap_width) >> 4;

  /* printf("%i %i %i\n", scroll_offset % 512, first_col, last_col); */

  for (int y = 0; y < rows; y++) {
    if (first_col <= last_col) {
      ptr = dst + y*DISPLAY_WIDTH*tile_height - first_col*tile_width;
      draw_16x16_tilemap_row(ptr, palette, rom, ram + y*TILE_SIZE, tile_width, tile_height, first_col, last_col);
    } else {
      ptr = dst + y*DISPLAY_WIDTH*tile_height - first_col*tile_width;
      draw_16x16_tilemap_row(ptr, palette, rom, ram + y*TILE_SIZE, tile_width, tile_height, first_col, cols - 1);

      ptr = dst + y*DISPLAY_WIDTH*tile_height + (cols - first_col)*tile_width;
      draw_16x16_tilemap_row(ptr, palette, rom, ram + y*TILE_SIZE, tile_width, tile_height, 0, last_col);
    }
  }
}

/**
 * Draws a 32x32 static tilemap.
 *
 * The tilemap is made up of 8x8 pixel tiles.
 */
void draw_32x32_tilemap(
  uint32_t* dst,
  uint32_t* palette,
  uint8_t* rom,
  uint8_t* ram,
  uint8_t tile_width,      // tile width in pixels
  uint8_t tile_height,     // tile height in pixels
  uint8_t cols,            // number of cols in the tilemap
  uint8_t rows,            // number of rows in the tilemap
  uint16_t scroll_offset   // scroll offset of the tilemap
) {
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      uint32_t* ptr = dst + y*DISPLAY_WIDTH*tile_height + x*tile_width;

      int addr = y*TILE_SIZE + x;
      uint8_t lo = ram[addr];
      uint8_t hi = ram[addr + 0x400];

      // The tile index is a 10-bit value, represented by the low byte and the
      // two LSBs of the high byte.
      uint16_t tile_index = ((hi & 0x03) << 8) | lo;

      // The four MSBs of the high byte represent the color value.
      uint8_t color = hi>>4;

      draw_8x8_tile(ptr, palette, rom, tile_width, tile_height, tile_index, color);
    }
  }
}
