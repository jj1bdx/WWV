// $Id: wwvsim.c,v 1.13 2018/11/07 19:24:47 karn Exp $
// WWV/WWVH simulator program. Generates their audio program as closely as possible
// Even supports UT1 offsets and leap second insertion
// Uses espeak synthesizer for speech announcements; needs a lot of work
// By default, uses system time, which should be NTP synchronized
// Time can be manually overridden for testing (announcements, leap seconds and other corner cases)

// July 2017, Phil Karn, KA9Q
// (Can you tell I have too much spare time?)

// Major rewrite 19 Oct 2018:
//   Decode generated timecode in verbose mode
//   When output is terminal, use portaudio to write to sound device with correct (?) timing
//   Factor out timecode, audio generation and textfile synthesis to more manageable functions
//   Changes to tone program tables to match actual WWV/WWVH schedule:
//     no GPS status
//     will probably require changes when oceanic weather goes away at end of Oct 2018

//#define DIRECT 1 // Enable direct on-time output to sound device with portaudio when stdout is a terminal
/*#define ASSERTS*/

#ifdef ASSERTS
#include <assert.h>
#endif
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <memory.h>
#include <sys/time.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>

#ifdef DIRECT
#include <portaudio.h>
#endif

/* for dynamic ut1 diff */
#include <fcntl.h>

#include "voice.h"
#include "geophys.h"
#include "audio/id.h"
#include "audio/mars_ann.h"
#include "audio/wwvh_phone.h"
/* HamSci */
#include "audio/hamsci.h"
/* 3G shutdown announcement (unofficial) */
#include "audio/3g-shutdown.h"

/* workaround for missing pi definition */
#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define SAMPLE_RATE	16000

extern int fileno(FILE *fd);

//char Libdir[] = "/usr/local/share/ka9q-radio";

static int Samprate = SAMPLE_RATE; // Samples per second - try to use this if possible
static int Samprate_ms = SAMPLE_RATE / 1000;      // Samples per millisecond - sampling rates not divisible by 1000 may break
#ifdef DIRECT
static int Direct_mode;      // Writing directly to audio device
#endif
static int WWVH = 0; // WWV by default
static int WWVB = 0;
static int Verbose = 0;

static int Negative_leap_second_pending = 0; // If 1, leap second will be removed at end of June or December, whichever is first
static int Positive_leap_second_pending = 0; // If 1, leap second will be inserted at end of June or December, whichever is first

static int shutdown = 0;

/* super primitive WAVE header */
static unsigned char wav_header[] = {
	// RIFF header
	'R', 'I', 'F', 'F',
	0, 0, 0, 0,
	'W', 'A', 'V', 'E',

	/* format */
	'f', 'm', 't', ' ',
	16, 0, 0, 0, /* chunk size */
	1, 0, /* audio format */
	1, 0, /* number of channels */

	/* sample rate */
	((SAMPLE_RATE & 0x00ff)),
	((SAMPLE_RATE & 0xff00) >> 8),
	0, 0,

	/* bytes per second */
	((SAMPLE_RATE*1*sizeof(short) & 0x00ff)),
	((SAMPLE_RATE*1*sizeof(short) & 0xff00) >> 8),
	0, 0,

	1*sizeof(short), 0, /* sample alignment */
	8*sizeof(short), 0, /* bits per sample */

	/* data */
	'd', 'a', 't', 'a',
	255, 255, 255, 255
};

/* needed to work around implicit declaration */
extern int nanosleep(const struct timespec *req, struct timespec *rem);

/* nanosecond sleep */
static void nsleep(unsigned int ns) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = ns;
	nanosleep(&ts, NULL);
}

static void exit_loop() {
	shutdown = 1;
}

static void wait_for_start() {
	struct tm *utc;
	time_t now;

	/* wait for the end of the minute if already at the 0th second */
	now = time(NULL);
	utc = gmtime(&now);
	if (utc->tm_sec == 0)
		nsleep(1000000);

	while (1) {
		/* check time */
		now = time(NULL);
		utc = gmtime(&now);

		if (utc->tm_sec == 0) break;

		/* wait 1 ms before polling again */
		nsleep(1000);
	}
}

static void get_ut1_diff(int *diff) {
	int fd;
	static char buf[3]; /* read up to 2 chars */

	if ((fd = open("/tmp/wwv-ut1.data", O_RDONLY)) < 0) return;
	if (read(fd, buf, 2) < 1) return;
	close(fd);

	/* parse */
	sscanf(buf, "%d", diff);
	if (*diff < -7 || *diff > 7) *diff = 0;
}

// Is specified year a leap year?
static int is_leap_year(int y) {
	if ((y % 4) != 0)
		return 0; // Ordinary year; example: 2017

	if ((y % 100) != 0)
		return 1; // Examples: 1956, 2004 (i.e., most leap years)

	if ((y % 400) != 0)
		return 0; // Examples: 1900, 2100 (the big exception to the usual rule; non-leap US presidential election years)

	return 1; // Example: 2000 (the exception to the exception)
}

// Applies only to non-leap years; you need special tests for February in leap year
static int Days_in_month[] = { // Index 1 = January, 12 = December
  0,31,28,31,30,31,30,31,31,30,31,30,31
};

// Tone schedules for each minute of the hour for each station
// Special exception: no 440 Hz tone in first hour of UTC day; must be handled ad-hoc
static int WWV_tone_schedule[60] = {
    0,600,440,  0,  0,600,500,600,  0,600, // 3 is nist reserved at wwvh, 4 reserved at wwv; 8-10 storms; 7 undoc wwv
    0,600,500,600,500,600,500,600,  0,600, // 14-15 GPS (no longer used - tones), 16 nist reserved, 18 geoalerts; 11 undoc wwv
  500,600,500,600,500,600,500,600,500,  0, // 29 is silent to protect wwvh id
    0,600,500,600,500,600,500,600,500,600, // 30 is station ID
  500,600,500,  0,  0,  0,  0,  0,  0,  0, // 43-51 is silent period to protect wwvh
    0,  0,  0,600,500,600,500,600,500,  0  // 59 is silent to protect wwvh id; 52 new special at wwvh, not protected by wwv
};

