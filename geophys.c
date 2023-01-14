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
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "audio/geophys_ann.h"
#include "audio/geophys_months.h"
#include "audio/geophys_nums_0.h"
#include "audio/geophys_nums_1.h"
#include "audio/geophys_nums_2.h"
#include "audio/geophys_nums_3.h"
#include "audio/geophys_storms.h"
#include "geophys.h"

/* obtain geophysical data via a text file */
void get_geophys_data(struct geophys_data_t *data) {
	int fd;
	static char buf[GEO_DATA_SIZE];

	memset(buf, 0, GEO_DATA_SIZE);

	/*
	 * clear the data here so if parsing fails for some reason we're not
	 * broadcasting old data
	 */
	memset(data, 0, sizeof(struct geophys_data_t));

	if ((fd = open(GEO_DATA_PATH, O_RDONLY)) < 0) return;
	read(fd, buf, GEO_DATA_SIZE - 1);
	close(fd);

	/* parse the file for the data */
	sscanf(buf,
		"%hhu,%hhu,%hu,%hhu,%hhu,%hhu,%hhu,%hu.%hu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
		&data->month_of_prev_day, &data->prev_day,
		&data->solar_flux,
		&data->a_index,
		&data->hour, &data->month_of_cur_day, &data->cur_day,
		&data->k_index_int, &data->k_index_dec,
		&data->obs_space_wx,
		&data->obs_srs,
		&data->obs_radio_blackout,
		&data->pred_space_wx,
		&data->pred_srs,
		&data->pred_radio_blackout
	);
}

