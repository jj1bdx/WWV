# $Id: Makefile,v 1.7 2018/10/20 02:59:16 karn Exp karn $ Makefile for standalone WWV/WWVH program
BINDIR=/usr/local/bin
WWVDIR=/usr/local/share/ka9q-radio/wwv
WWVHDIR=/usr/local/share/ka9q-radio/wwvh
CFLAGS=-g -O2 -I/opt/local/include

all:	wwvsim

clean:
	rm -f *.o wwvsim
	rcsclean

install: wwvsim	
	install -D --target-directory=$(BINDIR) wwvsim
	install -D --target-directory=$(WWVDIR) wwv-id.txt wwv-id.raw
	install -D --target-directory=$(WWVHDIR) wwvh-id.txt wwvh-id.raw
	ln -f $(WWVDIR)/wwv-id.raw $(WWVDIR)/0.raw
	ln -f $(WWVDIR)/wwv-id.txt $(WWVDIR)/0.txt
	ln -f $(WWVDIR)/wwv-id.raw $(WWVDIR)/30.raw
	ln -f $(WWVDIR)/wwv-id.txt $(WWVDIR)/30.txt
	ln -f $(WWVHDIR)/wwvh-id.raw $(WWVDIR)/29.raw
	ln -f $(WWVHDIR)/wwvh-id.txt $(WWVDIR)/29.txt
	ln -f $(WWVHDIR)/wwvh-id.raw $(WWVDIR)/59.raw
	ln -f $(WWVHDIR)/wwvh-id.txt $(WWVDIR)/59.txt

wwvsim: wwvsim.o
	$(CC) -g -o $@ $^ -lportaudio -lm

