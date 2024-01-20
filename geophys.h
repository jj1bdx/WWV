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

#define GEO_DATA_PATH	"/tmp/wwv-geophys.data"
#define GEO_DATA_SIZE	64

typedef struct geophys_data_t {
	/* previous day */
	uint8_t month_of_prev_day;
	uint8_t prev_day;

	uint16_t solar_flux;

	uint8_t a_index;

	uint8_t hour;

	uint8_t month_of_cur_day;
	uint8_t cur_day;

	/* decimal parts */
	uint8_t k_index_int;
	uint8_t k_index_dec;

	uint8_t obs_space_wx;
	uint8_t obs_srs;
	uint8_t obs_geomag;
	uint8_t obs_radio_blackout;

	uint8_t pred_space_wx;
	uint8_t pred_srs;
	uint8_t pred_geomag;
	uint8_t pred_radio_blackout;
} geophys_data_t;

extern void get_geophys_data(struct geophys_data_t *data);
extern void build_geophys_announcement(
	struct geophys_data_t *data,
	int len, short *voice);
