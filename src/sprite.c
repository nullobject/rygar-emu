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

#include "sprite.h"

#include <stdbool.h>

void sprite_draw(bitmap_t *bitmap, uint8_t *ram, uint8_t *rom,
                 uint16_t palette_offset, uint8_t flags) {
  /* Sprites are sorted from highest to lowest priority, so we need to iterate
   * backwards to ensure that the sprites with the highest priority are drawn
   * last */
  for (int addr = SPRITE_RAM_SIZE - SPRITE_SIZE; addr >= 0;
       addr -= SPRITE_SIZE) {
    bool enable = ram[addr] & 0x04;

    if (enable) {
      uint8_t bank = ram[addr];
      uint16_t code = (bank & 0xf0) << 4 | ram[addr + 1];
      int size = ram[addr + 2] & 0x03;

      /* Ensure the lower sprite code bits are masked. This is required because
       * we add the tile code offset from the lookup table for the different
       * sprite sizes. */
      code &= ~((1 << (size * 2)) - 1);

      /* the size is the number of 8x8 tiles (8x8, 16x16, 32x32, 64x64) */
      size = 1 << size;

      uint8_t b3 = ram[addr + 3];
      int xpos = ram[addr + 5] - ((b3 & 0x10) << 4);
      int ypos = ram[addr + 4] - ((b3 & 0x20) << 3);

      bool flip_x = bank & 0x01;
      bool flip_y = bank & 0x02;
      uint8_t color = b3 & 0x0f;
      uint8_t priority_mask;

      switch (b3 >> 6) {
      default:
      case 0x0:
        priority_mask = TILE_LAYER0;
        break; /* obscured by other sprites */
      case 0x1:
        priority_mask = TILE_LAYER0 | TILE_LAYER1;
        break; /* obscured by text layer */
      case 0x2:
        priority_mask = TILE_LAYER0 | TILE_LAYER1 | TILE_LAYER2;
        break; /* obscured by foreground */
      case 0x3:
        priority_mask = TILE_LAYER0 | TILE_LAYER1 | TILE_LAYER2 | TILE_LAYER3;
        break; /* obscured by background */
      }

      for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
          int x = xpos + TILE_WIDTH * (flip_x ? (size - 1 - col) : col);
          int y = ypos + TILE_HEIGHT * (flip_y ? (size - 1 - row) : row);

          tile_draw(bitmap, rom, code + sprite_tile_offset_table[row][col],
                    color, palette_offset, x, y, TILE_WIDTH, TILE_HEIGHT,
                    flip_x, flip_y, priority_mask, flags);
        }
      }
    }
  }
}
