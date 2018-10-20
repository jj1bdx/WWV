# $Id: Makefile,v 1.7 2018/10/20 02:59:16 karn Exp $ Makefile for standalone WWV/WWVH program
BINDIR=/usr/local/bin
LIBDIR=/usr/local/share/ka9q-radio
CFLAGS=-g -O2 -I/opt/local/include

all:	wwvsim wwv.txt wwvh.txt

clean:
	rm -f *.o wwvsim
	rcsclean

install: wwvsim	
	install -D --target-directory=$(BINDIR) wwvsim
	install -D --target-directory=$(LIBDIR) wwv.txt wwvh.txt

wwvsim: wwvsim.o
	$(CC) -g -o $@ $^ -lportaudio -lm

