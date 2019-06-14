#pragma once

#include "bitmap.h"

// Pixels drawn with this pen will be marked as transparent.
#define TRANSPARENT_PEN 0

// tile flags
#define TILE_DIRTY 0x01

typedef struct {
  uint16_t code;
  uint8_t color;
  uint8_t flags;
} tile_t;

typedef struct {
  uint8_t* rom;

  // dimensions
  int tile_width;
  int tile_height;
  int rows;
  int cols;

  // tile info callback
  void (*tile_cb)(tile_t* tile, int index);
} tilemap_desc_t;

typedef struct {
  uint8_t* rom;

  // dimensions
  int tile_width;
  int tile_height;
  int rows;
  int cols;
  int width;
  int height;

  // scroll offset
  int scroll_x;

  // pixel data
  uint16_t* buffer;

  // tile data
  tile_t* tiles;

  // tile info callback
  void (*tile_cb)(tile_t* tile, int index);
} tilemap_t;

/**
 * Draws the tile.
 */
static void tilemap_draw_tile(tilemap_t* tilemap, tile_t* tile, uint8_t col, uint8_t row, uint8_t layer) {
  uint16_t* dst = tilemap->buffer + row*tilemap->width*tilemap->tile_height + col*tilemap->tile_width;

  for (int y = 0; y < tilemap->tile_height; y++) {
    int base_addr = tile->code*tilemap->tile_width*tilemap->tile_height + y*tilemap->tile_width;
    uint16_t* ptr = dst + y*tilemap->width;

    for (int x = 0; x < tilemap->tile_width; x++) {
      uint8_t pen = tilemap->rom[base_addr + x] & 0xf;
      uint8_t pen_mask = pen == TRANSPARENT_PEN ? 0 : layer;
      *ptr++ = pen_mask<<8 | tile->color<<4 | pen;
    }
  }

  tile->flags ^= TILE_DIRTY;
}

/*
 * Initialises a new tilemap instance.
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

  tilemap->tiles = malloc(tilemap->cols * tilemap->rows * sizeof(tile_t));
  tilemap->buffer = malloc(tilemap->width * tilemap->height * sizeof(uint16_t));
}

void tilemap_shutdown(tilemap_t* tilemap) {
  free(tilemap->tiles);
  free(tilemap->buffer);
  tilemap->tiles = 0;
  tilemap->buffer = 0;
}

void tilemap_mark_tile_dirty(tilemap_t* tilemap, const int index) {
  tilemap->tiles[index].flags |= TILE_DIRTY;
}

void tilemap_set_scroll_x(tilemap_t* tilemap, const uint16_t value) {
  tilemap->scroll_x = value;
}

/**
 * Draws the tilemap to the given buffer.
 */
void tilemap_draw(tilemap_t* tilemap, bitmap_t* bitmap, uint16_t palette_offset, uint8_t layer) {
  for (int row = 0; row < tilemap->rows; row++) {
    for (int col = 0; col < tilemap->cols; col++) {
      int index = row*tilemap->cols + col;
      tile_t* tile = &tilemap->tiles[index];

      if (tile->flags & TILE_DIRTY) {
        // Get tile info.
        tilemap->tile_cb(tile, index);
        tilemap_draw_tile(tilemap, tile, col, row, layer);
      }
    }
  }

  uint16_t* data = bitmap->data;
  uint8_t* priority = bitmap->priority;

  for (int y = 0; y < bitmap->height; y++) {
    for (int x = 0; x < bitmap->width; x++) {
      // Calculate the wrapped x coordinate in tilemap space. Wrapping occurs
      // when the visible area is outside of the tilemap.
      uint32_t wrapped_x = (x + tilemap->scroll_x) % tilemap->width;
      uint32_t addr = y*tilemap->width + wrapped_x;
      uint16_t pixel = tilemap->buffer[addr];

      if (pixel & 0x0f00) {
        *data = (pixel&0xff) + palette_offset;
        *priority = pixel>>8;
      }

      data++;
      priority++;
    }
  }
}
