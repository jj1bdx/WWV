# $Id: Makefile,v 1.5 2017/09/26 17:26:35 karn Exp $ Makefile for standalone WWV/WWVH program
BINDIR=/usr/local/bin
LIBDIR=/usr/local/share/ka9q-radio
CFLAGS=-g -O2

all:	wwvsim wwv.txt wwvh.txt

clean:
	rm -f *.o wwvsim
	rcsclean

install: wwvsim	
	install -D --target-directory=$(BINDIR) wwvsim
	install -D --target-directory=$(LIBDIR) wwv.txt wwvh.txt

wwvsim: wwvsim.o
	$(CC) -g -o $@ $^ -lncurses -lportaudio -lm

