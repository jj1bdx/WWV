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

	int frames;
	int sizes[2];
	int total_frames;
	char *stations[] = {"wwv", "wwvh"};

	int newline = 0;

	audio = malloc(16000*60*2 * sizeof(short));

	total_frames = 0;

	for (int i = 0; i < 2; i++) {
		snprintf(file, 32, "../assets/hamsci_ann-%s.wav", stations[i]);
		if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;

		frames = sfinfo.frames;
		sizes[i] = frames;

		sf_read_short(inf, audio+total_frames, frames);
		sf_close(inf);
		total_frames += frames;
	}

	printf(HEADER);
	fprintf(stderr, HEADER);

	printf("short hamsci_ann[%d] = {\n", total_frames);
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
	fprintf(stderr, "extern short hamsci_ann[%d];\n", total_frames);

	printf("int hamsci_ann_sizes[%d] = {\n", 2);
	for (int i = 0; i < 2; i++) {
		if (i == 2 - 1) {
			printf("%6d\n", sizes[i]);
		} else {
			printf("%6d,", sizes[i]);
		}
	}
	printf("};\n");
	fprintf(stderr, "extern int hamsci_ann_sizes[%d];\n", 2);

	snprintf(file, 32, "../assets/hamsci_test.wav");
	if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;

	frames = sfinfo.frames;

	sf_read_short(inf, audio, frames);
	sf_close(inf);

	printf("short hamsci_test[%d] = {\n", frames);
	for (int i = 0; i < frames; i++) {
		if (i == frames - 1) {
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
	fprintf(stderr, "extern short hamsci_test[%d];\n", frames);

	printf("int hamsci_test_size = %d;\n", frames);
	fprintf(stderr, "extern int hamsci_test_size;\n");

exit:
	free(audio);

	return 0;
}
