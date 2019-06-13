void bitmap_fill(uint16_t* bitmap, uint16_t color, int size) {
  for (int i = 0; i < size; i++) {
    *bitmap++ = color;
  }
}
