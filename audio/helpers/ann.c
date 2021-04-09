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
	int sizes[4];

	short audio[16000*60] = {0};
	char file[32];

	char *stations[] = {"wwv" , "wwvh"};
	char *ann[] = {"att", "hours", "minutes", "utc"};

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;
	int total_frames;

	int newline;

	for (int i = 0; i < 2; i++) {
		total_frames = 0; // accumulated number of frames for each station

		for (int j = 0; j < 4; j++) {
			snprintf(file, 32, "../assets/%s/%s.wav", stations[i], ann[j]);
			if (!(inf = sf_open(file, SFM_READ, &sfinfo))) return 1;

			frames = sfinfo.frames;
			sizes[j] = frames;

			sf_read_short(inf, audio+total_frames, frames);
			sf_close(inf);

			total_frames += frames;
		}

		newline = 0;

		printf("short %s_time_ann[%d] = {\n", stations[i], total_frames);
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

		fprintf(stderr, "extern short %s_time_ann[%d];\n", stations[i], total_frames);

		printf("int %s_time_ann_sizes[%d] = {", stations[i], 4);
		for (int j = 0; j < 4; j++) {
			if (j == 4 - 1) {
				printf("%d", sizes[j]);
			} else {
				printf("%d, ", sizes[j]);
			}
		}
		printf("};\n");

		fprintf(stderr, "extern int %s_time_ann_sizes[%d];\n", stations[i], 4);
	}
	return 0;
}
