CFLAGS = -O2 -Wall -pedantic
CC = gcc

COMMON_SRC = voice.o audio/audio.a

all: wwvsim

audio/audio.a:
	$(MAKE) -C audio

wwvsim: $(COMMON_SRC) wwvsim.o
	$(CC) -o $@ wwvsim.o $(COMMON_SRC) -lm -s

helpers:
	$(MAKE) -C audio/helpers

audio-data:
	$(MAKE) -C audio/helpers audio-data

clean:
	$(MAKE) -C audio clean
	rm -f *.o
