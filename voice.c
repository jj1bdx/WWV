#include <string.h>
#include <stdlib.h>

#include "audio/time_nums.h"
#include "audio/time_ann.h"

void build_time_announcement(int hour, int minute, int h, int len, short *voice) {
	int samples = 0;
	int number_offset = 0;
	int number_len = 0;
	int time_ann_offset = 0;
	short *out_buffer;

	short *nums;
	int *nums_sizes;
	short *time_ann;
	int *time_ann_sizes;

	if (h) {
		nums = wwvh_nums;
		nums_sizes = wwvh_nums_sizes;
		time_ann = wwvh_time_ann;
		time_ann_sizes = wwvh_time_ann_sizes;
	} else {
		nums = wwv_nums;
		nums_sizes = wwv_nums_sizes;
		time_ann = wwv_time_ann;
		time_ann_sizes = wwv_time_ann_sizes;
	}

	out_buffer = malloc(16000 * 60 * 2 * sizeof(*voice)); // doubled just in case

	// at the tone
	memcpy(out_buffer, time_ann, time_ann_sizes[0]*sizeof(*voice));
	time_ann_offset += time_ann_sizes[0];
	samples += time_ann_sizes[0];

	samples += 100*50;

	// # of hours
	for (int i = 0; i < 24; i++) {
		if (i == hour) {
			number_len = nums_sizes[i];
			memcpy(out_buffer+samples, nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += nums_sizes[i]; // continue stepping through the data
	}
	samples += number_len;

	// hours
	memcpy(out_buffer+samples, time_ann+time_ann_offset, time_ann_sizes[1]*sizeof(*voice));
	time_ann_offset += time_ann_sizes[1];
	samples += time_ann_sizes[1];

	samples += 100*50;

	// # of minutes
	number_offset = 0;
	for (int i = 0; i < 60; i++) {
		if (i == minute) {
			number_len = nums_sizes[i];
			memcpy(out_buffer+samples, nums+number_offset,
				number_len*sizeof(*voice));
			break;
		}
		number_offset += nums_sizes[i]; // continue stepping through the data
	}
	samples += number_len;

	// minutes
	memcpy(out_buffer+samples, time_ann+time_ann_offset, time_ann_sizes[2]*sizeof(*voice));
	time_ann_offset += time_ann_sizes[2];
	samples += time_ann_sizes[2];

	// add pause
	samples += 100*100;

	// coordinated universal time
	memcpy(out_buffer+samples, time_ann+time_ann_offset, time_ann_sizes[3]*sizeof(*voice));
	samples += time_ann_sizes[3];

	if (samples > len) samples = len;

	memcpy(voice, out_buffer, samples*sizeof(*voice));
	free(out_buffer);
}
