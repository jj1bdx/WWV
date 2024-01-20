CC = gcc
AUDIO = 3g-shutdown.o geophys_ann.o geophys_months.o geophys_nums_0.o geophys_nums_1.o geophys_nums_2.o geophys_nums_3.o geophys_storms.o hamsci.o id.o mars_ann.o time_ann.o time_nums.o wwvh_phone.o

all: audio.a

audio.a: $(AUDIO)
	ar rc audio.a *.o

clean:
	rm -f audio.a *.o
