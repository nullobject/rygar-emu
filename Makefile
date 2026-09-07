SDL_FLAGS = $(shell pkg-config --cflags --libs sdl3)

CFLAGS = -Wall -Werror -O2 -ggdb

SRCS = src/bitmap.c src/rygar.c src/sprite.c src/tile.c src/tilemap.c
HDRS = $(wildcard src/*.h src/chips/*.h)

ROM_MANIFEST = src/roms/rygar-roms.yml
ROM_HEADER = src/roms/rygar-roms.h
ROM_FILES = $(shell python3 tools/dump.py --list $(ROM_MANIFEST))

rygar: $(SRCS) $(HDRS) $(ROM_HEADER)
	cc $(CFLAGS) -o rygar $(SRCS) $(SDL_FLAGS)

$(ROM_HEADER): $(ROM_MANIFEST) $(ROM_FILES) tools/dump.py
	python3 tools/dump.py $(ROM_MANIFEST) $(ROM_HEADER)

clean:
	rm -f rygar $(ROM_HEADER)

.DELETE_ON_ERROR:
.PHONY: clean
