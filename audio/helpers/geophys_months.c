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

int main() {
	/* lengths */
	int sizes[12];

	short *audio;
	char file[32];

	char *months[] = {
		"January",
		"February",
		"March",
		"April",
		"May",
		"June",
		"July",
		"August",
		"September",
		"October",
		"November",
		"December"
	};

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;
	int total_frames;
	int not_found[12];

	int newline;

	const int default_num_size = 10;

	audio = malloc(16000*3*12 * sizeof(short));

	total_frames = 0; /* accumulated number of frames for each station */

	for (int i = 0; i < 12; i++) {
		not_found[i] = 0;
		snprintf(file, 32, "../assets/geophys/%s.wav", months[i]);
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

	printf(HEADER);
	printf("short geophys_months[%d] = {\n", total_frames);
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

	printf("int geophys_months_sizes[%d] = {\n", 12);
	for (int i = 0; i < 12; i++) {
		if (not_found[i]) {
			if (i == 12 - 1) {
				printf("%6d /* no audio for %d */\n", default_num_size, i);
			} else {
				printf("%6d /* no audio for %d */,", default_num_size, i);
			}
		} else {
			if (i == 12 - 1) {
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
	printf("\n");

	fprintf(stderr, HEADER);
	fprintf(stderr, "extern short geophys_months[%d];\n", total_frames);
	fprintf(stderr, "extern int geophys_months_sizes[%d];\n", 12);

	free(audio);

	return 0;
}
