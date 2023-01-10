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
#include <stdint.h>

#include "audio/geophys_ann.h"
#include "audio/geophys_months.h"
#include "audio/geophys_nums.h"
#include "audio/geophys_storms.h"
#include "geophys.h"

void build_geophys_announcement(int hour,
	int month_of_prev_day, int month,
	int prev_day, int day,
	struct geophys_data_t *data,
	int len, short *voice) {

	int samples = 0;
	int ann_offset = 0;
	int offset = 0;
	int number_len = 0;

	short *out_buffer;

	/* doubled just in case */
	out_buffer = malloc(16000 * 60 * 2 * sizeof(*voice));

	/* clear out buffer */
	memset(out_buffer, 0, 16000 * 60 * 2 * sizeof(*voice));

	/* "solar-terrestrial indices for" */
	memcpy(out_buffer, geophys_ann, geophys_ann_sizes[0] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[0];
	samples += geophys_ann_sizes[0];

	/* yesterday's day of month */
	offset = 0;
	for (int i = 0; i < 31 + 1; i++) {
		if (i == prev_day) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer + samples, geophys_nums + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_sizes[i];
	}
	samples += number_len;

	/* the month of yesterday */
	offset = 0;
	for (int i = 0; i < 12 + 1; i++) {
		if (i == month_of_prev_day) {
			number_len = geophys_months_sizes[i - 1];
			memcpy(out_buffer + samples, geophys_months + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_months_sizes[i - 1];
	}
	samples += number_len;

	/* "follow" */
	memcpy(out_buffer + samples,
		geophys_ann + ann_offset,
		geophys_ann_sizes[1] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[1];
	samples += geophys_ann_sizes[1];

	/* pause */
	samples += 100*100;

	/* "solar flux" */
	memcpy(out_buffer + samples,
		geophys_ann + ann_offset,
		geophys_ann_sizes[2] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[2];
	samples += geophys_ann_sizes[2];

	/* current solar flux */
	offset = 0;
	for (int i = 0; i < 200; i++) {
		if (i == data->solar_flux) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer + samples, geophys_nums + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_sizes[i];
	}
	samples += number_len;

	/* "and estimated planetary A-index" */
	memcpy(out_buffer + samples,
		geophys_ann + ann_offset,
		geophys_ann_sizes[3] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[3];
	samples += geophys_ann_sizes[3];

	/* current A-index */
	offset = 0;
	for (int i = 0; i < 200; i++) {
		if (i == data->a_index) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer + samples, geophys_nums + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_sizes[i];
	}
	samples += number_len;

	/* "the estimated planetary K-index at" */
	memcpy(out_buffer + samples,
		geophys_ann + ann_offset,
		geophys_ann_sizes[4] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[4];
	samples += geophys_ann_sizes[4];

	/* current hour */
	offset = 0;
	for (int i = 0; i < 24; i++) {
		if (i == hour) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer + samples, geophys_nums + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_sizes[i];
	}
	samples += number_len;

	/* "UTC on" */
	memcpy(out_buffer + samples,
		geophys_ann + ann_offset,
		geophys_ann_sizes[5] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[5];
	samples += geophys_ann_sizes[5];

	/* today's day */
	offset = 0;
	for (int i = 0; i < 31 + 1; i++) {
		if (i == day) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer + samples, geophys_nums + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_sizes[i];
	}
	samples += number_len;

	/* today's month */
	offset = 0;
	for (int i = 0; i < 12 + 1; i++) {
		if (i == month) {
			number_len = geophys_months_sizes[i - 1];
			memcpy(out_buffer + samples, geophys_months + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_months_sizes[i - 1];
	}
	samples += number_len;

	/* "was" */
	memcpy(out_buffer + samples,
		geophys_ann + ann_offset,
		geophys_ann_sizes[6] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[6];
	samples += geophys_ann_sizes[6];

	/* estimated K-index */
	offset = 0;
	for (int i = 0; i < 10; i++) {
		if (i == data->k_index_int) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer + samples, geophys_nums + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_sizes[i];
	}
	samples += number_len;

	/* "point" */
	memcpy(out_buffer + samples,
		 geophys_ann + ann_offset,
		geophys_ann_sizes[7] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[7];
	samples += geophys_ann_sizes[7];

	offset = 0;
	for (int i = 0; i < 100; i++) {
		if (i == data->k_index_dec) {
			number_len = geophys_nums_sizes[i];
			memcpy(out_buffer + samples, geophys_nums + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_sizes[i];
	}
	samples += number_len;

	/* pause */
	samples += 100*100;

	if (data->obs_space_wx || data->obs_srs || data->obs_radio_blackout) {
		/* observed space weather */
		if (data->obs_space_wx) {
			offset = 0;
			for (int i = 0; i < 5 + 1; i++) {
				if (i + 1 == data->obs_space_wx) {
					number_len =
						geophys_obs_space_wx_sizes[i];
					memcpy(out_buffer + samples,
						geophys_obs_space_wx + offset,
						number_len * sizeof(*voice));
					break;
				}
				/* continue stepping through the data */
				offset += geophys_obs_space_wx_sizes[i];
			}
			samples += number_len;
		}

		/* observed radio blackout */
		if (data->obs_radio_blackout) {
			offset = 0;
			for (int i = 0; i < 5 + 1; i++) {
				if (i + 1 == data->obs_radio_blackout) {
					number_len = geophys_obs_rb_sizes[i];
					memcpy(out_buffer + samples,
						geophys_obs_rb + offset,
						number_len * sizeof(*voice));
					break;
				}
				/* continue stepping through the data */
				offset += geophys_obs_rb_sizes[i];
			}
			samples += number_len;
		}
	} else {
		/* "no space weather storms were observed
		 * for the past 24 hours."
		 */
		memcpy(out_buffer + samples,
			geophys_no_storms,
			geophys_no_storms_sizes[0] * sizeof(*voice));
		samples += geophys_no_storms_sizes[0];
	}
	ann_offset += geophys_no_storms_sizes[8];

	if (data->pred_space_wx || data->pred_srs || data->pred_radio_blackout)
	{
		/* predicted space weather */
		if (data->pred_space_wx) {
			offset = 0;
			for (int i = 0; i < 5 + 1; i++) {
				if (i + 1 == data->pred_space_wx) {
					number_len =
						geophys_pred_space_wx_sizes[i];
					memcpy(out_buffer + samples,
						geophys_pred_space_wx + offset,
						number_len * sizeof(*voice));
					break;
				}
				/* continue stepping through the data */
				offset += geophys_pred_space_wx_sizes[i];
			}
			samples += number_len;
		}

		/* predicted radio blackout */
		if (data->pred_radio_blackout) {
			offset = 0;
			for (int i = 0; i < 10 + 1; i++) {
				if (i + 1 == data->pred_radio_blackout) {
					number_len = geophys_pred_rb_sizes[i];
					memcpy(out_buffer + samples,
						geophys_pred_rb + offset,
						number_len * sizeof(*voice));
					break;
				}
				/* continue stepping through the data */
				offset += geophys_pred_rb_sizes[i];
			}
			samples += number_len;
		}
	} else {
		/* "no space weather storms are predicted
		 * for the next 24 hours."
		 */
		memcpy(out_buffer + samples,
			geophys_no_storms + geophys_no_storms_sizes[0],
			geophys_no_storms_sizes[1] * sizeof(*voice));
		samples += geophys_no_storms_sizes[1];
	}
	ann_offset += geophys_no_storms_sizes[1];

	if (samples > len) samples = len;

	memcpy(voice, out_buffer, samples * sizeof(*voice));
	free(out_buffer);
}
