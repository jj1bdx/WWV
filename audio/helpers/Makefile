CC = gcc
CFLAGS = -O2 -Wall -Wextra -pedantic -lsndfile -s

all: helpers

helpers: ann id mars nums phone

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

clean:
	rm -f ann id mars nums phone
