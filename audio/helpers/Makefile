CC = gcc
CFLAGS = -O2 -Wall -Wextra -pedantic -lsndfile -s

all: helpers

helpers: ann id mars nums phone hamsci 3g-shutdown

audio-src:
	./create-audio-data.sh

ann: ann.c
	$(CC) -o ann $< $(CFLAGS)

id: id.c
	$(CC) -o id $< $(CFLAGS)

mars: mars.c
	$(CC) -o mars $< $(CFLAGS)

nums: nums.c
	$(CC) -o nums $< $(CFLAGS)

phone: phone.c
	$(CC) -o phone $< $(CFLAGS)

hamsci: hamsci.o
	$(CC) -o hamsci $< $(CFLAGS)

3g-shutdown: 3g-shutdown.c
	$(CC) -o 3g-shutdown $< $(CFLAGS)

clean:
	rm -f ann id mars nums phone hamsci
