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
	int sizes[2];

	short audio[16000*60*2] = {0};
	char file[32];

	char *stations[] = {"wwv", "wwvh"};

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;
	int total_frames = 0;

	int newline = 0;

	for (int i = 0; i < 2; i++) {
		snprintf(file, 32, "../assets/%s/id.wav", stations[i]);
		if (!(inf = sf_open(file, SFM_READ, &sfinfo))) return 1;

		frames = sfinfo.frames;
		sizes[i] = frames;

		sf_read_short(inf, audio + total_frames, frames);
		sf_close(inf);

		total_frames += frames;
	}

	printf("short id[%d] = {\n", total_frames);
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
	fprintf(stderr, "extern short id[%d];\n", total_frames);

	printf("int id_sizes[%d] = {\n", 2);
	for (int i = 0; i < 2; i++) {
		if (i == 2 - 1) {
			printf("%6d\n", sizes[i]);
		} else {
			printf("%6d,", sizes[i]);
		}
	}
	printf("};\n");
	fprintf(stderr, "extern int id_sizes[%d];\n", 2);

	return 0;
}
