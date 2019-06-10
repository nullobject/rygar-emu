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

// Pixels drawn with this pen will be marked as transparent.
#define TRANSPARENT_PEN 0

// The size of the pixel buffer.
#define BUFFER_SIZE (DISPLAY_WIDTH*DISPLAY_HEIGHT*2)

#define MAX_TILES 1024

#define TILE_DIRTY 0x01

typedef struct {
  uint16_t code;
  uint8_t color;
  uint8_t flags;
} tile_t;

typedef struct {
  uint8_t* rom;
  uint8_t tile_width;
  uint8_t tile_height;
  uint8_t rows;
  uint8_t cols;
  void (*tile_cb)(tile_t* tile, uint16_t index);
} tilemap_desc_t;

typedef struct {
  uint8_t* rom;
  uint8_t tile_width;
  uint8_t tile_height;
  uint8_t rows;
  uint8_t cols;
  uint16_t width;
  uint16_t height;
  uint16_t scroll_x;
  uint16_t buffer[BUFFER_SIZE];
  tile_t tiles[MAX_TILES];
  void (*tile_cb)(tile_t* tile, uint16_t index);
} tilemap_t;

static inline void tilemap_draw_pixel(uint16_t* dst, uint8_t data, uint8_t color) {
  // The low and high nibbles represent the bitplane values for each pixel.
  uint8_t hi = data>>4 & 0xf;
  uint8_t lo = data & 0xf;

  *dst++ = hi != TRANSPARENT_PEN ? color<<4 | hi : 0xf000;
  *dst++ = lo != TRANSPARENT_PEN ? color<<4 | lo : 0xf000;
}

static void tilemap_draw_8x8_tile(tilemap_t* tilemap, tile_t* tile, uint8_t col, uint8_t row) {
  uint16_t* dst = tilemap->buffer + row*tilemap->width*8 + col*8;

  for (int y = 0; y < 8; y++) {
    int base_addr = tile->code*TILE_SIZE + y*NUM_BITPLANES;
    uint16_t* ptr = dst + y*tilemap->width;

    for (int x = 0; x < 4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = tilemap->rom[base_addr + x];
      tilemap_draw_pixel(ptr, data, tile->color);
      ptr+=2;
    }
  }
}

static void tilemap_draw_16x16_tile(tilemap_t* tilemap, tile_t* tile, uint8_t col, uint8_t row) {
  uint16_t* dst = tilemap->buffer + row*tilemap->width*tilemap->tile_height + col*tilemap->tile_width;

  for (int y = 0; y < tilemap->tile_height/2; y++) {
    int base_addr = tile->code*TILE_SIZE*4 + y*NUM_BITPLANES;
    uint16_t* ptr = dst + y*tilemap->width;

    for (int x = 0; x < tilemap->tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = tilemap->rom[base_addr + x];
      tilemap_draw_pixel(ptr, data, tile->color);
      ptr+=2;
    }

    for (int x = 0; x < tilemap->tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = tilemap->rom[base_addr + x + TILE_SIZE];
      tilemap_draw_pixel(ptr, data, tile->color);
      ptr+=2;
    }
  }

  for (int y = 0; y < tilemap->tile_height/2; y++) {
    int base_addr = tile->code*TILE_SIZE*4 + y*NUM_BITPLANES + TILE_SIZE*2;
    uint16_t* ptr = dst + (y+8)*tilemap->width;

    for (int x = 0; x < tilemap->tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = tilemap->rom[base_addr + x];
      tilemap_draw_pixel(ptr, data, tile->color);
      ptr+=2;
    }

    for (int x = 0; x < tilemap->tile_width/4; x++) {
      // Each byte in the char ROM contains the bitplane values for two pixels.
      uint8_t data = tilemap->rom[base_addr + x + TILE_SIZE];
      tilemap_draw_pixel(ptr, data, tile->color);
      ptr+=2;
    }
  }

  tile->flags ^= TILE_DIRTY;
}

/*
 * Initialise a new tilemap instance.
 */
void tilemap_init(tilemap_t* tilemap, const tilemap_desc_t* desc) {
  memset(tilemap, 0, sizeof(tilemap_t));

  tilemap->rom = desc->rom;
  tilemap->tile_width = desc->tile_width;
  tilemap->tile_height = desc->tile_height;
  tilemap->cols = desc->cols;
  tilemap->rows = desc->rows;
  tilemap->width = desc->tile_width * desc->cols;
  tilemap->height = desc->tile_height * desc->rows;
  tilemap->tile_cb = desc->tile_cb;
}

void tilemap_mark_tile_dirty(tilemap_t* tilemap, const uint16_t index) {
  tilemap->tiles[index].flags |= TILE_DIRTY;
}

void tilemap_set_scroll_x(tilemap_t* tilemap, const uint16_t value) {
  tilemap->scroll_x = value;
}

void tilemap_draw(tilemap_t* tilemap, uint32_t* dst, uint32_t* palette) {
  for (int row = 0; row < tilemap->rows; row++) {
    for (int col = 0; col < tilemap->cols; col++) {
      uint16_t index = row*tilemap->cols + col;
      tile_t* tile = &tilemap->tiles[index];

      if (tile->flags & TILE_DIRTY) {
        // Update tile info.
        tilemap->tile_cb(tile, index);

        // XXX: Dumb hack to choose between 16x16 and 8x8 tiles.
        if (tilemap->tile_width == 16) {
          tilemap_draw_16x16_tile(tilemap, tile, col, row);
        } else {
          tilemap_draw_8x8_tile(tilemap, tile, col, row);
        }
      }
    }
  }

  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      // Calculate the wrapped x coordinate in tilemap space. Wrapping occurs
      // when the visible area is outside of the tilemap.
      uint32_t wrapped_x = (x + tilemap->scroll_x) & (tilemap->width - 1);
      uint32_t addr = y*tilemap->width + wrapped_x;
      uint16_t pixel = tilemap->buffer[addr];

      if (!(pixel & 0xf000)) { *dst = palette[pixel]; }

      dst++;
    }
  }
}
