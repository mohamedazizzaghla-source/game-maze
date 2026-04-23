CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 \
	$(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf SDL2_mixer)
LIBS = $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer) -lm

SRC = src/main.c src/joueur.c src/menu_personnage.c src/meilleurs_scores.c
TARGET = game

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