void build_geophys_announcement(struct geophys_data_t *data,
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
	for (int i = 0; i <= 31; i++) {
		if (i == data->prev_day) {
			number_len = geophys_nums_0_99_sizes[i];
			memcpy(out_buffer + samples, geophys_nums_0_99 + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_0_99_sizes[i];
	}
	samples += number_len;

	/* the month of yesterday */
	offset = 0;
	for (int i = 0; i <= 12; i++) {
		if (i == data->month_of_prev_day) {
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
	if (data->solar_flux >= 100 && data->solar_flux <= 199) {
		data->solar_flux -= 100;
		for (int i = 0; i < 100; i++) {
			if (i == data->solar_flux) {
				number_len = geophys_nums_100_199_sizes[i];
				memcpy(out_buffer + samples,
					geophys_nums_100_199 + offset,
					number_len * sizeof(*voice));
				break;
			}
			/* continue stepping through the data */
			offset += geophys_nums_100_199_sizes[i];
		}
	} else if (data->solar_flux >= 200 && data->solar_flux <= 299) {
		data->solar_flux -= 200;
		for (int i = 0; i < 100; i++) {
			if (i == data->solar_flux) {
				number_len = geophys_nums_200_299_sizes[i];
				memcpy(out_buffer + samples,
					geophys_nums_200_299 + offset,
					number_len * sizeof(*voice));
				break;
			}
			/* continue stepping through the data */
			offset += geophys_nums_200_299_sizes[i];
		}
	} else if (data->solar_flux >= 300 && data->solar_flux <= 399) {
		data->solar_flux -= 300;
		for (int i = 0; i < 100; i++) {
			if (i == data->solar_flux) {
				number_len = geophys_nums_300_399_sizes[i];
				memcpy(out_buffer + samples,
					geophys_nums_300_399 + offset,
				number_len * sizeof(*voice));
				break;
			}
			/* continue stepping through the data */
			offset += geophys_nums_300_399_sizes[i];
		}
	} else { /* < 100 */
		for (int i = 0; i < 100; i++) {
			if (i == data->solar_flux) {
				number_len = geophys_nums_0_99_sizes[i];
				memcpy(out_buffer + samples,
					geophys_nums_0_99 + offset,
				number_len * sizeof(*voice));
				break;
			}
			/* continue stepping through the data */
			offset += geophys_nums_0_99_sizes[i];
		}
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
	for (int i = 0; i < 100; i++) {
		if (i == data->a_index) {
			number_len = geophys_nums_0_99_sizes[i];
			memcpy(out_buffer + samples, geophys_nums_0_99 + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_0_99_sizes[i];
	}
	samples += number_len;

	/* pause */
	samples += 100*100;

	/* "the estimated planetary K-index at" */
	memcpy(out_buffer + samples,
		geophys_ann + ann_offset,
		geophys_ann_sizes[4] * sizeof(*voice));
	ann_offset += geophys_ann_sizes[4];
	samples += geophys_ann_sizes[4];

	/* current hour */
	offset = 0;
	for (int i = 0; i < 24; i++) {
		if (i == data->hour) {
			number_len = geophys_nums_0_99_sizes[i];
			memcpy(out_buffer + samples, geophys_nums_0_99 + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_0_99_sizes[i];
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
	for (int i = 0; i <= 31; i++) {
		if (i == data->cur_day) {
			number_len = geophys_nums_0_99_sizes[i];
			memcpy(out_buffer + samples, geophys_nums_0_99 + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_0_99_sizes[i];
	}
	samples += number_len;

	/* today's month */
	offset = 0;
	for (int i = 0; i <= 12; i++) {
		if (i == data->month_of_cur_day) {
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
			number_len = geophys_nums_0_99_sizes[i];
			memcpy(out_buffer + samples, geophys_nums_0_99 + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_0_99_sizes[i];
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
			number_len = geophys_nums_0_99_sizes[i];
			memcpy(out_buffer + samples, geophys_nums_0_99 + offset,
				number_len * sizeof(*voice));
			break;
		}
		/* continue stepping through the data */
		offset += geophys_nums_0_99_sizes[i];
	}
	samples += number_len;

	/* pause */
	samples += 100*100;

	if (data->obs_space_wx || data->obs_srs || data->obs_radio_blackout) {
		/* observed space weather */
		if (data->obs_space_wx) {
			offset = 0;
			for (int i = 0; i <= 5; i++) {
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

		/* observed solar radiation storms */
		if (data->obs_srs) {
			offset = 0;
			for (int i = 0; i <= 5; i++) {
				if (i + 1 == data->obs_srs) {
					number_len =
						geophys_obs_srs_sizes[i];
					memcpy(out_buffer + samples,
						geophys_obs_srs + offset,
						number_len * sizeof(*voice));
					break;
				}
				/* continue stepping through the data */
				offset += geophys_obs_srs_sizes[i];
			}
			samples += number_len;
		}

		/* observed radio blackout */
		if (data->obs_radio_blackout) {
			offset = 0;
			for (int i = 0; i <= 5; i++) {
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
		/* "No space weather storms were observed
		 * for the past 24 hours."
		 */
		memcpy(out_buffer + samples,
			geophys_no_storms,
			geophys_no_storms_sizes[0] * sizeof(*voice));
		samples += geophys_no_storms_sizes[0];
	}

	if (data->pred_space_wx || data->pred_srs || data->pred_radio_blackout)
	{
		/* predicted space weather */
		if (data->pred_space_wx) {
			offset = 0;
			for (int i = 0; i <= 5; i++) {
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

		/* predicted solar radiation storms */
		if (data->pred_srs) {
			offset = 0;
			for (int i = 0; i <= 15; i++) {
				if (i + 1 == data->pred_srs) {
					number_len =
						geophys_pred_srs_sizes[i];
					memcpy(out_buffer + samples,
						geophys_pred_srs + offset,
						number_len * sizeof(*voice));
					break;
				}
				/* continue stepping through the data */
				offset += geophys_pred_srs_sizes[i];
			}
			samples += number_len;
		}

		/* predicted radio blackout */
		if (data->pred_radio_blackout) {
			offset = 0;
			for (int i = 0; i <= 10; i++) {
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
		/* "No space weather storms are predicted
		 * for the next 24 hours."
		 */
		memcpy(out_buffer + samples,
			geophys_no_storms + geophys_no_storms_sizes[0],
			geophys_no_storms_sizes[1] * sizeof(*voice));
		samples += geophys_no_storms_sizes[1];
	}

	if (samples > len) samples = len;

	memcpy(voice, out_buffer, samples * sizeof(*voice));
	free(out_buffer);
}
