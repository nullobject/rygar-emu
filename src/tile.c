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

#include "tile.h"

/**
 * Reads a single bit value from the tile ROM at the given offset.
 *
 * The offset value is specified in bits, and can span across multiple bytes.
 */
static inline int read_bit(const uint8_t *rom, int offset) {
  return rom[offset / 8] & (0x80 >> (offset % 8));
}

/**
 * Draws a single pixel.
 */
static inline void tile_draw_pixel(uint16_t *data, uint8_t *priority,
                                   uint8_t priority_mask,
                                   uint16_t palette_offset, uint8_t color,
                                   uint8_t pen, uint8_t flags) {
  /* bail out if we're using the transparent pen */
  if (pen == TRANSPARENT_PEN && !(flags & TILE_OPAQUE))
    return;

  /* bail out if there's already a pixel with higher priority */
  if ((*priority & priority_mask) != 0)
    return;

  *data = palette_offset | color << 4 | pen;
  *priority = (pen != TRANSPARENT_PEN) ? flags & TILE_LAYER_MASK : 0;
}

void tile_decode(const tile_decode_desc_t *desc, uint8_t *rom, uint8_t *dst,
                 int count) {
  for (int tile = 0; tile < count; tile++) {
    uint8_t *ptr = dst + (tile * desc->tile_width * desc->tile_height);

    /* clear the bytes for the next tile */
    memset(ptr, 0, desc->tile_width * desc->tile_height);

    for (int plane = 0; plane < desc->planes; plane++) {
      int plane_bit = 1 << (desc->planes - 1 - plane);
      int plane_offset =
          (tile * desc->tile_size * 8) + desc->plane_offsets[plane];

      for (int y = 0; y < desc->tile_height; y++) {
        int y_offset = plane_offset + desc->y_offsets[y];
        ptr = dst + (tile * desc->tile_width * desc->tile_height) +
              (y * desc->tile_width);

        for (int x = 0; x < desc->tile_width; x++) {
          if (read_bit(rom, y_offset + desc->x_offsets[x])) {
            ptr[x] |= plane_bit;
          }
        }
      }
    }
  }
}

void tile_draw(bitmap_t *bitmap, uint8_t *rom, uint16_t code, uint8_t color,
               uint16_t palette_offset, int x, int y, int width, int height,
               bool flip_x, bool flip_y, uint8_t priority_mask, uint8_t flags) {
  /* bail out if the tile is completely off-screen */
  if (x < 0 - width - 1 || y < 0 - height - 1 || x >= bitmap->width ||
      y >= bitmap->height)
    return;

  uint16_t *data = bitmap_data(bitmap, x, y);
  uint8_t *priority = bitmap_priority(bitmap, x, y);
  uint8_t *tile = rom + (code * width * height);

  int flip_mask_x = flip_x ? (width - 1) : 0;
  int flip_mask_y = flip_y ? (height - 1) : 0;

  for (int v = 0; v < height; v++) {
    /* ensure we're inside the bitmap */
    if (y + v < 0 || y + v >= bitmap->height)
      continue;

    for (int u = 0; u < width; u++) {
      /* ensure we're inside the bitmap */
      if (x + u < 0 || x + u >= bitmap->width)
        continue;

      int offset = (v * bitmap->width) + u;
      uint8_t pen = tile[(v ^ flip_mask_y) * width + (u ^ flip_mask_x)] & 0xf;

      tile_draw_pixel(data + offset, priority + offset, priority_mask,
                      palette_offset, color, pen, flags);
    }
  }
}
