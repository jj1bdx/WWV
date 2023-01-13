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
#include "header.h"

int main(int argc, char **argv) {
	/* lengths */
	unsigned int sizes[99 + 1];

	short *audio;
	char file[32];

	SNDFILE *inf;
	SF_INFO sfinfo;

	unsigned int frames;
	unsigned int total_frames;
	unsigned int total_frames_per_segment[4 + 1];
	int not_found[99 + 1];

	int newline;

	const int default_num_size = 10;

	int segment;

	if (argc < 1) return 1;

	segment = strtol(argv[1], NULL, 10);

	audio = malloc(16000*(99 + 1)*2 * sizeof(short));

	total_frames = 0; /* accumulated number of frames */

	for (int j = 0; j < 99 + 1; j++) {
		not_found[j] = 0;
		snprintf(file, 32, "../assets/geophys/%d.wav", segment * 100 + j);
		if (!(inf = sf_open(file, SFM_READ, &sfinfo))) {
			not_found[j] = 1;
			frames = default_num_size;
			sizes[j] = default_num_size;
		} else {
			frames = sfinfo.frames;
			sizes[j] = frames;

			sf_read_short(inf, audio+total_frames, frames);
			sf_close(inf);
		}
		total_frames += frames;
	}
	total_frames_per_segment[segment] = total_frames;

	newline = 0;

	printf(HEADER);
	fprintf(stderr, HEADER);

	printf("short geophys_nums_%d_%d[%d] = {\n",
		segment * 100, segment * 100 + 99, total_frames_per_segment[segment]);
	for (unsigned int j = 0; j < total_frames_per_segment[segment]; j++) {
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

	newline = 0;

	printf("int geophys_nums_%d_%d_sizes[%d] = {\n",
		segment * 100, segment * 100 + 99, total_frames_per_segment[segment]);
	for (int j = 0; j < 99 + 1; j++) {
		if (not_found[j]) {
			if (j == 99) {
				printf("%6d /* no audio for %d */\n", default_num_size, j);
			} else {
				printf("%6d /* no audio for %d */,", default_num_size, j);
			}
		} else {
			if (j == 99) {
				printf("%6d\n", sizes[j]);
			} else {
				printf("%6d,", sizes[j]);
			}
		}
		if (++newline == 10) {
			printf("\n");
			newline = 0;
		}
	}
	printf("};\n");

	fprintf(stderr, "extern short geophys_nums_%d_%d[%d];\n",
		segment * 100, segment * 100 + 99, total_frames_per_segment[segment]);
	fprintf(stderr, "extern int geophys_nums_%d_%d_sizes[%d];\n",
		segment * 100, segment * 100 + 99, 99 + 1);

	free(audio);

	return 0;
}
