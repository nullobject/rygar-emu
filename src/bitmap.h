#pragma once

typedef struct {
  int width;
  int height;
  uint16_t* data;
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

void bitmap_fill(bitmap_t* bitmap, uint16_t color) {
  uint16_t* data = bitmap->data;
  uint8_t* priority = bitmap->priority;

  for (int i = 0; i < bitmap->width * bitmap->height; i++) {
    *data++ = color;
    *priority++ = 0;
  }
}
