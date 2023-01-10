CC = gcc
AUDIO = mars_ann.o wwvh_phone.o id.o time_nums.o time_ann.o geophys_ann.o geophys_months.o geophys_nums.o 3g-shutdown.o hamsci.o geophys_storms.o

all: audio.a

audio.a: $(AUDIO)
	ar rc audio.a *.o

clean:
	rm -f audio.a *.o
