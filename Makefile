SDL_FLAGS = $(shell pkg-config --cflags --libs sdl3)

rygar: src/rygar.c
	cc -Wall -o rygar src/rygar.c $(SDL_FLAGS)

clean:
	rm rygar
.PHONY: clean

