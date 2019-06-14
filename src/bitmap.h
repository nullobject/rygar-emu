#pragma once

typedef struct {
  // dimensions
  int width;
  int height;

  // bitmap data
  uint16_t* data;

  // priority map
  uint8_t* priority;
} bitmap_t;

void bitmap_init(bitmap_t* bitmap, int width, int height) {
  memset(bitmap, 0, sizeof(bitmap_t));
  bitmap->width = width;
  bitmap->height = height;
  bitmap->data = malloc(width * height * sizeof(uint16_t));
  bitmap->priority = malloc(width * height * sizeof(uint8_t));
}

void bitmap_shutdown(bitmap_t* bitmap) {
  free(bitmap->data);
  free(bitmap->priority);
  bitmap->data = 0;
  bitmap->priority = 0;
}

uint16_t* bitmap_data(bitmap_t* bitmap, int x, int y) {
  return bitmap->data + y*bitmap->width + x;
}

uint8_t* bitmap_priority(bitmap_t* bitmap, int x, int y) {
  return bitmap->priority + y*bitmap->width + x;
}

void bitmap_fill(bitmap_t* bitmap, uint16_t color) {
  uint16_t* data = bitmap->data;
  uint8_t* priority = bitmap->priority;

  for (int i = 0; i < bitmap->width * bitmap->height; i++) {
    *data++ = color;
    *priority++ = 0;
  }
}

void bitmap_copy(bitmap_t* src, bitmap_t* dst, int scroll_x) {
  uint16_t* data = dst->data;
  uint8_t* priority = dst->priority;

  for (int y = 0; y < dst->height; y++) {
    for (int x = 0; x < dst->width; x++) {
      // Calculate the wrapped x coordinate in tilemap space. Wrapping occurs
      // when the visible area is outside of the tilemap.
      uint32_t wrapped_x = (x + scroll_x) % src->width;
      uint32_t addr = y*src->width + wrapped_x;

      if (src->priority[addr]) {
        *data = src->data[addr];
        *priority = src->priority[addr];
      }

      data++;
      priority++;
    }
  }
}
