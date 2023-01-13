#!/bin/sh
./ann	> ../time_ann.c		2> ../time_ann.h
./nums	> ../time_nums.c	2> ../time_nums.h
./id	> ../id.c		2> ../id.h
./phone	> ../wwvh_phone.c	2> ../wwvh_phone.h
./mars	> ../mars_ann.c		2> ../mars_ann.h
./hamsci	> ../hamsci.c		2> ../hamsci.h
./3g-shutdown	> ../3g-shutdown.c	2> ../3g-shutdown.h
./geophys_ann	> ../geophys_ann.c	2> ../geophys_ann.h
./geophys_months	>	../geophys_months.c	2> ../geophys_months.h
for i in 0 1 2 3; do
  ./geophys_nums $i	>	../geophys_nums_${i}.c	2> ../geophys_nums_${i}.h
done
./geophys_storms	>	../geophys_storms.c	2> ../geophys_storms.h
