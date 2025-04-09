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

#include "bitmap.h"
#include "tile.h"

/* sprite size (in bytes) */
#define SPRITE_SIZE 8

/* sprite tile dimensions */
#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define SPRITE_RAM_SIZE 0x800

/* There are four possible sprite sizes: 8x8, 16x16, 32x32, and 64x64. All
 * sprites are composed of a number of 8x8 tiles. This lookup table allows us
 * to easily find the offsets of the tiles which make up a sprite.
 *
 * For example, a 8x8 sprite contains only a single tile with an offset value
 * of zero. A 16x16 sprite contains four tiles, with offset values 0, 1, 2, and
 * 3. */
static const uint8_t sprite_tile_offset_table[TILE_HEIGHT][TILE_WIDTH] = {
  { 0, 1, 4, 5, 16, 17, 20, 21 },     { 2, 3, 6, 7, 18, 19, 22, 23 },
  { 8, 9, 12, 13, 24, 25, 28, 29 },   { 10, 11, 14, 15, 26, 27, 30, 31 },
  { 32, 33, 36, 37, 48, 49, 52, 53 }, { 34, 35, 38, 39, 50, 51, 54, 55 },
  { 40, 41, 44, 45, 56, 57, 60, 61 }, { 42, 43, 46, 47, 58, 59, 62, 63 }
};

/**
 * Draws the sprites to the given bitmap.
 *
 * The sprites are stored in the following format:
 *
 *  byte     bit        description
 * --------+-76543210-+----------------
 *       0 | xxxx---- | hi code
 *         | -----x-- | enable
 *         | ------x- | flip Y
 *         | -------x | flip X
 *       1 | xxxxxxxx | lo code
 *       2 | ------xx | size
 *       3 | xx-------| priority
 *         | --x----- | hi pos Y
 *         | ---x---- | hi pos X
 *         | ----xxxx | colour
 *       4 | xxxxxxxx | lo pos Y
 *       5 | xxxxxxxx | lo pos X
 *       6 | -------- |
 *       7 | -------- |
 */
void sprite_draw(bitmap_t *bitmap,
                 uint8_t *ram,
                 uint8_t *rom,
                 uint16_t palette_offset,
                 uint8_t flags);
