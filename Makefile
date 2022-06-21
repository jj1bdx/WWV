CC = gcc
CFLAGS = -O2 -Wall -Wextra -pedantic -std=c11

all: wwvsim

wwvsim: wwvsim.o voice.o audio/*.c
	$(MAKE) -C audio
	$(CC) -o $@ wwvsim.o voice.o audio/audio.a -lm -s -pthread

clean:
	$(MAKE) -C audio clean
	rm -f *.o
