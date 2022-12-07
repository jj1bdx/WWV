#!/bin/sh
# this requires ffmpeg
mkdir converted
for i in *; do
  ffmpeg -i ./$i -ar 16000 -ac 1 -f wav converted/$i
done
