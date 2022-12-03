CC = gcc
CFLAGS = -O2 -Wall -Wextra -pedantic -lsndfile -s

all: helpers

helpers: ann id mars nums phone hamsci 3g-shutdown geophys_ann geophys_months geophys_nums

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

hamsci: hamsci.c
	$(CC) -o hamsci $< $(CFLAGS)

3g-shutdown: 3g-shutdown.c
	$(CC) -o 3g-shutdown $< $(CFLAGS)

geophys_ann: geophys_ann.c
	$(CC) -o geophys_ann $< $(CFLAGS)

geophys_months: geophys_months.c
	 $(CC) -o geophys_months $< $(CFLAGS)

geophys_nums: geophys_nums.c
	 $(CC) -o geophys_nums $< $(CFLAGS)

clean:
	rm -f ann id mars nums phone hamsci 3g-shutdown geophys_ann geophys_months geophys_nums
