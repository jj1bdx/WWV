/*
 * WWV simulator
 * Copyright (C) 2022 Anthony96922
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
#include <string.h>
#include <stdlib.h>

#include "audio/geophys_ann.h"
#include "audio/geophys_months.h"
#include "audio/geophys_nums.h"

void build_geophys_announcement(int hour,
	int month_of_prev_day, int month,
	int prev_day, int day,
	int solar_flux, int a_index,
	int k_index_int, int k_index_dec,
	int len, short *voice) {

	int samples = 0;
	int geophys_ann_offset = 0;
	int number_offset = 0;
	int number_len = 0;
	int month_offset = 0;
	int month_len = 0;

	short *out_buffer;

	out_buffer = malloc(16000 * 60 * 2 * sizeof(*voice)); /* doubled just in case */

	/* clear out buffer */
	memset(out_buffer, 0, 16000 * 60 * 2 * sizeof(*voice));

	/* "solar-terrestrial indices for" */
	memcpy(out_buffer, geophys_ann, geophys_ann_sizes[0]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[0];
	samples += geophys_ann_sizes[0];

	/* yesterday's day of month */
	number_offset = 0;
	for (int i = 0; i < 31 + 1; i++) {
		if (i == prev_day) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer+samples, geophys_nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += geophys_nums_sizes[i]; /* continue stepping through the data */
	}
	samples += number_len;

	/* the month of yesterday */
	month_offset = 0;
	for (int i = 0; i < 12 + 1; i++) {
		if (i == month_of_prev_day) {
			month_len = geophys_months_sizes[i-1];
			memcpy(out_buffer+samples, geophys_months+month_offset,
				month_len*sizeof(*voice));
			break;
		}
		month_offset += geophys_months_sizes[i-1]; /* continue stepping though the data */
	}
	samples += month_len;

	/* "follow" */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[1]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[1];
	samples += geophys_ann_sizes[1];

	/* pause */
	samples += 100*100;

	/* "solar flux" */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[2]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[2];
	samples += geophys_ann_sizes[2];

	/* current solar flux */
	number_offset = 0;
	for (int i = 0; i < 200; i++) {
		if (i == solar_flux) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer+samples, geophys_nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += geophys_nums_sizes[i]; /* continue stepping through the data */
	}
	samples += number_len;

	/* "and estimated planetary A-index" */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[3]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[3];
	samples += geophys_ann_sizes[3];

	/* current A-index */
	number_offset = 0;
	for (int i = 0; i < 200; i++) {
		if (i == a_index) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer+samples, geophys_nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += geophys_nums_sizes[i]; /* continue stepping through the data */
	}
	samples += number_len;

	/* "the estimated planetary K-index at" */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[4]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[4];
	samples += geophys_ann_sizes[4];

	/* current hour */
	number_offset = 0;
	for (int i = 0; i < 24; i++) {
		if (i == hour) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer+samples, geophys_nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += geophys_nums_sizes[i]; /* continue stepping through the data */
	}
	samples += number_len;

	/* "UTC on" */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[5]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[5];
	samples += geophys_ann_sizes[5];

	/* today's day */
	number_offset = 0;
	for (int i = 0; i < 31 + 1; i++) {
		if (i == day) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer+samples, geophys_nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += geophys_nums_sizes[i]; /* continue stepping through the data */
	}
	samples += number_len;

	/* today's month */
	month_offset = 0;
	for (int i = 0; i < 12 + 1; i++) {
		if (i == month) {
			month_len = geophys_months_sizes[i-1];
			memcpy(out_buffer+samples, geophys_months+month_offset,
				month_len*sizeof(*voice));
			break;
		}
		month_offset += geophys_months_sizes[i-1]; /* continue stepping though the data */
	}
	samples += month_len;

	/* "was" */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[6]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[6];
	samples += geophys_ann_sizes[6];

	/* estimated K-index */
	number_offset = 0;
	for (int i = 0; i < 10; i++) {
	if (i == k_index_int) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer+samples, geophys_nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += geophys_nums_sizes[i]; /* continue stepping through the data */
	}
	samples += number_len;

	/* "point" */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[7]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[7];
	samples += geophys_ann_sizes[7];

	number_offset = 0;
	for (int i = 0; i < 100; i++) {
		if (i == k_index_dec) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer+samples, geophys_nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += geophys_nums_sizes[i]; /* continue stepping through the data */
	}
	samples += number_len;

	/* pause */
	samples += 100*100;

	/* "no space weather storms were observed for the past 24 hours.
	    no space weather storms are predicted for the next 24 hours."
	 */
	memcpy(out_buffer+samples, geophys_ann+geophys_ann_offset, geophys_ann_sizes[8]*sizeof(*voice));
	geophys_ann_offset += geophys_ann_sizes[8];
	samples += geophys_ann_sizes[8];

	if (samples > len) samples = len;

	memcpy(voice, out_buffer, samples*sizeof(*voice));
	free(out_buffer);
}
