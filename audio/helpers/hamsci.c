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
	short *audio;
	char file[32];

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames[2];

	int newline = 0;

	audio = malloc(16000*60*2 * sizeof(short));

	snprintf(file, 32, "../assets/hamsci_ann.wav");
	if (!(inf = sf_open(file, SFM_READ, &sfinfo))) return 1;

	frames[0] = sfinfo.frames;

	sf_read_short(inf, audio, frames[0]);
	sf_close(inf);

	printf(HEADER);
	printf("short hamsci_ann[%d] = {\n", frames[0]);
	for (int i = 0; i < frames[0]; i++) {
		if (i == frames[0] - 1) {
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
	printf("int hamsci_ann_size = %d;\n", frames[0]);

	snprintf(file, 32, "../assets/hamsci_test.wav");
	if (!(inf = sf_open(file, SFM_READ, &sfinfo))) return 1;

	frames[1] = sfinfo.frames;

	sf_read_short(inf, audio, frames[1]);
	sf_close(inf);

	printf("short hamsci_test[%d] = {\n", frames[1]);
	for (int i = 0; i < frames[1]; i++) {
		if (i == frames[1] - 1) {
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
	printf("int hamsci_test_size = %d;\n", frames[1]);

	fprintf(stderr, HEADER);
	fprintf(stderr, "extern short hamsci_ann[%d];\n", frames[0]);
	fprintf(stderr, "extern int hamsci_ann_size;\n");
	fprintf(stderr, "extern short hamsci_test[%d];\n", frames[1]);
	fprintf(stderr, "extern int hamsci_test_size;\n");

	free(audio);

	return 0;
}
