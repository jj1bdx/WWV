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

typedef struct geophys_data_t {
	uint16_t solar_flux;
	uint8_t a_index;
	/* decimal parts */
	uint16_t k_index_int;
	uint16_t k_index_dec;

	uint8_t obs_space_wx;
	uint8_t obs_srs;
	uint8_t obs_radio_blackout;

	uint8_t pred_space_wx;
	uint8_t pred_srs;
	uint8_t pred_radio_blackout;
} geophys_data_t;

extern void build_geophys_announcement(
	int hour, int prev_month, int month, int prev_day, int day,
	struct geophys_data_t *data,
	int len, short *voice);
