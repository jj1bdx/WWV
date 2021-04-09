CFLAGS = -O2 -Wall -pedantic
CC = gcc

COMMON_SRC = voice.o audio/audio.a

all: wwvsim

wwvsim: $(COMMON_SRC) wwvsim.o
	$(CC) -o $@ wwvsim.o $(COMMON_SRC) -lm -s

clean:
	rm -f *.o
