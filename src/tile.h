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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bitmap.h"

/* step macros */
#define STEP2(start, step) (start), (start) + (step)
#define STEP4(start, step) STEP2(start, step), STEP2((start) + 2 * (step), step)
#define STEP8(start, step) STEP4(start, step), STEP4((start) + 4 * (step), step)
#define STEP16(start, step)                                                    \
  STEP8(start, step), STEP8((start) + 8 * (step), step)
#define STEP32(start, step)                                                    \
  STEP16(start, step), STEP16((start) + 16 * (step), step)
#define STEP64(start, step)                                                    \
  STEP32(start, step), STEP32((start) + 32 * (step), step)

/* flags */
#define TILE_LAYER0 0x01
#define TILE_LAYER1 0x02
#define TILE_LAYER2 0x04
#define TILE_LAYER3 0x08
#define TILE_OPAQUE 0x80

/* masks the layer value from the flags */
#define TILE_LAYER_MASK 0x0f

/* pen zero is marked as transparent */
#define TRANSPARENT_PEN 0

/* parameters for decoding pixels from the tile ROMs */
typedef struct {
  /* tile dimensions */
  int tile_width;
  int tile_height;

  /* number of bit planes */
  int planes;

  /* offset arrays */
  int plane_offsets[8];
  int x_offsets[32];
  int y_offsets[32];

  /* tile size (in bytes) */
  int tile_size;
} tile_decode_desc_t;

/**
 * Decodes the given tile ROM to 8-bit pixel data.
 *
 * The decoded data takes up more space that the original tile ROM, but the
 * advantage is that you don't have to jump around to get the pixel data. You
 * can just iterate through the pixels sequentially, as each pixel is
 * represented by only one byte.
 */
void tile_decode(const tile_decode_desc_t *desc,
                 uint8_t *rom,
                 uint8_t *dst,
                 int count);

/**
 * Draws the given tile to a bitmap.
 */
void tile_draw(bitmap_t *bitmap,
               uint8_t *rom,
               uint16_t code,
               uint8_t color,
               uint16_t palette_offset,
               int x,
               int y,
               int width,
               int height,
               bool flip_x,
               bool flip_y,
               uint8_t priority_mask,
               uint8_t flags);
