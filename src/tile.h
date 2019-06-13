#pragma once

#define STEP2(start, step)  (start), (start)+(step)
#define STEP4(start, step)  STEP2(start, step), STEP2((start)+2*(step), step)
#define STEP8(start, step)  STEP4(start, step), STEP4((start)+4*(step), step)
#define STEP16(start, step) STEP8(start, step), STEP8((start)+8*(step), step)
#define STEP32(start, step) STEP16(start, step), STEP16((start)+16*(step), step)
#define STEP64(start, step) STEP32(start, step), STEP32((start)+32*(step), step)

typedef struct {
  // tile dimensions
  int tile_width;
  int tile_height;

  // number of bit planes
  int planes;

  // offset arrays
  int plane_offsets[8];
  int x_offsets[32];
  int y_offsets[32];

  // tile size in bits
  int tile_size;
} tile_decode_desc_t;

/**
 * Reads a single bit from the tile ROM at the given offset.
 *
 * The offset value is specified in bits, and can span across multiple bytes.
 */
static inline int read_bit(const uint8_t* src, int offset) {
	return src[offset / 8] & (0x80 >> (offset % 8));
}

/**
 * Decodes the given tile ROM to 8-bit pixel data.
 *
 * The decoded data takes up more space that the original tile ROM, but the
 * advantage is that you don't have to jump around to get the pixel data. You
 * can just iterate through the pixels sequentially, as each pixel is
 * represented by only one byte.
 */
void tile_decode(const tile_decode_desc_t* desc, uint8_t *src, uint8_t *dst, int count) {
  for (int tile = 0; tile < count; tile++) {
    uint8_t *ptr = dst + (tile * desc->tile_width * desc->tile_height);

    // Clear the bytes for the next tile.
		memset(ptr, 0, desc->tile_width * desc->tile_height);

    for (int plane = 0; plane < desc->planes; plane++) {
      int plane_bit = 1 << (desc->planes - 1 - plane);
			int plane_offset = (tile * desc->tile_size) + desc->plane_offsets[plane];

			for (int y = 0; y < desc->tile_height; y++) {
				int y_offset = plane_offset + desc->y_offsets[y];
				ptr = dst + (tile * desc->tile_width * desc->tile_height) + (y * desc->tile_width);

				for (int x = 0; x < desc->tile_width; x++) {
					if (read_bit(src, y_offset + desc->x_offsets[x])) {
            ptr[x] |= plane_bit;
          }
				}
			}
    }
  }
}