static int WWVH_tone_schedule[60] = {
    0,440,600,  0,  0,500,600,500,  0,  0, // 0 silent to protect wwv id; 3 nist reserved; 4 reserved at wwv; 7 protects undoc wwv; 8-10 protects storms at wwv
    0,500,600,500,  0,  0,  0,  0,  0,  0, // 14-19 is silent period to protect wwv; 11 silent to protect undoc wwv
  600,500,600,500,600,500,600,500,600,  0, // 29 is station ID
    0,500,600,500,600,500,600,500,600,500, // 30 silent to protect wwv id
  600,500,600,500,600,  0,600,  0,  0,  0, // 43-44 GPS (unused-tones); 45 geoalerts; 47 nist reserved; 48-51 storms
    0,  0,  0,500,600,500,600,500,600,  0  // 59 is station ID; 52 new special at wwvh?, NOT protected at WWV
};


// Generate complex phasor with specified angle in radians
// Used for tone generation
static complex double csincos(double x) {
	return cos(x) + I*sin(x);
}

#if 0
// Insert PCM audio file into audio output at specified offset
static int announce_audio_file(int16_t *output, char *file, int startms) {
	if (startms < 0 || startms >= 61000)
		return -1;

	int r = -1;
	FILE *fp;

	if ((fp = fopen(file,"r")) != NULL) {
		r = fread(output+startms*Samprate_ms,sizeof(*output),Samprate_ms*(61000-startms),fp);
		fclose(fp);
	}
	return r;
}

// Synthesize speech from a text file and insert into audio output at specified offset
// Use female = 1 for WWVH, 0 for WWV
static int announce_text_file(int16_t *output,char *file, int startms, int female) {
	int r = -1;
	char *fullname = NULL;
	char *command = NULL;

	char tempfile_raw[L_tmpnam];
	strncpy(tempfile_raw,"/tmp/speakXXXXXXXXXX.raw",sizeof(tempfile_raw));
	mkstemps(tempfile_raw,4);

#ifdef __APPLE__
	char tempfile_wav[L_tmpnam];
	strncpy(tempfile_wav,"/tmp/speakXXXXXXXXXX.wav",sizeof(tempfile_wav));
	mkstemps(tempfile_wav,4);
#endif

	int asr = -1;
	if (file[0] == '/')
		asr = asprintf(&fullname,"%s",file); // Leading slash indicates absolute path name
	else
		asr = asprintf(&fullname,"%s/%s",Libdir,file); // Otherwise relative to library directory

	if (asr == -1 || !fullname)
		goto done; // asprintf failed for some reason

	if (access(fullname,R_OK) != 0)
		goto done; // file isn't readable (what if it's a directory?

	char *voice = NULL;
	asr = -1;

#ifdef __APPLE__
	voice = female ? "Samantha" : "Alex";
	asr = asprintf(&command,"say -v %s --output-file=%s --data-format=LEI16@48000 -f %s; sox %s -t raw -r 48000 -c 1 -b 16 -e signed-integer %s",
		voice,tempfile_wav,fullname,tempfile_wav,tempfile_raw);

#else // linux
	voice = female ? "en-us+f3" : "en-us";
	asr = asprintf(&command,"espeak -v %s -a 70 -f %s --stdout | sox -t wav - -t raw -r 48000 -c 1 -b 16 -e signed-integer %s",
		voice,fullname,tempfile_raw);
#endif
	if (asr == -1 || !command)
		goto done; // asprintf failed somehow

	system(command);

	r = announce_audio_file(output,tempfile_raw, startms);

done: // Go here directly on errors
	// Clean up
	unlink(tempfile_raw);
#ifdef __APPLE__
	unlink(tempfile_wav);
#endif

	if (command)
		free(command);

	if (fullname)
		free(fullname);

	return r;
}

// Synthesize a text announcement and insert into output buffer
static int announce_text(int16_t *output,char const *message,int startms,int female) {
	char tempfile_txt[L_tmpnam];
	strncpy(tempfile_txt,"/tmp/speakXXXXXXXXXX.txt",sizeof(tempfile_txt));
	mkstemps(tempfile_txt,4);

	FILE *fp;
	if ((fp = fopen(tempfile_txt,"w")) == NULL)
		return -1;

	fputs(message,fp);
	fclose(fp);
	int r = announce_text_file(output,tempfile_txt,startms,female);
	unlink(tempfile_txt);
	return r;
}
#endif

