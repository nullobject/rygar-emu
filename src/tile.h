#pragma once

#include <stdint.h>
#include <string.h>

/* step macros */
#define STEP2(start, step) (start), (start)+(step)
#define STEP4(start, step) STEP2(start, step), STEP2((start)+2*(step), step)
#define STEP8(start, step) STEP4(start, step), STEP4((start)+4*(step), step)
#define STEP16(start, step) STEP8(start, step), STEP8((start)+8*(step), step)
#define STEP32(start, step) STEP16(start, step), STEP16((start)+16*(step), step)
#define STEP64(start, step) STEP32(start, step), STEP32((start)+32*(step), step)

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

  /* tile size (in bits) */
  int tile_size;
} tile_decode_desc_t;

/**
 * Reads a single bit value from the tile ROM at the given offset.
 *
 * The offset value is specified in bits, and can span across multiple bytes.
 */
static inline int read_bit(const uint8_t *rom, int offset) {
	return rom[offset / 8] & (0x80 >> (offset % 8));
}

/**
 * Decodes the given tile ROM to 8-bit pixel data.
 *
 * The decoded data takes up more space that the original tile ROM, but the
 * advantage is that you don't have to jump around to get the pixel data. You
 * can just iterate through the pixels sequentially, as each pixel is
 * represented by only one byte.
 */
void tile_decode(const tile_decode_desc_t *desc, uint8_t *rom, uint8_t *dst, int count) {
  for (int tile = 0; tile < count; tile++) {
    uint8_t *ptr = dst + (tile * desc->tile_width * desc->tile_height);

    /* clear the bytes for the next tile */
		memset(ptr, 0, desc->tile_width * desc->tile_height);

    for (int plane = 0; plane < desc->planes; plane++) {
      int plane_bit = 1 << (desc->planes - 1 - plane);
			int plane_offset = (tile * desc->tile_size) + desc->plane_offsets[plane];

			for (int y = 0; y < desc->tile_height; y++) {
				int y_offset = plane_offset + desc->y_offsets[y];
				ptr = dst + (tile * desc->tile_width * desc->tile_height) + (y * desc->tile_width);

				for (int x = 0; x < desc->tile_width; x++) {
					if (read_bit(rom, y_offset + desc->x_offsets[x])) {
            ptr[x] |= plane_bit;
          }
				}
			}
    }
  }
}

/**
 * Draws a single pixel.
 */
static inline void tile_draw_pixel(uint16_t *data, uint8_t *priority, uint8_t priority_mask, uint16_t palette_offset, uint8_t color, uint8_t pen, uint8_t flags) {
  /* bail out if we're using the transparent pen */
  if (pen == TRANSPARENT_PEN && !(flags & TILE_OPAQUE)) return;

  /* bail out if there's already a pixel with higher priority */
  if ((*priority & priority_mask) != 0) return;

  *data = palette_offset | color << 4 | pen;
  *priority = (pen != TRANSPARENT_PEN) ? flags & TILE_LAYER_MASK : 0;
}

/**
 * Draws the given tile to a bitmap.
 */
void tile_draw(
  bitmap_t *bitmap,
  uint8_t *rom,
  uint16_t code,
  uint8_t color,
  uint16_t palette_offset,
  int x, int y,
  int width, int height,
  bool flip_x, bool flip_y,
  uint8_t priority_mask,
  uint8_t flags
) {
  /* bail out if the tile is completely off-screen */
  if (x < 0 - width - 1 || y < 0 - height - 1 || x >= bitmap->width || y >= bitmap->height) return;

  uint16_t *data = bitmap_data(bitmap, x, y);
  uint8_t *priority = bitmap_priority(bitmap, x, y);
  uint8_t *tile = rom + (code * width * height);

  int flip_mask_x = flip_x ? (width - 1) : 0;
  int flip_mask_y = flip_y ? (height - 1) : 0;

  for (int v = 0; v < height; v++) {
    /* ensure we're inside the bitmap */
    if (y + v < 0 || y + v >= bitmap->height) continue;

    for (int u = 0; u < width; u++) {
      /* ensure we're inside the bitmap */
      if (x + u < 0 || x + u >= bitmap->width) continue;

      int offset = (v * bitmap->width) + u;
      uint8_t pen = tile[(v ^ flip_mask_y) * width + (u ^ flip_mask_x)] & 0xf;

      tile_draw_pixel(
        data + offset,
        priority + offset,
        priority_mask,
        palette_offset,
        color,
        pen,
        flags
      );
    }
  }
}
