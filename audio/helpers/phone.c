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

	short audio[16000*60];
	char file[32] = "../assets/wwvh/phone.wav";

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;
	int newline;

	if (!(inf = sf_open(file, SFM_READ, &sfinfo))) {
		frames = 2;
		audio[0] = audio[1] = 0;
	} else {
		frames = sfinfo.frames;
		sf_read_short(inf, audio, frames);
		sf_close(inf);
	}

	newline = 0;

	printf("short wwvh_phone[%d] = {\n", frames);
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

	printf("int wwvh_phone_size = %d;\n", frames);

	fprintf(stderr, "extern short wwvh_phone[%d];\n", frames);
	fprintf(stderr, "extern int wwvh_phone_size;\n");

	return 0;
}
