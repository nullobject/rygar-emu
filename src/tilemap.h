#pragma once

#include "bitmap.h"
#include "tile.h"

/* tile flags */
#define TILEMAP_TILE_DIRTY 0x01

/* a single tile in the tilemap */
typedef struct {
  uint16_t code;
  uint8_t color;
  uint8_t flags;
} tile_t;

/* descriptor for initialising a tilemap */
typedef struct {
  uint8_t *rom;

  /* dimensions */
  int tile_width;
  int tile_height;
  int rows;
  int cols;

  /* tile info callback */
  void (*tile_cb)(tile_t *tile, int index);
} tilemap_desc_t;

/* the tilemap */
typedef struct {
  uint8_t *rom;

  /* dimensions */
  int tile_width;
  int tile_height;
  int rows;
  int cols;

  /* scroll offset */
  int scroll_x;

  /* pixel data */
  bitmap_t bitmap;

  /* tile data */
  tile_t *tiles;

  /* tile info callback */
  void (*tile_cb)(tile_t *tile, int index);
} tilemap_t;

/*
 * Initialises a new tilemap instance.
 */
void tilemap_init(tilemap_t *tilemap, const tilemap_desc_t *desc) {
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

/**
 * Tears down the tilemap.
 */
void tilemap_shutdown(tilemap_t *tilemap) {
  free(tilemap->tiles);
  tilemap->tiles = 0;
  bitmap_shutdown(&tilemap->bitmap);
}

/**
 * Marks the given tile as dirty.
 */
void tilemap_mark_tile_dirty(tilemap_t *tilemap, const int index) {
  tilemap->tiles[index].flags |= TILEMAP_TILE_DIRTY;
}

/**
 * Sets the horizontal scroll offset.
 */
void tilemap_set_scroll_x(tilemap_t *tilemap, const uint16_t value) {
  tilemap->scroll_x = value;
}

/**
 * Draws the tilemap to the given bitmap.
 */
void tilemap_draw(tilemap_t *tilemap, bitmap_t *bitmap, uint16_t palette_offset, uint8_t flags) {
  /* force opaque drawing, otherwise old pixels in the buffer will be visible
   * through any transparent parts of the tile */
  flags |= TILE_OPAQUE;

  for (int row = 0; row < tilemap->rows; row++) {
    for (int col = 0; col < tilemap->cols; col++) {
      int index = (row * tilemap->cols) + col;
      tile_t *tile = &tilemap->tiles[index];

      if (tile->flags & TILEMAP_TILE_DIRTY) {
        tilemap->tile_cb(tile, index);

        int x = col * tilemap->tile_width;
        int y = row * tilemap->tile_height;

        tile_draw(
          &tilemap->bitmap,
          tilemap->rom,
          tile->code,
          tile->color,
          palette_offset,
          x, y,
          tilemap->tile_width, tilemap->tile_height,
          false, false,
          0, /* don't bother masking, as we're only rendering to the internal buffer */
          flags
        );

        tile->flags ^= TILEMAP_TILE_DIRTY;
      }
    }
  }

  /* copy the internal buffer to the output bitmap */
  bitmap_copy(&tilemap->bitmap, bitmap, tilemap->scroll_x);
}
