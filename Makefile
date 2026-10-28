CC = clang
CFLAGS = -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -MMD -MP -Isrc -I/opt/homebrew/include/SDL2
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -framework CoreGraphics -framework ImageIO -framework CoreFoundation -lm

SRCS = src/main.c \
       src/game.c \
       src/pokedex.c \
       src/kanto_routes.c \
       src/balance_sim.c \
       src/image_loader.c \
       src/sprite.c \
       src/spatial_grid.c \
       src/weapons.c \
       src/dev_menu.c \
       src/profiler.c \
       src/font.c \
       src/telemetry.c \
       src/tilemap.c

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET = rogue_game

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f src/*.o src/*.d $(TARGET) test_suite

run: $(TARGET)
	./$(TARGET)

test: test_suite
	./test_suite

test_suite: test_suite.c $(OBJS)
	$(CC) $(CFLAGS) test_suite.c $(filter-out src/main.o, $(OBJS)) -o test_suite $(LDFLAGS)

.PHONY: all clean run test
