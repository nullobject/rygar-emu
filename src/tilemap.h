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

  // scroll offset
  int scroll_x;

  // pixel data
  bitmap_t bitmap;

  // tile data
  tile_t* tiles;

  // tile info callback
  void (*tile_cb)(tile_t* tile, int index);
} tilemap_t;

/**
 * Draws the tile.
 */
static void tilemap_draw_tile(tilemap_t* tilemap, tile_t* tile, uint16_t palette_offset, int sx, int sy, uint8_t layer) {
  uint16_t* data_addr_base = tilemap->bitmap.data + sy*tilemap->bitmap.width + sx;
  uint8_t* priority_addr_base = tilemap->bitmap.priority + sy*tilemap->bitmap.width + sx;

  for (int y = 0; y < tilemap->tile_height; y++) {
    int base_addr = tile->code*tilemap->tile_width*tilemap->tile_height + y*tilemap->tile_width;
    uint16_t* data_ptr = data_addr_base + y*tilemap->bitmap.width;
    uint8_t* priority_ptr = priority_addr_base + y*tilemap->bitmap.width;

    for (int x = 0; x < tilemap->tile_width; x++) {
      uint8_t pen = tilemap->rom[base_addr + x] & 0xf;
      *priority_ptr++ = pen != TRANSPARENT_PEN ? layer : 0;
      *data_ptr++ = palette_offset | tile->color<<4 | pen;
    }
  }
}

/*
 * Initialises a new tilemap instance.
 */
void tilemap_init(tilemap_t* tilemap, const tilemap_desc_t* desc) {
  int width = desc->tile_width * desc->cols;
  int height = desc->tile_height * desc->rows;

  memset(tilemap, 0, sizeof(tilemap_t));

  tilemap->rom = desc->rom;
  tilemap->tile_width = desc->tile_width;
  tilemap->tile_height = desc->tile_height;
  tilemap->cols = desc->cols;
  tilemap->rows = desc->rows;
  tilemap->tile_cb = desc->tile_cb;

  tilemap->tiles = malloc(tilemap->cols * tilemap->rows * sizeof(tile_t));

  bitmap_init(&tilemap->bitmap, width, height);
}

void tilemap_shutdown(tilemap_t* tilemap) {
  free(tilemap->tiles);
  tilemap->tiles = 0;
  bitmap_shutdown(&tilemap->bitmap);
}

void tilemap_mark_tile_dirty(tilemap_t* tilemap, const int index) {
  tilemap->tiles[index].flags |= TILE_DIRTY;
}

void tilemap_set_scroll_x(tilemap_t* tilemap, const uint16_t value) {
  tilemap->scroll_x = value;
}

/**
 * Draws the tilemap to the given bitmap.
 */
void tilemap_draw(tilemap_t* tilemap, bitmap_t* bitmap, uint16_t palette_offset, uint8_t layer) {
  for (int row = 0; row < tilemap->rows; row++) {
    for (int col = 0; col < tilemap->cols; col++) {
      int index = row*tilemap->cols + col;
      tile_t* tile = &tilemap->tiles[index];

      if (tile->flags & TILE_DIRTY) {
        tilemap->tile_cb(tile, index);
        int sx = col*tilemap->tile_width;
        int sy = row*tilemap->tile_height;
        tilemap_draw_tile(tilemap, tile, palette_offset, sx, sy, layer);
        tile->flags ^= TILE_DIRTY;
      }
    }
  }

  bitmap_copy(&tilemap->bitmap, bitmap, tilemap->scroll_x);
}
