CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -ldl

all: fulmen

fulmen: fulmen.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f fulmen
