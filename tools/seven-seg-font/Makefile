# From the repo root:
#   make -C tools/seven-seg-font
#   ./tools/seven-seg-font/seven-seg-font

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I../fw -I../sim
SDL_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf)
LDLIBS = $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf)

seven-seg-font: main.c ../sim/seven_seg.c ../fw/seven_seg_font.c ../sim/chips/max7219.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ main.c ../sim/seven_seg.c ../fw/seven_seg_font.c ../sim/chips/max7219.c $(LDLIBS)

clean:
	rm -f seven-seg-font
