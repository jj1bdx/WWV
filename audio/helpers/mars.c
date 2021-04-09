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
	int sizes[2];

	short audio[16000*60];
	char file[32];

	char *stations[] = {"wwv" , "wwvh"};

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;

	int newline;

	for (int k = 0; k < 2; k++) {
		snprintf(file, 32, "../assets/%s/mars.wav", stations[k]);
		if (!(inf = sf_open(file, SFM_READ, &sfinfo))) return 1;

		frames = sfinfo.frames;
		sizes[k] = frames;

		sf_read_short(inf, audio, frames);
		sf_close(inf);

		newline = 0;

		printf("short %s_mars_ann[%d] = {\n", stations[k], frames);
		for (int j = 0; j < frames; j++) {
			if (j == frames - 1) {
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
		fprintf(stderr, "extern short %s_mars_ann[%d];\n", stations[k], frames);
	}

	printf("int mars_ann_sizes[2] = {");
	for (int i = 0; i < 2; i++) {
		if (i == 2 - 1) {
			printf("%6d", sizes[i]);
		} else {
			printf("%d, ", sizes[i]);
		}
	}
	printf("};\n");
	fprintf(stderr, "extern int mars_ann_sizes[2];\n");

	return 0;
}