// Time announcement
static int announce_time(int16_t *audio, int startms, int stopms, int next_hour, int next_minute, int female) {
	if (startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
		return -1;

	build_time_announcement(next_hour, next_minute, female,
		(stopms - startms)*Samprate_ms, audio + startms*Samprate_ms);

	return 0;
}

// Station ID
static int announce_station(int16_t *audio, int startms, int stopms, int wwvh) {
	if (startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
		return -1;

	int max_len = (stopms - startms)*Samprate_ms;
	int samples = id_sizes[wwvh];
	if (samples > max_len) samples = max_len;
	short *this_id = wwvh ? id + id_sizes[0]: id;

	memcpy(audio + startms*Samprate_ms, this_id, samples*sizeof(*audio));
	return 0;
}

#if 0
// DoD M.A.R.S. (no actual messages yet)
static int announce_mars(int16_t *audio, int startms, int stopms, int wwvh) {
	if (startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
		return -1;

	int max_len = (stopms - startms)*Samprate_ms;
	int samples = mars_ann_sizes[wwvh];
	if (samples > max_len) samples = max_len;
	short *this_mars_ann = wwvh ? mars_ann + mars_ann_sizes[0] : mars_ann;

	memcpy(audio + startms*Samprate_ms, this_mars_ann, samples*sizeof(*audio));
	return 0;
}
#endif

// WWVH only: announce WWVH broadcast availability over the phone
static int announce_phone(int16_t *audio, int startms, int stopms) {
	if (startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
		return -1;

	int max_len = (stopms - startms)*Samprate_ms;
	int samples = wwvh_phone_size;
	if (samples > max_len) samples = max_len;

	memcpy(audio + startms*Samprate_ms, wwvh_phone, samples*sizeof(*audio));
	return 0;
}

/* Geophysical report: WWV/H */
static int announce_geophys(int16_t *audio, int startms, int stopms,
	struct geophys_data_t *data) {
	if (startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
		return -1;

	build_geophys_announcement(data,
		(stopms - startms)*Samprate_ms, audio + startms*Samprate_ms
	);

	return 0;
}

/* HamSci (announcement and test) */
static int announce_hamsci(int16_t *audio, int startms, int stopms, int wwvh, int test) {
	if (startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
		return -1;

	int max_len = (stopms - startms)*Samprate_ms;
	int ann_offset = wwvh ? hamsci_ann_sizes[0] : 0;
	int samples = test ? hamsci_test_size : hamsci_ann_sizes[wwvh];
	if (samples > max_len) samples = max_len;

	memcpy(audio + startms*Samprate_ms,
		test ? hamsci_test : hamsci_ann + ann_offset, samples*sizeof(*audio));
	return 0;
}

#if 0
/* Sprint LTE and T-Mobile UMTS shutdown announcement: WWV/H */
static int announce_3g_shutdown(int16_t *audio, int startms, int stopms, int wwvh) {
	if (startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
		return -1;

	int max_len = (stopms - startms)*Samprate_ms;
	int samples = _3g_shutdown_ann_sizes[wwvh];
	if (samples > max_len) samples = max_len;
	short *this_3g_ann = wwvh ? _3g_shutdown_ann + _3g_shutdown_ann_sizes[0] : _3g_shutdown_ann;

	memcpy(audio + startms*Samprate_ms, this_3g_ann, samples*sizeof(*audio));
	return 0;
}
#endif

// Overlay a tone with frequency 'freq' in audio buffer, overwriting whatever was there
// starting at 'startms' within the minute and stopping one sample before 'stopms'.
// Amplitude 1.0 is 100% modulation, 0.5 is 50% modulation, etc
// Used first for 500/600 Hz continuous audio tones
// Then used for 1000/1200 Hz minute/hour beeps and second ticks, which pre-empt everything else.
static int overlay_tone(int16_t *output,int startms,int stopms,float freq,float amp) {
	if (startms < 0 || stopms <= startms || stopms > 61000)
	return -1;

#ifdef ASSERTS
	assert((startms * (int)freq % 1000) == 0); // All tones start with a positive zero crossing?
#endif

	complex double phase = 1;
	complex double const phase_step = csincos(2*M_PI*freq/Samprate);
	output += startms*Samprate_ms;
	int samples = (stopms - startms)*Samprate_ms;
	while (samples-- > 0) {
		*output++ = cimag(phase) * amp * SHRT_MAX; // imaginary component is sine, real is cosine
		phase *= phase_step;  // Rotate the tone phasor
	}

	return 0;
}

// Same as overlay_tone() except that the tone is added to whatever is already in the audio buffer
// Take care to avoid overmodulation; the result will be clipped but could still sound bad
// Used mainly for 100 Hz subcarrier
static int add_tone(int16_t *output,int startms,int stopms,float freq,float amp) {
	if (startms < 0 || stopms <= startms || stopms > 61000)
		return -1;

#ifdef ASSERTS
	assert((startms * (int)freq % 1000) == 0); // All tones start with a positive zero crossing?
#endif

	complex double phase = 1;
	complex double const phase_step = csincos(2*M_PI*freq/Samprate);
	output += startms*Samprate_ms;
	int samples = (stopms - startms)*Samprate_ms;
	while (samples-- > 0) {
		// Add and clip
		float const samp = *output + cimag(phase) * amp * SHRT_MAX;
		*output++ = samp > 32767 ? 32767 : samp < -32767 ? -32767 : samp;
		phase *= phase_step; // Rotate the tone phasor
	}

	return 0;
}

// Blank out whatever is in the audio buffer starting at startms and ending just before stopms
// Used mainly to blank out 40 ms guard interval around seconds ticks
static int overlay_silence(int16_t *output,int startms,int stopms) {
	if (startms < 0 || stopms <= startms || stopms > 61000)
		return -1;

	output += startms*Samprate_ms;
	int samples = (stopms - startms)*Samprate_ms;

	while (samples-- > 0)
		*output++ = 0;

	return 0;
}

// Encode a BCD digit in little-endian format (lsb first)
// NB! Only WWV/WWVH; WWVB uses big-endian format
static void encode(unsigned char *code,int x) {
	for (int i=0;i<4;i++) {
		code[i] = x & 1;
		x >>= 1;
	}
}
/* WWVB */
static void encode_be(unsigned char *code,int x) {
	for (int i=4;i>0;i--) {
		code[i] = x & 1;
		x >>= 1;
	}
}
static int decode(unsigned char *code) {
	int r = 0;
	for (int i=3; i>=0; i--) {
		r <<= 1;
#ifdef ASSERTS
		assert(code[i] == 0 || code[i] == 1);
#endif
		r += code[i];
	}

	return r;
}

/* Determine day of year when daylight savings time starts
   Only US rules are needed, since WWV/WWVH are American stations
   US rules last changed in 2007 to 2nd sunday of March to first sunday in November
   Always lasts for 238 days (34 weeks)
   Pattern repeats every 28 years (7 days in week x 4 years in leap year cycle)
   Hopefully DST will be abolished before long!
                                          2007: 3/11 (70)    2008: 3/9  (69)
   2009: 3/8  (67)     2010: 3/14 (73)    2011: 3/13 (72)    2012: 3/11 (71)
   2013: 3/10 (69)     2014: 3/9  (68)    2015: 3/8  (67)    2016: 3/13 (73)
   2017: 3/12 (71)     2018: 3/11 (70)    2019: 3/10 (69)    2020: 3/8  (68)
   2021: 3/14 (73)     2022: 3/13 (72)    2023: 3/12 (71)    2024: 3/10 (70)
   2025: 3/9  (68)     2026: 3/8  (67)    2027: 3/14 (73)    2028: 3/12 (72)
   2029: 3/11 (70)     2030: 3/10 (69)    2031: 3/9  (68)    2032: 3/14 (74)

   2033: 3/13 (72)     2034: 3/12 (71)    2035: 3/11 (70)    2036: 3/9  (69)
   2037: 3/8  (67)     2038: 3/14 (73)    2039: 3/13 (72)    2040: 3/11 (71)
   2041: 3/10 (69)     2042: 3/9  (68)    2043: 3/8  (67)    2044: 3/13 (73)
   2045: 3/12 (71)     2046: 3/11 (70)    2047: 3/10 (69)    2048: 3/8  (68)
   2049: 3/14 (73)     2050: 3/13 (72)    2051: 3/12 (71)    2052: 3/10 (70)
   2053: 3/9  (68)     2054: 3/8  (67)    2055: 3/14 (73)    2056: 3/12 (72)
   2057: 3/11 (70)     2058: 3/10 (69)    2059: 3/9  (68)    2060: 3/14 (74)
*/
static int dst_start_doy(int year) {
	int r = -1;
	if (year >= 2007) {
		r = 72;  // DST would have started on day 72 in year 2005 if rule had been in effect then
		for (int ytmp = 2005; ytmp < year; ytmp++) {
			r -= 1 + is_leap_year(ytmp);
			if (r < 67) // Never before day 67
				r += 7;
		}
		if (r == 67 && is_leap_year(year)) // day 67 is 1st sunday in march
			r += 7;
	}
	return r;
}

static int get_days_in_month(int year, int month) {
	if (is_leap_year(year) && month == 2)
		return 29;

	return Days_in_month[month];
}

static int day_of_year(int year,int month,int day) {
	// Compute day of year
	// don't use doy in tm struct in case date was manually overridden
	// (Bug found and reported by Jayson Smith jaybird@bluegrasspals.com)
	int doy = day;
	for (int i = 1; i < month; i++) {
		doy += get_days_in_month(year, month);
	}
	return doy;
}




// Construct time code as array of **61** unsigned chars with values 0 or 1
static void maketimecode(unsigned char *code,int dut1,int leap_pending,int year,int month,int day,int hour,int minute) {
	memset(code,0,61*sizeof(*code)); // All bits default to 0

	int doy = day_of_year(year,month,day);
	int dst_start = dst_start_doy(year);

	if (dst_start >= 1) {
		// DST always lasts for 238 days
		if (doy > dst_start && doy <= dst_start + 238)
			code[2] = 1; // DST status at 00:00 UTC

		if (doy >= dst_start && doy < dst_start + 238)
			code[55] = 1; // DST status at 24:00 UTC
#if 0
		fprintf(stderr,"year %d month %d day %d doy %d dst_start_doy %d dst_start_doy + 238 %d\n",
			year, month, day, doy, dst_start, dst_start + 238);
#endif
	}

	code[3] = leap_pending;

	// Year
	encode(code+4,year % 10); // Least significant digit
	encode(code+51,(year/10)%10); // Tens digit

	// Minute of hour, 0-59
	encode(code+10,minute%10); // Least significant digit
	encode(code+15,minute/10); // Most significant digit, extends into unused bit 18

	// Hour of day, 0-23
	encode(code+20,hour%10);   // Least significant digit
	encode(code+25,hour/10);   // Most significant digit, extends into unused bits 27-28

	// Day of year, 1-366
	encode(code+30,doy%10);    // Least significant digit
	encode(code+35,(doy/10)%10); // Middle digit
	encode(code+40,doy/100);   // High digit, extends into unused bits 42-43

	// UT1 offset, +/-0.0 through 0.7; adjusted after leap second
	code[50] = (dut1 >= 0); // sign
	encode(code+56,abs(dut1));  // magnitude, extends into marker 59 and is ignored
}

static void makebetimecode(unsigned char *code,int dut1,int leap_pending,int year,int month,int day,int hour,int minute) {
	memset(code,0,61*sizeof(*code)); // All bits default to 0

	int doy = day_of_year(year,month,day);
	int dst_start = dst_start_doy(year);

	if (dst_start >= 1) {
		// DST always lasts for 238 days
		if (doy > dst_start && doy <= dst_start + 238)
			code[57] = 1; // DST status at 00:00 UTC

		if (doy >= dst_start && doy < dst_start + 238)
			code[58] = 1; // DST status at 24:00 UTC
#if 0
		fprintf(stderr,"year %d month %d day %d doy %d dst_start_doy %d dst_start_doy + 238 %d\n",
			year, month, day, doy, dst_start, dst_start + 238);
#endif
	}

	// Minute of hour, 0-59
	encode_be(code+1,minute/10); // Most significant digit, extends into unused bit 18
	encode_be(code+5,minute%10); // Least significant digit

	// Hour of day, 0-23
	encode_be(code+12,hour/10);   // Most significant digit, extends into unused bits 27-28
	encode_be(code+15,hour%10);   // Least significant digit

	// Day of year, 1-366
	encode_be(code+22,doy/100);   // High digit, extends into unused bits 42-43
	encode_be(code+25,(doy/10)%10); // Middle digit
	encode_be(code+30,doy%10);    // Least significant digit

	// UT1 offset, +/-0.0 through 0.7; adjusted after leap second
	code[36] = code[38] = (dut1 >= 0); // sign
	code[37] = (dut1 < 0);
	encode_be(code+40,abs(dut1));  // magnitude, extends into marker 59 and is ignored

	// Year
	encode_be(code+45,(year/10)%10); // Tens digit
	encode_be(code+50,year % 10); // Least significant digit

	code[55] = is_leap_year(year);
	code[56] = leap_pending;
}

// Decode frame of timecode to stderr for debugging
static void decode_timecode(unsigned char *code,int length) {
	for (int s=0;s<length;s++) {
		if ((s % 10) == 0 && s < 60)
			fprintf(stderr,"%02d: ",s);
		if (s == 0)
			fputc(' ',stderr);
		else if ((s % 10) == 9)
			fprintf(stderr,"M");
		else
			fputc(code[s] ? '1' : '0',stderr);

		if (s < 59 && (s % 10 == 9))
			fputc('\n',stderr);
	}

	fputc('\n',stderr);
	fprintf(stderr,"year %d%d",decode(code+51),decode(code+4));
	fprintf(stderr," doy %d%d%d",decode(code+40),decode(code+35),decode(code+30));

	fprintf(stderr," hour %d%d",decode(code+25),decode(code+20));
	fprintf(stderr," minute %d%d",decode(code+15),decode(code+10));
	int dut1 = decode(code+56);
	if (!code[50])
		dut1 = -dut1;

	fprintf(stderr,"; dut1 %+d",dut1);

	if (code[3])
		fprintf(stderr,"; leap second pending");

	if (code[2] && code[55])
		fprintf(stderr,"; DST in effect");
	else if (!code[2] && code[55])
		fprintf(stderr,"; DST starts today");
	else if (code[2]  && !code[55])
		fprintf(stderr,"; DST ends today");
	else
		fprintf(stderr,"; DST not in effect");

	fprintf(stderr,"\n\n");
}

// Insert tone or announcement into seconds 1-44
static void gen_tone_or_announcement(int16_t *output,int wwvh,int hour,int minute,
	struct geophys_data_t *geophys_data) {
	const double tone_amp = pow(10.,-6.0/20.); // -6 dB

#if 0
	// A raw audio file pre-empts everything else
	char *rawfilename = NULL;
	char *textfilename = NULL;

	if (asprintf(&rawfilename,"%s/%s/%d.raw",Libdir,wwvh ? "wwvh" : "wwv",minute)
		&& access(rawfilename,R_OK) == 0) {
		announce_audio_file(output,rawfilename,1000);
		goto done;
	} else if (asprintf(&textfilename,"%s/%s/%d.txt",Libdir,wwvh ? "wwvh" : "wwv",minute)
		&& access(textfilename,R_OK) == 0) {
		announce_text_file(output,textfilename,1000,wwvh);
		goto done;
	} else {
		// Otherwise generate a tone, unless silent
		double tone = wwvh ? WWVH_tone_schedule[minute] : WWV_tone_schedule[minute];

		// Special case: no 440 Hz tone during hour 0
		if (tone == 440 && hour == 0)
			tone = 0;

		if (tone)
			add_tone(output,1000,45000,tone,tone_amp); // Continuous tone from 1 sec until 45 sec
	}

done:
	if (rawfilename)
		free(rawfilename);

	if (textfilename)
		free(textfilename);
#else
	// Continuous tone (or silence) from start of second 1 through end of second 44
	int tone = wwvh ? WWVH_tone_schedule[minute] : WWV_tone_schedule[minute];

	// Special case: no 440 Hz tone during hour 0
	if (tone == 440 && hour == 0)
		tone = 0;

	// WWVH IDs at minute 29 and 59 in female voice
	if (wwvh && (minute == 59 || minute == 29)) {
		announce_station(output,1000,45000,1);
	// WWV IDs at minute 0 and 30 in male voice
	} else if (!wwvh && (minute == 0 || minute == 30)) {
		announce_station(output,1000,45000,0);

#if 0 /* no broadcasts? */
	/* DoD M.A.R.S. announcement on minute 10 */
	} else if (!wwvh && (minute == 10)) {
		announce_mars(output, 2000, 45000, 0);
	/* ...and on minute 50 */
	} else if (wwvh && (minute == 50)) {
		announce_mars(output, 2000, 45000, 1);
#endif

	/* dial-in information broadcast on WWVH only */
	} else if (wwvh && (minute == 47 || minute == 52)) {
		announce_phone(output,1000, 45000);

	/* geophysical alerts on minute 18 */
	} else if (!wwvh && minute == 18) {
		announce_geophys(output, 2500, 45000, geophys_data);
	/* ... and on minute 45 */
	} else if (wwvh && minute == 45) {
		announce_geophys(output, 2500, 45000, geophys_data);

	/* HamSci */
	} else if (!wwvh && minute == 4) {
		announce_hamsci(output, 1000, 45000, 0, 0);
	} else if (wwvh && minute == 3) {
		announce_hamsci(output, 1000, 45000, 1, 0);
	} else if (!wwvh && minute == 8) {
		announce_hamsci(output, 1000, 45000, 0, 1);
	} else if (wwvh && minute == 48) {
		announce_hamsci(output, 1000, 45000, 1, 1);

#if 0 /* will be removed soon */
	/* Sprint LTE and T-Mobile UMTS shutdown announcement (unofficial) */
	} else if (!wwvh && (minute == 14 || minute == 44)) {
		announce_3g_shutdown(output, 2000, 45000, 0);
	} else if (wwvh && (minute == 16 || minute == 46)) {
		announce_3g_shutdown(output, 2000, 45000, 1);
#endif

	} else {
		if (tone)
			add_tone(output,1000,45000,tone,tone_amp); // Continuous tone from 1 sec until 45 sec
	}
#endif
}



static void makeminute(int16_t *output,int length,int wwvh,unsigned char *code,int dut1,int hour,int minute,
	struct geophys_data_t *geophys_data) {
	// Amplitudes
	// NIST 250-67, p 50
	const double marker_high_amp = pow(10.,-6.0/20.);
	//  NIST 250-67, p 47 says 1/3.3 (about -10 dB) but is apparently incorrect; observed is ~ -20 dB
	//  const double marker_low_amp = marker_high_amp / 3.3;
	const double marker_low_amp = marker_high_amp / 10;
	const double tick_amp = 1.0; // 100%, 0dBFS

	const double tickfreq = wwvh ? 1200.0 : 1000.0;
	const double hourbeep = 1500.0; // Both WWV and WWVH

	/* this is updated hourly */
	if (minute == 15)
		get_geophys_data(geophys_data);

	// Build a minute of audio
	memset(output,0,(length+1)*Samprate*sizeof(*output)); // Clear previous audio
	gen_tone_or_announcement(output,wwvh,hour,minute,
		geophys_data);

	// Insert minute announcement
	int nextminute,nexthour; // What are the next hour and minute?
	nextminute = minute;
	nexthour = hour;
	if (++nextminute == 60) {
		nextminute = 0;
		if (++nexthour == 24)
			nexthour = 0;
	}
#if 0
	char *message = NULL;
	int asr = -1;
	asr = asprintf(&message,"At the tone, %d %s %d %s Coordinated Universal Time",
		nexthour,nexthour == 1 ? "hour" : "hours",
		nextminute,nextminute == 1 ? "minute" : "minutes");

	if (asr != -1 && message) {
		if (!wwvh)
			announce_text(output,message,52500,0); // WWV: male voice at 52.5 seconds
		else
			announce_text(output,message,45000,1); // WWVH: female voice at 45 seconds

		free(message);
		message = NULL;
	}
#else
	if (!wwvh)
		announce_time(output, 52500, 60000, nexthour, nextminute, 0); // WWV: male voice at 52.5 seconds
	else
		announce_time(output, 45000, 52500, nexthour, nextminute, 1); // WWVH: female voice at 45 seconds
#endif

	// Modulate time code onto 100 Hz subcarrier
	int s;
	for (s=1; s<length; s++) { // No subcarrier during second 0 (minute/hour beep)
		if ((s % 10) == 9) {
			add_tone(output,s*1000,s*1000+800,100,marker_high_amp);	 // 800 ms position markers on seconds 9, 19, 29, ...
			add_tone(output,s*1000+800,s*1000+1000,100,marker_low_amp);
		} else if (code[s]) {
			add_tone(output,s*1000,s*1000+500,100,marker_high_amp);	 // 500 ms = 1 bit
			add_tone(output,s*1000+500,s*1000+1000,100,marker_low_amp);
		} else {
			add_tone(output,s*1000,s*1000+200,100,marker_high_amp);	 // 200 ms = 0 bit
			add_tone(output,s*1000+200,s*1000+1000,100,marker_low_amp);
		}
	}

	// Pre-empt with minute/hour beep and guard interval
	overlay_tone(output,0,800,minute == 0 ? hourbeep : tickfreq,tick_amp);
	overlay_silence(output,800,1000);

	// Pre-empt with second ticks and guard interval
	for (s=1; s<length; s++) {
		if (s != 29 && s < 59) {
			// No ticks or blanking on 29, 59 or 60
			// Blank with silence from t-10 ms to t+30, total 40 ms
			overlay_silence(output,1000*s-10,1000*s+30);
			overlay_tone(output,1000*s,1000*s+5,tickfreq,tick_amp); // 5 ms tick at 100% modulation on second
		}

		// Double ticks without guard time for UT1 offset
		if ((dut1 > 0 && s >= 1 && s <= dut1)
			|| (-dut1 > 0 && s >= 9 && s <= 8-dut1)) {
			overlay_tone(output,1000*s+100,1000*s+105,tickfreq,tick_amp); // 5 ms second tick at 100 ms
		}
	}
}

static void makewwvbminute(int16_t *output,int length,unsigned char *code) {
	// Amplitudes
	// NIST 250-67, p 50
	const double marker_high_amp = 1.0;
	//  NIST 250-67, p 47 says 1/3.3 (about -10 dB) but is apparently incorrect; observed is ~ -20 dB
	//  const double marker_low_amp = marker_high_amp / 3.3;
	const double marker_low_amp = pow(10.,-17.0/20.); // -17 dB;

	// Build a minute of audio
	memset(output,0,(length+1)*Samprate*sizeof(*output)); // Clear previous audio

	// Modulate time code onto 100 Hz subcarrier
	add_tone(output,0,800,100,marker_low_amp);
	add_tone(output,800,1000,100,marker_high_amp);
	int s;
	for (s=1; s<length; s++) {
		if ((s % 10) == 9) {
			add_tone(output,s*1000,s*1000+800,100,marker_low_amp);  // 800 ms position markers on seconds 9, 19, 29, ...
			add_tone(output,s*1000+800,s*1000+1000,100,marker_high_amp);
		} else if (code[s]) {
			add_tone(output,s*1000,s*1000+500,100,marker_low_amp);  // 500 ms = 1 bit
			add_tone(output,s*1000+500,s*1000+1000,100,marker_high_amp);
		} else {
			add_tone(output,s*1000,s*1000+200,100,marker_low_amp);  // 200 ms = 0 bit
			add_tone(output,s*1000+200,s*1000+1000,100,marker_high_amp);
		}
	}
}

#ifdef DIRECT

volatile int Odd_minute_length = 60; // 59 or 61 if leap second pending at end of current odd minute
volatile PaTime Buffer_start_time; // Portaudio time at start of buffer (even minute boundary)
PaStream *Stream;
volatile int Buffers;
// Portaudio callback
static int pa_callback(const void *inputBuffer, void *outputBuffer,
		       unsigned long framesPerBuffer,
		       const PaStreamCallbackTimeInfo* timeInfo,
		       PaStreamCallbackFlags statusFlags,
		       void *userData) {
	if (!outputBuffer)
		return paAbort; // can this happen??

	// use portaudio time to figure out from where to send
	int16_t *rdptr = Audio_buffer + (int)(Samprate * (timeInfo->outputBufferDacTime - Buffer_start_time));
	int16_t *out = outputBuffer;
	int in_low_half = rdptr < Audio_buffer + (60 * Samprate);

	while (framesPerBuffer--) {
		if (!in_low_half && rdptr >= Audio_buffer + (60 + Odd_minute_length) * Samprate) {
			// Wrapped back to beginning
			in_low_half = 1;
			rdptr -= (60 + Odd_minute_length) * Samprate;
			Buffer_start_time += 60.0 + Odd_minute_length;
			Odd_minute_length = 60; // Reset to normal after possible leap second
			Buffers--;
		} else if (in_low_half && rdptr >= Audio_buffer + 60 * Samprate) {
			// Passed halfway mark
			in_low_half = 0;
			Buffers--;
		}
		*out++ = *rdptr++;
	}
	return paContinue;
}

#endif

int main(int argc,char *argv[]) {
	int c;
	int year,month,day,hour,minute;
	int dut1 = 0;
	int manual_time = 0;
	int samplenum;
	int delaysamplenum;
	int sync_delay;
	int leadlag;
	struct timeval synctv;
#ifdef DIRECT
	double fsec;
	int devnum = -1;
#endif

	// Address of malloc'ed audio output buffer, 2 minutes + 1 second long (in case of leap second)
	int16_t *Audio_buffer;

	// Use current computer clock time as default
	time_t now;
	struct tm *utc;
#ifdef DIRECT
	fsec = 0.0;
#endif

	struct geophys_data_t geophys_data;

#if 0
	for (int y=2007;y < 2100;y++) {
		fprintf(stderr,"year %d dst start %d\n",y,dst_start_doy(y));
	}
#endif

	// Read and process command line arguments
	while ((c = getopt(argc,argv,"BHY:M:D:h:m:s:u:r:LNvn:")) != EOF) {
		switch(c) {
			case 'n':
#ifdef DIRECT
				devnum = strtol(optarg,NULL,0);
#endif
				break;
			case 'v':
				Verbose++;
				break;
			case 'r':
				fprintf(stderr, "Sample rate cannot be changed\n");
				//Samprate = strtol(optarg,NULL,0); // Try not to change this, may not work
				break;
			case 'B': /* WWVB mode */
				WWVB++;
				break;
			case 'H': // Simulate WWVH, otherwise WWV
				WWVH++;
				break;
			case 'u': // UT1 offset in tenths of a second, +/- 7
				dut1 = strtol(optarg,NULL,0);
				break;
			case 'Y': // Manual year setting
				year = strtol(optarg,NULL,0);
				manual_time++;
				break;
			case 'M': // Manual month setting
				month = strtol(optarg,NULL,0);
				manual_time++;
				break;
			case 'D': // Manual day setting
				day = strtol(optarg,NULL,0);
				manual_time++;
				break;
			case 'h': // Manual hour setting
				hour = strtol(optarg,NULL,0);
				manual_time++;
				break;
			case 'm': // Manual minute setting
				minute = strtol(optarg,NULL,0);
				manual_time++;
				break;
			case 's': // Manual second setting
				//sec = strtol(optarg,NULL,0);
#ifdef DIRECT
				fsec = 0;
#endif
				manual_time++;
				break;
			case 'L':
				Positive_leap_second_pending++; // Positive leap second at end of current month
				break;
			case 'N':
				Negative_leap_second_pending++; // Leap second at end of current month
				break;
			case '?':
				fprintf(stderr,"Usage: %s [-v] [-r samprate] [-H] [-u ut1offset] [-Y year] [-M month] [-D day] [-h hour] [-m min] [-s sec] [-L|-N]\n",argv[0]);
				fprintf(stderr,"Default sample rate: %d kHz\n", Samprate_ms);
				fprintf(stderr,"By default uses current system time; Use -Y/-M/-D/-h/-m/-s to override for testing, e.g., of leap seconds\n");
				fprintf(stderr,"-v turns on verbose reporting. -H selects the WWVH format; default is WWV\n");
				fprintf(stderr,"-u specifies current UT1-UTC offset in tenths of a second, must be between -7 and +7\n");
				fprintf(stderr,"-L introduces a positive leap second at the end of June or December, whichever comes first\n");
				fprintf(stderr,"-N introduces a negative leap second at the end of June or December, whichever comes first. Only one of -L and -N can be given\n");
				exit(1);
		}
	}

	signal(SIGINT, exit_loop);
	signal(SIGTERM, exit_loop);

	if (isatty(fileno(stdout))) {
#ifdef DIRECT
		// No output redirection, so use portaudio to write directly to audio hardware with "precise" (?) timing
		Direct_mode = 1;

		Pa_Initialize();
		PaDeviceIndex dev = Pa_GetDefaultOutputDevice();
		if (devnum != -1)
			dev = devnum;

		PaStreamParameters param;
		param.device = dev;
		param.channelCount = 1;
		param.sampleFormat = paInt16;
		param.suggestedLatency = .01;
		param.hostApiSpecificStreamInfo = NULL;

		//    Pa_OpenDefaultStream(&Stream,0,1,paInt16,(double)Samprate,48000,pa_callback,NULL); // one whole second?
		Pa_OpenStream(&Stream,NULL,&param,(double)Samprate,48000,0,pa_callback,NULL);
		// How about execution latency between gettimeofday() call and here?
		Buffer_start_time = Pa_GetStreamTime(Stream) - (fsec + sec + ((minute & 1) ? 60 : 0));
		Pa_StartStream(Stream);
#else
		fprintf(stderr,"Won't send PCM to a terminal (direct mode not compiled in)\n");
		exit(1);
#endif

	}

	// Allocate an audio buffer >= 2 minutes + 2 seconds long
	// Even minutes will use first half, odd minutes second half
	Audio_buffer = malloc(2*Samprate*62*sizeof(int16_t));
	memset(Audio_buffer,0,2*Samprate*62*sizeof(int16_t));

	//if (year < 2007)
	//	fprintf(stderr,"Warning: DST rules prior to %d not implemented; DST bits = 0\n",year);    // Punt

	if (Positive_leap_second_pending && Negative_leap_second_pending) {
		fprintf(stderr,"Positive and negative leap seconds can't both be pending! Both cancelled\n");
		Positive_leap_second_pending = Negative_leap_second_pending = 0;
	}

	if (dut1 > 7 || dut1 < -7) {
		fprintf(stderr,"ut1 offset %d out of range, limited to -7 to +7 tenths\n",dut1);
		dut1 = 0;
	}
	if (Positive_leap_second_pending && dut1 > -3) {
		fprintf(stderr,"Postive leap second cancelled since dut1 > -0.3 sec\n");
		Positive_leap_second_pending = 0;
	} else if (Negative_leap_second_pending && dut1 < 3) {
		fprintf(stderr,"Negative leap second cancelled since dut1 < +0.3 sec\n");
		Negative_leap_second_pending = 0;
	}

	// output WAVE header
	fwrite(wav_header, sizeof(int16_t), sizeof(wav_header), stdout);
	fflush(stdout);

	wait_for_start();

	now = time(NULL);
	utc = gmtime(&now);
	minute = utc->tm_min;
	hour = utc->tm_hour;
	day = utc->tm_mday;
	month = utc->tm_mon + 1;
	year = utc->tm_year + 1900;

	/* get geophysical report on startup */
	get_geophys_data(&geophys_data);

	while (1) {
		// First buffer half for even minutes, latter half for odd minutes
		// Even minutes are always 60 seconds long
		int16_t *audio = (minute % 2) ? (Audio_buffer + 60 * Samprate) : Audio_buffer;

		int length = 60;    // Default length 60 seconds
		if ((month == 6 || month == 12) && hour == 23 && minute == 59) {
			if (Positive_leap_second_pending) {
				length = 61; // This minute ends with a leap second!
			} else if (Negative_leap_second_pending) {
				length = 59; // Negative leap second
			}
		}

		// Generate timecode
		unsigned char code[61]; // one extra for a possible leap second
		int leap_pending = (Positive_leap_second_pending || Negative_leap_second_pending) ? 1 : 0;
		if (WWVB) {
			makebetimecode(code,dut1,leap_pending,year,month,day,hour,minute);
		} else {
			maketimecode(code,dut1,leap_pending,year,month,day,hour,minute);
		}

		// Optionally dump timecode
		if (Verbose) {
			fprintf(stderr,"%d/%d/%d %02d:%02d\n",month,day,year,hour,minute);
			decode_timecode(code,length);
		}

		/* get UT1 difference */
		get_ut1_diff(&dut1);

		// Build a minute of audio
		if (WWVB) {
			makewwvbminute(audio,length,code);
		} else {
			makeminute(audio,length,WWVH,code,dut1,hour,minute,&geophys_data);
		}

#ifdef DIRECT
		if (!Direct_mode) {
#endif
			if (!manual_time) {
				manual_time = 1; // do this only first time
			}

			for (int i = 0; i < length; i++) {
				/* take a time reading */
				gettimeofday(&synctv, NULL);

				/*
				 * get the number of samples that should be skipped so the simulator
				 * stays in sync
				 */
				if (synctv.tv_usec < 250000) { /* going too slow */
					sync_delay = synctv.tv_usec / 1000;
					leadlag = 1;
				} else if (synctv.tv_usec > 750000) { /* going too fast */
					sync_delay = (1000000 - synctv.tv_usec) / 1000;
					leadlag = 2;
				} else {
					sync_delay = 0;
					leadlag = 0;
				}

				if (leadlag == 1) { /* skip ahead if we're lagging */
					samplenum = (sync_delay * Samprate_ms / 1000) + 2;
					delaysamplenum = 0;
				} else if (leadlag == 2) { /* delay if we're leading */
					samplenum = 0;
					delaysamplenum = (sync_delay * Samprate_ms / 1000) + 2;
				} else {
					samplenum = 0;
					delaysamplenum = 0;
				}

				if (Verbose) {
					if (samplenum)
						fprintf(stderr, "+ sample num: %d\n", samplenum);
					if (delaysamplenum)
						fprintf(stderr, "- sample num: %d\n", delaysamplenum);
				}

				/*
				 * avoid possible negative index when on the 1st second
				 * (correction will be done on the 2nd second)
				 */
				if (i == 0) samplenum = 0;

				// Write the constructed buffer, minus startup delay plus however many seconds
				// have already elapsed since the minute. This happens only at startup;
				// on all subsequent minutes the entire buffer will be written
				fwrite(audio + samplenum + Samprate * i, sizeof(*audio),
					Samprate + delaysamplenum - samplenum, stdout);
				fflush(stdout);

				if (shutdown) break;
			}
#ifdef DIRECT
		} else {
			Buffers++;
			if ((minute & 1) && length != 60)
				Odd_minute_length = length;	// Just wrote odd minute with leap second at end

			while (Buffers > 1)
				sleep(1);
		}
#endif

		if (shutdown) break;

		if (length == 61) {
			// Leap second just occurred in this last minute
			Positive_leap_second_pending = 0;
			dut1 += 10;
		} else if (length == 59) {
			Negative_leap_second_pending = 0;
			dut1 -= 10;
		}

		// Advance to next minute
		if (++minute > 59) {
			// New hour
			minute = 0;
			if (++hour > 23) {
				// New day
				hour = 0;

				if (++day > get_days_in_month(year, month)) {
					// New month
					day = 1;

					if (++month > 12) {
						// New year
						month = 1;
						++year;
					}
				}
			}
		}
	}

	free(Audio_buffer);
}

