/*
 * Helper for wwvsim
 * Copyright (C) 2021 Anthony96922
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <sndfile.h>

int main(int argc, char *argv[]) {
	// lengths
	int sizes[61];

	short audio[16000*60*2] = {0};
	char file[32];

	char *stations[] = {"wwv" , "wwvh"};

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;
	int total_frames;
	int not_found[61];

	int newline;

	for (int i = 0; i < 2; i++) {
		total_frames = 0; // accumulated number of frames for each station

		for (int j = 0; j < 61; j++) {
			not_found[j] = 0;
			snprintf(file, 32, "../assets/%s/%d.wav", stations[i], j);
			if (!(inf = sf_open(file, SFM_READ, &sfinfo))) {
				not_found[j] = 1;
				frames = 10;
				sizes[j] = 10;
			} else {
				frames = sfinfo.frames;
				sizes[j] = frames;

				sf_read_short(inf, audio+total_frames, frames);
				sf_close(inf);
			}

			total_frames += frames;

		}

		newline = 0;

		printf("short %s_nums[%d] = {\n", stations[i], total_frames);
		for (int j = 0; j < total_frames; j++) {
			if (j == total_frames - 1) {
				printf("%6d\n", audio[j]);
			} else {
				printf("%6d,", audio[j]);
			}
			if (++newline == 10) {
				printf("\n");
				newline = 0;
			}
		}
		printf("};\n");

		fprintf(stderr, "extern short %s_nums[%d];\n", stations[i], total_frames);

		printf("int %s_nums_sizes[%d] = {", stations[i], 61);
		for (int j = 0; j < 61; j++) {
			if (not_found[j]) {
				if (j == 61 - 1) {
					printf("0 /* no audio for %d */", j);
				} else {
					printf("0 /* no audio for %d */, ", j);
				}
			} else {
				if (j == 61 - 1) {
					printf("%d", sizes[j]);
				} else {
					printf("%d, ", sizes[j]);
				}
			}
		}
		printf("};\n");

		fprintf(stderr, "extern int %s_nums_sizes[%d];\n", stations[i], 60);

		printf("\n");
	}
	return 0;
}
