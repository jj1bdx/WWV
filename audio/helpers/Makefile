CC = gcc
CFLAGS = -O2 -Wall -pedantic -lsndfile -s

all: ann id mars nums phone

ann:
	$(CC) -o ann ann.c $(CFLAGS)
id:
	$(CC) -o id id.c $(CFLAGS)
mars:
	$(CC) -o mars mars.c $(CFLAGS)
nums:
	$(CC) -o nums nums.c $(CFLAGS)
phone:
	$(CC) -o phone phone.c $(CFLAGS)

audio-data:
	./create-audio-data.sh

clean:
	rm -f ann id mars nums phone
