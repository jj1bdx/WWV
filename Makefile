CC = gcc
CFLAGS = -O2 -Wall -pedantic

all: wwvsim

wwvsim: wwvsim.o voice.o audio/*.c
	$(MAKE) -C audio
	$(CC) -o $@ wwvsim.o voice.o audio/audio.a -lm -s

clean:
	$(MAKE) -C audio clean
	rm -f *.o
