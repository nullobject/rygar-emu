SDL_FLAGS = $(shell pkg-config --cflags --libs sdl3)

rygar: src/rygar.c
	gcc -Wall -ggdb -o rygar src/rygar.c $(SDL_FLAGS)

clean:
	rm rygar
.PHONY: clean

