/*
 *   __   __     __  __     __         __
 *  /\ "-.\ \   /\ \/\ \   /\ \       /\ \
 *  \ \ \-.  \  \ \ \_\ \  \ \ \____  \ \ \____
 *   \ \_\\"\_\  \ \_____\  \ \_____\  \ \_____\
 *    \/_/ \/_/   \/_____/   \/_____/   \/_____/
 *   ______     ______       __     ______     ______     ______
 *  /\  __ \   /\  == \     /\ \   /\  ___\   /\  ___\   /\__  _\
 *  \ \ \/\ \  \ \  __<    _\_\ \  \ \  __\   \ \ \____  \/_/\ \/
 *   \ \_____\  \ \_____\ /\_____\  \ \_____\  \ \_____\    \ \_\
 *    \/_____/   \/_____/ \/_____/   \/_____/   \/_____/     \/_/
 *
 * https://joshbassett.info
 *
 * Copyright (c) 2025 Joshua Bassett
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "tile.h"

#define MAX_TILE_ROWS 32
#define MAX_TILE_COLS 32

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
  uint8_t *ram;
  uint8_t *rom;

  /* dimensions */
  int tile_width;
  int tile_height;
  int rows;
  int cols;

  /* tile info callback */
  void (*tile_cb)(uint8_t *ram, tile_t *tile, int index);
} tilemap_desc_t;

/* the tilemap */
typedef struct {
  uint8_t *ram;
  uint8_t *rom;

  /* dimensions */
  int tile_width;
  int tile_height;
  int rows;
  int cols;

  /* scroll offset */
  int scroll_x;
  int scroll_y;

  /* pixel data */
  bitmap_t bitmap;

  /* tile data */
  tile_t tiles[MAX_TILE_COLS * MAX_TILE_ROWS];

  /* tile info callback */
  void (*tile_cb)(uint8_t *ram, tile_t *tile, int index);
} tilemap_t;

/*
 * Initialises a new tilemap instance.
 */
void tilemap_init(tilemap_t *tilemap, const tilemap_desc_t *desc);

/**
 * Tears down the tilemap.
 */
void tilemap_shutdown(tilemap_t *tilemap);

/**
 * Marks the given tile as dirty.
 */
void tilemap_mark_tile_dirty(tilemap_t *tilemap, const int index);

/**
 * Sets the horizontal scroll offset.
 */
void tilemap_set_scroll_x(tilemap_t *tilemap, const uint16_t value);

/**
 * Sets the vertical scroll offset.
 */
void tilemap_set_scroll_y(tilemap_t *tilemap, const uint16_t value);

/**
 * Draws the tilemap to the given bitmap.
 */
void tilemap_draw(tilemap_t *tilemap,
                  bitmap_t *bitmap,
                  uint16_t palette_offset,
                  uint8_t flags);
