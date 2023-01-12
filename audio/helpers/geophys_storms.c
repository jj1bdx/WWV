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
	int sizes[10];
	int sizes_counter = 0;

	short *audio;
	char file[64];

	char *times[] = {"obs", "pred"};
	char *events[] = {"rb", "space_wx", "srs"};
	char *prob[] = {"likely", "expected", "expected_to_cont"};
	char *levels[] = {"minor", "moderate", "strong", "severe", "extreme"};
	char *no_storms[] = {"no_past_storms", "no_future_storms"};

	SNDFILE *inf;
	SF_INFO sfinfo;

	int frames;
	int total_frames;

	int newline;

	audio = malloc(16000*10*9 * sizeof(short));

	printf(HEADER);
	fprintf(stderr, HEADER);

	/* observed and predicted */
	for (int i = 0; i < 2; i++) {
		total_frames = 0;
		newline = 0;

		/* radio blackout */
		sizes_counter = 5;
		if (i == 1) {
			int m = 0;
			sizes_counter = 10;
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 5; k++) {
					snprintf(file, 64, "../assets/geophys/%s_%s_r%d_%s.wav",
					times[i], events[0], k + 1, prob[j]);
					if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;

					frames = sfinfo.frames;
					sizes[m] = frames;

					sf_read_short(inf, audio+total_frames, frames);
					sf_close(inf);

					total_frames += frames;
					m++;
				}
			}
		} else {
			for (int j = 0; j < 5; j++) {
				snprintf(file, 64, "../assets/geophys/%s_%s_r%d.wav",
				times[i], events[0], j + 1);
				if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;

				frames = sfinfo.frames;
				sizes[j] = frames;

				sf_read_short(inf, audio+total_frames, frames);
				sf_close(inf);

				total_frames += frames;
			}
		}

		printf("short geophys_%s_%s[%d] = {\n", times[i], events[0], total_frames);
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
		fprintf(stderr, "extern short geophys_%s_%s[%d];\n", times[i], events[0], total_frames);

		newline = 0;

		printf("int geophys_%s_%s_sizes[%d] = {\n", times[i], events[0], sizes_counter);
		for (int j = 0; j < sizes_counter; j++) {
			if (j == sizes_counter - 1) {
				printf("%6d\n", sizes[j]);
			} else {
				printf("%6d,", sizes[j]);
			}
			if (++newline == 10) {
				printf("\n");
				newline = 0;
			}
		}
		printf("};\n");
		fprintf(stderr, "extern int geophys_%s_%s_sizes[%d];\n", times[i], events[0], sizes_counter);

		total_frames = 0;
		newline = 0;

		/* space weather */
		sizes_counter = 5;
		for (int j = 0; j < 5; j++) {
			snprintf(file, 64, "../assets/geophys/%s_%s_%s.wav",
				times[i], events[1], levels[j]);
			if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;
			frames = sfinfo.frames;
			sizes[j] = frames;

			sf_read_short(inf, audio+total_frames, frames);
			sf_close(inf);
			total_frames += frames;
		}

		printf("short geophys_%s_%s[%d] = {\n", times[i], events[1], total_frames);
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
		fprintf(stderr, "extern short geophys_%s_%s[%d];\n", times[i], events[1], total_frames);

		total_frames = 0;
		newline = 0;

		printf("int geophys_%s_%s_sizes[%d] = {\n", times[i], events[1], sizes_counter);
		for (int j = 0; j < sizes_counter; j++) {
			if (j == sizes_counter - 1) {
				printf("%6d\n", sizes[j]);
			} else {
				printf("%6d,", sizes[j]);
			}
			if (++newline == 10) {
				printf("\n");
				newline = 0;
			}
		}
		printf("};\n");
		fprintf(stderr, "extern int geophys_%s_%s_sizes[%d];\n", times[i], events[1], sizes_counter);

		total_frames = 0;
		newline = 0;

		/* solar radiation storms */
		int m = 0;
		sizes_counter = 10;
		if (i == 1) {
			sizes_counter = 15;
			for (int j = 0; j < 3; j++) {
				for (int k = 0; k < 5; k++) {
					snprintf(file, 64, "../assets/geophys/%s_%s_s%d_%s.wav",
					times[i], events[2], k + 1, prob[j]);
					if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;

					frames = sfinfo.frames;
					sizes[m] = frames;

					sf_read_short(inf, audio+total_frames, frames);
					sf_close(inf);

					total_frames += frames;
					m++;
				}
			}
		} else {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 5; k++) {
					snprintf(file, 64, "../assets/geophys/%s_%s_s%d_%s.wav",
					times[i], events[2], k + 1, prob[j]);
					if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;

					frames = sfinfo.frames;
					sizes[m] = frames;

					sf_read_short(inf, audio+total_frames, frames);
					sf_close(inf);
					total_frames += frames;
					m++;
				}
			}
		}

		printf("short geophys_%s_%s[%d] = {\n", times[i], events[2], total_frames);
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
		fprintf(stderr, "extern short geophys_%s_%s[%d];\n", times[i], events[2], total_frames);

		total_frames = 0;
		newline = 0;

		printf("int geophys_%s_%s_sizes[%d] = {\n", times[i], events[2], sizes_counter);
		for (int j = 0; j < sizes_counter; j++) {
			if (j == sizes_counter - 1) {
				printf("%6d\n", sizes[j]);
			} else {
				printf("%6d,", sizes[j]);
			}
			if (++newline == 10) {
				printf("\n");
				newline = 0;
			}
		}
		printf("};\n");
		fprintf(stderr, "extern int geophys_%s_%s_sizes[%d];\n", times[i], events[2], sizes_counter);
	}

	for (int i = 0; i < 2; i++) {
		snprintf(file, 64, "../assets/geophys/%s.wav", no_storms[i]);
		if (!(inf = sf_open(file, SFM_READ, &sfinfo))) goto exit;

		frames = sfinfo.frames;
		sizes[i] = frames;

		sf_read_short(inf, audio+total_frames, frames);
		sf_close(inf);

		total_frames += frames;
	}

	newline = 0;

	printf("short geophys_no_storms[%d] = {\n", total_frames);
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

	printf("int geophys_no_storms_sizes[%d] = {\n", 2);
	for (int i = 0; i < 2; i++) {
		if (i == 2 - 1) {
			printf("%6d\n", sizes[i]);
		} else {
			printf("%6d,", sizes[i]);
		}
	}
	printf("};\n");

	fprintf(stderr, "extern short geophys_no_storms[%d];\n", total_frames);
	fprintf(stderr, "extern int geophys_no_storms_sizes[%d];\n", 2);

exit:
	free(audio);

	return 0;
}
