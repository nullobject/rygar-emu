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

typedef struct {
  /* dimensions */
  int width;
  int height;

  /* bitmap data */
  uint16_t *data;

  /* priority map */
  uint8_t *priority;
} bitmap_t;

void bitmap_init(bitmap_t *bitmap, int width, int height) {
  memset(bitmap, 0, sizeof(bitmap_t));
  bitmap->width = width;
  bitmap->height = height;
  bitmap->data = calloc(width * height, sizeof(uint16_t));
  bitmap->priority = calloc(width * height, sizeof(uint8_t));
}

void bitmap_shutdown(bitmap_t *bitmap) {
  free(bitmap->data);
  free(bitmap->priority);
  bitmap->data = 0;
  bitmap->priority = 0;
}

uint16_t *bitmap_data(bitmap_t *bitmap, int x, int y) {
  return (bitmap->data + y * bitmap->width) + x;
}

uint8_t *bitmap_priority(bitmap_t *bitmap, int x, int y) {
  return (bitmap->priority + y * bitmap->width) + x;
}

void bitmap_fill(bitmap_t *bitmap, uint16_t color) {
  uint16_t *data = bitmap->data;
  uint8_t *priority = bitmap->priority;

  for (int i = 0; i < bitmap->width * bitmap->height; i++) {
    *data++ = color;
    *priority++ = 0;
  }
}

/**
 * Copies a bitmap, respecting the priority of the pixels.
 */
void bitmap_copy(bitmap_t *src, bitmap_t *dst, int scroll_x, int scroll_y) {
  uint16_t *data = dst->data;
  uint8_t *priority = dst->priority;

  for (int y = 0; y < dst->height; y++) {
    for (int x = 0; x < dst->width; x++) {
      /* Calculate the wrapped coordinates in tilemap space. Wrapping occurs
       * when the visible area is outside of the tilemap. */
      uint32_t wrapped_x = (x + scroll_x) % src->width;
      uint32_t wrapped_y = (y + scroll_y) % src->height;
      uint32_t addr = (wrapped_y * src->width) + wrapped_x;

      if (src->priority[addr]) {
        *data = src->data[addr];
        *priority = src->priority[addr];
      }

      data++;
      priority++;
    }
  }
}
