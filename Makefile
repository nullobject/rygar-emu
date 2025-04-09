SDL_FLAGS = $(shell pkg-config --cflags --libs sdl3)

rygar: src/rygar.c
	cc -Wall -ggdb -o rygar src/bitmap.c  src/rygar.c src/sprite.c src/tile.c src/tilemap.c $(SDL_FLAGS)

clean:
	rm rygar
.PHONY: clean

