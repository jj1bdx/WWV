# $Id: Makefile,v 1.8 2018/11/22 10:01:04 karn Exp $ Makefile for standalone WWV/WWVH program
BINDIR=/usr/local/bin
WWV_DIR=/usr/local/share/ka9q-radio/wwv
WWVH_DIR=/usr/local/share/ka9q-radio/wwvh
CFLAGS=-g -O2 -I/opt/local/include

all:	wwvsim

clean:
	rm -f *.o wwvsim
	rcsclean

install: wwvsim	
	install -D --target-directory=$(BINDIR) wwvsim
	install -D --target-directory=$(WWV_DIR) wwv-id.txt wwv-id.raw
	install -D --target-directory=$(WWVH_DIR) wwvh-id.txt wwvh-id.raw
	ln -f $(WWV_DIR)/wwv-id.raw $(WWV_DIR)/0.raw
	ln -f $(WWV_DIR)/wwv-id.txt $(WWV_DIR)/0.txt
	ln -f $(WWV_DIR)/wwv-id.raw $(WWV_DIR)/30.raw
	ln -f $(WWV_DIR)/wwv-id.txt $(WWV_DIR)/30.txt
	ln -f $(WWVH_DIR)/wwvh-id.raw $(WWVH_DIR)/29.raw
	ln -f $(WWVH_DIR)/wwvh-id.txt $(WWVH_DIR)/29.txt
	ln -f $(WWVH_DIR)/wwvh-id.raw $(WWVH_DIR)/59.raw
	ln -f $(WWVH_DIR)/wwvh-id.txt $(WWVH_DIR)/59.txt

wwvsim: wwvsim.o
	$(CC) -g -o $@ $^ -lportaudio -lm

