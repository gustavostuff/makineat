# AVR128DB28 image. Needs avr-gcc with that device.
#   make -C tools/fw/avr
#   make -C tools/fw/avr CART=paddle
#   make -C tools/fw/avr CART=arkaled
#   pymcuprog write -d avr128db28 -t uart -u /dev/ttyACM0 --verify -f game.hex

MCU = avr128db28
CLOCK = 12000000
CC = avr-gcc
OBJCOPY = avr-objcopy
CART ?= space-shooter
CART_DIR = ../../../games/$(CART)

ifeq ($(CART),pong)
GAME_SRCS = $(CART_DIR)/pong.c $(CART_DIR)/title/title.c
else ifeq ($(CART),paddle)
GAME_SRCS = ../../../games/pong/pong.c ../../../games/pong/title/title.c
else ifeq ($(CART),space-shooter)
GAME_SRCS = $(CART_DIR)/game.c \
            $(CART_DIR)/entity/player.c \
            $(CART_DIR)/entity/bullet.c $(CART_DIR)/entity/rock.c
else ifeq ($(CART),arkaled)
GAME_SRCS = $(CART_DIR)/arkaled.c ../collide.c
else ifeq ($(CART),text-demo)
GAME_SRCS = $(CART_DIR)/text_demo.c
else ifeq ($(CART),snake)
GAME_SRCS = $(CART_DIR)/snake.c ../map.c
else ifeq ($(CART),super-led-bros)
GAME_SRCS = $(CART_DIR)/super_led_bros.c $(CART_DIR)/stage_01_test.c ../stage.c ../platform.c
else
$(error CART must be space-shooter, pong, arkaled, text-demo, snake, or super-led-bros)
endif

CFLAGS = -std=c11 -Wall -Wextra -Os -mmcu=$(MCU) -DF_CPU=$(CLOCK)UL -I$(CART_DIR) -I..
LDFLAGS = -mmcu=$(MCU)

SRCS = main.c hal.c sound.c $(GAME_SRCS) ../frame.c ../input.c ../flush.c \
       ../score.c ../text.c ../text_large.c ../over.c ../entity.c ../rand.c persist.c

game.elf: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

game.hex: game.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

clean:
	rm -f game.elf game.hex square.elf square.hex
