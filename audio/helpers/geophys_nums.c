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

int main() {
	// lengths
	int sizes[99];

	short audio[16000*99*2] = {0};
	char file[32];

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;
	int total_frames;
	int not_found[99];

	int newline;

	const int default_num_size = 10;

	total_frames = 0; // accumulated number of frames for each station

	for (int i = 0; i < 99; i++) {
		not_found[i] = 0;
		snprintf(file, 32, "../assets/geophys/%d.wav", i);
		if (!(inf = sf_open(file, SFM_READ, &sfinfo))) {
			not_found[i] = 1;
			frames = default_num_size;
			sizes[i] = default_num_size;
		} else {
			frames = sfinfo.frames;
			sizes[i] = frames;

			sf_read_short(inf, audio+total_frames, frames);
			sf_close(inf);
		}

		total_frames += frames;

	}

	newline = 0;

	printf("short geophys_nums[%d] = {\n", total_frames);
	for (int i = 0; i < total_frames; i++) {
		if (i == total_frames - 1) {
			printf("%6d\n", audio[i]);
		} else {
			printf("%6d,", audio[i]);
		}
		if (++newline == 10) {
			printf("\n");
			newline = 0;
		}
	}
	printf("};\n");

	newline = 0;

	fprintf(stderr, "extern short geophys_nums[%d];\n", total_frames);

	printf("int geophys_nums_sizes[%d] = {\n", 99);
	for (int i = 0; i < 99; i++) {
		if (not_found[i]) {
			if (i == 99 - 1) {
				printf("%6d /* no audio for %d */\n", default_num_size, i);
			} else {
				printf("%6d /* no audio for %d */,", default_num_size, i);
			}
		} else {
			if (i == 99 - 1) {
				printf("%6d\n", sizes[i]);
			} else {
				printf("%6d,", sizes[i]);
			}
		}
		if (++newline == 10) {
			printf("\n");
			newline = 0;
		}
	}
	printf("};\n");

	fprintf(stderr, "extern int geophys_nums_sizes[%d];\n", 99);

	printf("\n");

	return 0;
}
