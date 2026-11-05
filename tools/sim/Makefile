# From the repo root:
#   make -C tools/sim
#   ./tools/sim/makineat-ui
#   make -C tools/sim test
#   make -C tools/sim PONG_DEBUG=1 makineat-ui      # pong ball at 1 FPS
#   make -C tools/sim ARKALED_DEBUG=1 makineat-ui   # arkaled ball 0.25 FPS, paddles 1 FPS

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I../fw -I../../games/space-shooter -I../../games/pong -I../../games/arkaled -I../../games/text-demo -I../../games/snake -I.
ifeq ($(PONG_DEBUG),1)
CFLAGS += -DPONG_DEBUG_SLOW_BALL=1
endif
ifeq ($(ARKALED_DEBUG),1)
CFLAGS += -DARKALED_DEBUG_SLOW=1
endif
SDL_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf)
LDLIBS = $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf)

CORE = engine/signal.c chips/max7219.c chips/matrix8x8.c chips/status_digits.c chips/avr128db28.c board.c ../fw/wire.c
COMMON = ../fw/frame.c ../fw/input.c ../fw/score.c ../fw/text.c ../fw/text_large.c ../fw/over.c ../fw/entity.c ../fw/status.c ../fw/rand.c
SHOOTER = ../../games/space-shooter/game.c \
          ../../games/space-shooter/entity/player.c \
          ../../games/space-shooter/entity/bullet.c \
          ../../games/space-shooter/entity/rock.c
PONG = ../../games/pong/pong.c ../../games/pong/title/title.c
ARKALED = ../../games/arkaled/arkaled.c
TEXT_DEMO = ../../games/text-demo/text_demo.c
SNAKE = ../../games/snake/snake.c
COLLIDE = ../fw/collide.c
FW = $(COMMON) $(SHOOTER) ../fw/flush.c

TEST_BINS = tests/test_max7219 tests/test_matrix tests/test_status_digits tests/test_avr128db28 tests/test_entity tests/test_map tests/test_board tests/test_game \
            tests/test_pong tests/test_arkaled tests/test_text_demo tests/test_snake tests/test_cart tests/test_synth tests/test_acetate
CHECK = tests/report.c

UI_HDRS = acetate.h seven_seg.h board.h cart.h host.h

makineat-ui: main.c seven_seg.c acetate.c acetate_geom.c hal.c persist.c sound.c cart.c $(CORE) $(FW) $(PONG) $(ARKALED) $(TEXT_DEMO) $(SNAKE) $(COLLIDE) ../fw/map.c ../fw/synth.c $(UI_HDRS)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ main.c seven_seg.c acetate.c acetate_geom.c hal.c persist.c sound.c cart.c $(CORE) $(FW) $(PONG) $(ARKALED) $(TEXT_DEMO) $(SNAKE) $(COLLIDE) ../fw/map.c ../fw/synth.c $(LDLIBS)

tests/test_max7219: tests/test_max7219.c chips/max7219.c engine/signal.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_max7219.c chips/max7219.c engine/signal.c $(CHECK)

tests/test_matrix: tests/test_matrix.c chips/matrix8x8.c chips/max7219.c engine/signal.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_matrix.c chips/matrix8x8.c chips/max7219.c engine/signal.c $(CHECK)

tests/test_status_digits: tests/test_status_digits.c seven_seg.c chips/status_digits.c chips/max7219.c engine/signal.c $(CHECK)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ tests/test_status_digits.c seven_seg.c chips/status_digits.c chips/max7219.c engine/signal.c $(LDLIBS) $(CHECK)

tests/test_avr128db28: tests/test_avr128db28.c chips/avr128db28.c engine/signal.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_avr128db28.c chips/avr128db28.c engine/signal.c $(CHECK)

tests/test_entity: tests/test_entity.c ../fw/entity.c ../fw/frame.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_entity.c ../fw/entity.c ../fw/frame.c $(CHECK)

tests/test_map: tests/test_map.c ../fw/map.c ../fw/entity.c ../fw/frame.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_map.c ../fw/map.c ../fw/entity.c ../fw/frame.c $(CHECK)

tests/test_board: tests/test_board.c hal.c $(CORE) $(FW) ../fw/persist_ram.c tests/sound_log.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_board.c hal.c $(CORE) $(FW) ../fw/persist_ram.c tests/sound_log.c $(CHECK)

tests/test_game: ../../games/space-shooter/test.c $(COMMON) $(SHOOTER) ../fw/persist_ram.c tests/sound_log.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ ../../games/space-shooter/test.c $(COMMON) $(SHOOTER) ../fw/persist_ram.c tests/sound_log.c $(CHECK)

tests/test_pong: ../../games/pong/test.c ../../games/pong/test_grids.c ../../games/pong/test_grids.h $(COMMON) $(PONG) ../fw/persist_ram.c tests/sound_log.c $(CHECK)
	$(CC) $(CFLAGS) -DPONG_DEBUG_SLOW_BALL=0 -o $@ ../../games/pong/test.c ../../games/pong/test_grids.c $(COMMON) $(PONG) ../fw/persist_ram.c tests/sound_log.c $(CHECK)

tests/test_arkaled: ../../games/arkaled/test.c $(COMMON) $(ARKALED) $(COLLIDE) ../fw/persist_ram.c tests/sound_log.c $(CHECK)
	$(CC) $(CFLAGS) -DARKALED_DEBUG_SLOW=0 -o $@ ../../games/arkaled/test.c $(COMMON) $(ARKALED) $(COLLIDE) ../fw/persist_ram.c tests/sound_log.c $(CHECK)

tests/test_text_demo: ../../games/text-demo/test.c $(COMMON) $(TEXT_DEMO) ../fw/persist_ram.c tests/sound_log.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ ../../games/text-demo/test.c $(COMMON) $(TEXT_DEMO) ../fw/persist_ram.c tests/sound_log.c $(CHECK)

tests/test_snake: ../../games/snake/test.c $(COMMON) $(SNAKE) ../fw/map.c ../fw/persist_ram.c tests/sound_log.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ ../../games/snake/test.c $(COMMON) $(SNAKE) ../fw/map.c ../fw/persist_ram.c tests/sound_log.c $(CHECK)

tests/test_cart: tests/test_cart.c cart.c $(COMMON) $(SHOOTER) $(PONG) $(ARKALED) $(TEXT_DEMO) $(SNAKE) $(COLLIDE) ../fw/map.c ../fw/persist_ram.c tests/sound_log.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_cart.c cart.c $(COMMON) $(SHOOTER) $(PONG) $(ARKALED) $(TEXT_DEMO) $(SNAKE) $(COLLIDE) ../fw/map.c ../fw/persist_ram.c tests/sound_log.c $(CHECK)

tests/test_synth: tests/test_synth.c ../fw/synth.c ../fw/wire.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_synth.c ../fw/synth.c ../fw/wire.c $(CHECK)

tests/test_acetate: tests/test_acetate.c acetate_geom.c $(CHECK)
	$(CC) $(CFLAGS) -o $@ tests/test_acetate.c acetate_geom.c $(CHECK)

test: $(TEST_BINS)
	./tests/test_max7219
	./tests/test_matrix
	./tests/test_status_digits
	./tests/test_avr128db28
	./tests/test_entity
	./tests/test_map
	./tests/test_game
	./tests/test_pong
	./tests/test_arkaled
	./tests/test_text_demo
	./tests/test_snake
	./tests/test_cart
	./tests/test_board
	./tests/test_synth
	./tests/test_acetate

clean:
	rm -f makineat-ui maki-nita-ui makinita-ui retrodiode-ui test_chips $(TEST_BINS)
