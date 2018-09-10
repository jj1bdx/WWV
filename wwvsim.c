// $Id: wwvsim.c,v 1.5 2018/03/12 17:12:37 karn Exp karn $
// WWV/WWVH simulator program. Generates their audio program as closely as possible
// Even supports UT1 offsets and leap second insertion
// Uses espeak synthesizer for speech announcements; needs a lot of work
// By default, uses system time, which should be NTP synchronized
// Time can be manually overridden for testing (announcements, leap seconds and other corner cases)

// July 2017, Phil Karn, KA9Q
// (Can you tell I have too much spare time?)

#include <assert.h>
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
#include <locale.h>

char Libdir[] = "/usr/local/share/ka9q-radio";

int Samprate = 48000; // Samples per second - try to use this if possible
int Samprate_ms;      // Samples per millisecond - sampling rates not divisible by 1000 may break

int Negative_leap_second_pending = 0; // If 1, leap second will be removed at end of June or December, whichever is first
int Positive_leap_second_pending = 0; // If 1, leap second will be inserted at end of June or December, whichever is first

// Is specified year a leap year?
int const is_leap_year(int y){
  if((y % 4) != 0)
    return 0; // Ordinary year; example: 2017
  if((y % 100) != 0)
    return 1; // Examples: 1956, 2004 (i.e., most leap years)
  if((y % 400) != 0)
    return 0; // Examples: 1900, 2100 (the big exception to the usual rule; non-leap US presidential election years)
  return 1; // Example: 2000 (the exception to the exception)
}

// Applies only to non-leap years; you need special tests for February in leap year
int const Days_in_month[] = { // Index 1 = January, 12 = December
  0,31,28,31,30,31,30,31,31,30,31,30,31
};

// Tone schedules for each minute of the hour for each station
// Special exception: no 440 Hz tone in first hour of UTC day; must be handled ad-hoc
int const WWV_tone_schedule[60] = {
    0,600,440,600,  0,600,500,600,  0,  0,
    0,  0,500,600,  0,  0,  0,600,  0,  0,
  500,600,500,600,500,600,500,600,500,  0,
    0,600,500,600,500,600,500,600,500,600,
  500,600,500,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,600,500,600,500,600,500,  0
};

int const WWVH_tone_schedule[60] = {
    0,440,600,  0,600,500,600,500,  0,  0,
    0,  0,600,500,  0,  0,  0,  0,  0,  0,
  600,500,600,500,600,500,600,500,600,  0,
    0,500,600,500,600,500,600,500,600,500,
  600,500,600,  0,  0,  0,600,  0,  0,  0,
    0,  0,  0,500,600,500,600,500,600,  0
};


// Generate complex phasor with specified angle in radians
// Used for tone generation
complex double const csincos(double x){
  return cos(x) + I*sin(x);
}

int16_t *Audio; // Audio output buffer, 1 minute long

// Synthesize an audio announcement and insert it into the audio buffer at 'startms' milliseconds within the minute
// Use female = 1 for WWVH, 0 for WWV
int announce(int startms,char const *message,int female){
  if(startms < 0 || startms >= 61000)
    return -1;

  // Overwrites buffer, so do early
  FILE *fp;
  if ((fp = fopen("/tmp/speakin","w")) == NULL)
    return -1;
  fputs(message,fp);
  fclose(fp);
#ifdef __APPLE__
  if(female){
    system("say -v Samantha --output-file=/tmp/speakout.wav --data-format=LEI16@48000 -f /tmp/speakin;sox /tmp/speakout.wav -t raw -r 48000 -c 1 -b 16 -e signed-integer /tmp/speakout");
  } else {
    system("say -v Alex --output-file=/tmp/speakout.wav --data-format=LEI16@48000 -f /tmp/speakin;sox /tmp/speakout.wav -t raw -r 48000 -c 1 -b 16 -e signed-integer /tmp/speakout");
  }
#else // linux
  if(female){
    system("espeak -v en-us+f3 -a 70 -f /tmp/speakin --stdout | sox -t wav - -t raw -r 48000 -c 1 -e signed-integer -b 16 /tmp/speakout");
  } else {
    system("espeak -v en-us -a 70 -f /tmp/speakin --stdout | sox -t wav - -t raw -r 48000 -c 1 -e signed-integer -b 16 /tmp/speakout");
  }
#endif

  unlink("/tmp/speakin");
  if((fp = fopen("/tmp/speakout","r")) == NULL)
    return -1;
  fread(Audio+startms*Samprate_ms,sizeof(*Audio),Samprate_ms*(61000-startms),fp);
  fclose(fp);
  unlink("/tmp/speakout");
  return 0;
}

// Overlay a tone with frequency 'freq' in audio buffer, overwriting whatever was there
// starting at 'startms' within the minute and stopping one sample before 'stopms'. 
// Amplitude 1.0 is 100% modulation, 0.5 is 50% modulation, etc
// Used first for 500/600 Hz continuous audio tones
// Then used for 1000/1200 Hz minute/hour beeps and second ticks, which pre-empt everything else.
int overlay_tone(int startms,int stopms,float freq,float amp){
  if(startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
    return -1;

  assert((startms * (int)freq % 1000) == 0); // All tones start with a positive zero crossing?

  complex double phase = 1;
  complex double const phase_step = csincos(2*M_PI*freq/Samprate);
  int16_t *buffer = Audio + startms*Samprate_ms;
  int samples = (stopms - startms)*Samprate_ms;
  while(samples-- > 0){
    *buffer++ = cimag(phase) * amp * SHRT_MAX; // imaginary component is sine, real is cosine
    phase *= phase_step;  // Rotate the tone phasor
  }
 return 0;
}

// Same as overlay_tone() except that the tone is added to whatever is already in the audio buffer
// Take care to avoid overmodulation; the result will be clipped but could still sound bad
// Used mainly for 100 Hz subcarrier
int add_tone(int startms,int stopms,float freq,float amp){
  if(startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
    return -1;

  assert((startms * (int)freq % 1000) == 0); // All tones start with a positive zero crossing?

  complex double phase = 1;
  complex double const phase_step = csincos(2*M_PI*freq/Samprate);
  int16_t *buffer = Audio + startms*Samprate_ms;
  int samples = (stopms - startms)*Samprate_ms;
  while(samples-- > 0){
    // Add and clip
    float const samp = *buffer + cimag(phase) * amp * SHRT_MAX;
    *buffer++ = samp > 32767 ? 32767 : samp < -32767 ? -32767 : samp;
    phase *= phase_step; // Rotate the tone phasor
  }
  return 0;
}

// Blank out whatever is in the audio buffer starting at startms and ending just before stopms
// Used mainly to blank out 40 ms guard interval around seconds ticks
int overlay_silence(int startms,int stopms){
  if(startms < 0 || startms >= 61000 || stopms <= startms || stopms > 61000)
    return -1;
  int16_t *buffer = Audio + startms*Samprate_ms;
  int samples = (stopms - startms)*Samprate_ms;

  while(samples-- > 0)
    *buffer++ = 0;

  return 0;
}

// Encode a BCD digit in little-endian format (lsb first)
// NB! Only WWV/WWVH; WWVB uses big-endian format
void encode_bcd_le(unsigned char *code,int x){
  int i;
  for(i=0;i<4;i++){
    code[i] = x & 1;
    x >>= 1;
  }
}

int WWVH = 0; // WWV by default
int Verbose = 0;

int main(int argc,char *argv[]){
  int c;
  int year,month,day,hour,minute,sec,doy;
  // Amplitudes
  const float marker_high_amp = pow(10.,-15.0/20.);
  const float marker_low_amp = pow(10.,-30.0/20.);
  const float tick_amp = 1.0; // 100%, 0dBFS
  const float tone_amp = pow(10.,-6.0/20.); // -6 dB

  // Defaults
  double tickfreq = 1000.0; // WWV
  double hourbeep = 1500.0; // Both WWV and WWVH
  int dut1 = 0;
  int manual_time = 0;

  // Use current computer time as default
  struct timeval startup_tv;
  gettimeofday(&startup_tv,NULL);
  struct tm const * const tm = gmtime(&startup_tv.tv_sec);
  sec = tm->tm_sec;
  minute = tm->tm_min;
  hour = tm->tm_hour;
  day = tm->tm_mday;
  month = tm->tm_mon + 1;
  year = tm->tm_year + 1900;
  setlocale(LC_ALL,getenv("LANG"));

  // Read and process command line arguments
  while((c = getopt(argc,argv,"HY:M:D:h:m:s:u:r:LNv")) != EOF){
    switch(c){
    case 'v':
      Verbose++;
      break;
    case 'r':
      Samprate = strtol(optarg,NULL,0); // Try not to change this, may not work
      break;
    case 'H': // Simulate WWVH, otherwise WWV
      WWVH++;
      tickfreq = 1200;
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
      sec = strtol(optarg,NULL,0);
      manual_time++;
      break;
    case 'L':
      Positive_leap_second_pending++; // Positive leap second at end of current month
      break;
    case 'N':
      Negative_leap_second_pending++;  // Leap second at end of current month
      break;
    case '?':
      fprintf(stderr,"Usage: %s [-v] [-r samprate] [-H] [-u ut1offset] [-Y year] [-M month] [-D day] [-h hour] [-m min] [-s sec] [-L|-N]\n",argv[0]);
      fprintf(stderr,"Default sample rate: 48 kHz\n");
      fprintf(stderr,"By default uses current system time; Use -Y/-M/-D/-h/-m/-s to override for testing, e.g., of leap seconds\n");
      fprintf(stderr,"-v turns on verbose reporting. -H selects the WWVH format; default is WWV\n");
      fprintf(stderr,"-u specifies current UT1-UTC offset in tenths of a second, must be between -7 and +7\n");
      fprintf(stderr,"-L introduces a positive leap second at the end of June or December, whichever comes first\n");
      fprintf(stderr,"-N introduces a negative leap second at the end of June or December, whichever comes first. Only one of -L and -N can be given\n");

      exit(1);
	      
    }	
  }
  // Compute day of year
  // don't use doy in tm struct in case date was manually overridden
  // (Bug found and reported by Jayson Smith jaybird@bluegrasspals.com)
  doy = day;
  for(int i = 1; i < month; i++){
    if(i == 2 && is_leap_year(year))
      doy += 29;
    else
      doy += Days_in_month[i];
  }

  if(isatty(fileno(stdout))){
    fprintf(stderr,"Won't write raw PCM audio to a terminal. Redirect or pipe.\n");
    exit(1);
  }

  if(Positive_leap_second_pending && Negative_leap_second_pending){
    fprintf(stderr,"Positive and negative leap seconds can't both be pending! Both cancelled\n");
    Positive_leap_second_pending = Negative_leap_second_pending = 0;
  }

  if(dut1 > 7 || dut1 < -7){
    fprintf(stderr,"ut1 offset %d out of range, limited to -7 to +7 tenths\n",dut1);
    dut1 = 0;
  }

  Audio = malloc(Samprate*61*sizeof(int16_t));
  Samprate_ms = Samprate/1000; // Samples per ms

  // Only US rules are needed, since WWV/WWVH are American stations
  // US rules last changed in 2007 to 2nd sunday of March to first sunday in November
  // Always lasts for 238 days
  // 2007: 3/11      2008: 3/9      2009: 3/8      2010: 3/14      2011: 3/13      2012: 3/11
  // 2013: 3/10      2014: 3/9      2015: 3/8      2016: 3/13      2017: 3/12      2018: 3/11
  // 2019: 3/10      2020: 3/8

  int dst_start_doy = 0;
  if(year < 2007){
    // Punt
    fprintf(stderr,"Warning: DST rules prior to %d not implemented; DST bits = 0\n",year);
  } else {
    int ytmp = 2007;
    int dst_start_dom = 11;
    for(;ytmp<= year;ytmp++){
      dst_start_dom--; // One day earlier each year
      if(is_leap_year(ytmp))
	dst_start_dom--; // And an extra day earlier after a leap year february
      if(dst_start_dom <= 7) // No longer second sunday, slip a week
	dst_start_dom += 7;
    }
    dst_start_doy = 59 + dst_start_dom;
    if(is_leap_year(year))
      dst_start_doy++;
  }
  while(1){
    int length = 60;    // Default length 60 seconds
    if((month == 6 || month == 12) && hour == 23 && minute == 59){
      if(Positive_leap_second_pending){
	length = 61; // This minute ends with a leap second!
      } else if(Negative_leap_second_pending){
	length = 59; // Negative leap second
      }
    }
    // Build a minute of audio
    memset(Audio,0,61*Samprate*sizeof(*Audio)); // Clear previous audio

    // Continuous tone (or silence) from start of second 1 through end of second 44
    double tone;
    if(WWVH)
      tone = WWVH_tone_schedule[minute];
    else
      tone = WWV_tone_schedule[minute];
    
    // Special case: no 440 Hz tone during hour 0
    if(tone == 440 && hour == 0)
      tone = 0;

    if(tone){
      add_tone(1000,45000,tone,tone_amp); // Continuous tone from 1 sec until 45 sec
    } else if(WWVH && (minute == 59 || minute == 29)){
      // WWVH IDs at minute 29 and 59 in female voice
      char fname[PATH_MAX];

      snprintf(fname,sizeof(fname),"%s/%s",Libdir,"wwvh.txt");
      FILE *fp = fopen(fname,"r");
      if(fp != NULL){
	char buffer[10000];
	int r;
	r = fread(buffer,sizeof(*buffer),10000,fp);
	fclose(fp);
	if(r > 0){
	  buffer[r] = '\0';
	  announce(1000,buffer,1);
	}
      }
    } else if(!WWVH && (minute == 0 || minute == 30)){
      // WWV IDs at minute 0 and 30 in male voice
      char fname[PATH_MAX];

      snprintf(fname,sizeof(fname),"%s/%s",Libdir,"wwv.txt");
      FILE *fp = fopen(fname,"r");
      if(fp != NULL){
	char buffer[10000];
	int r;

	r = fread(buffer,sizeof(*buffer),10000,fp);
	fclose(fp);
	if(r > 0){
	  buffer[r] = '\0';
	  announce(1000,buffer,0);
	}
      }
    }

    // Insert minute announcement
    int nextminute,nexthour; // What are the next hour and minute?
    nextminute = minute;
    nexthour = hour;
    if(++nextminute == 60){
      nextminute = 0;
      if(++nexthour == 24)
	nexthour = 0;
    }
    char message[1024];
    snprintf(message,sizeof(message),"At the tone, %d %s %d %s Coordinated Universal Time",
	     nexthour,nexthour == 1 ? "hour" : "hours",
	     nextminute,nextminute == 1 ? "minute" : "minutes");
    if(!WWVH)
      announce(52500,message,0); // WWV: male voice at 52.5 seconds
    else
      announce(45000,message,1); // WWVH: female voice at 45 seconds

    // Generate and add 100 Hz timecode
    unsigned char code[61]; // one extra for a possible leap second

    memset(code,0,sizeof(code)); // All bits default to 0
    if(dst_start_doy != 0){
      // DST always lasts for 238 days
      if(doy > dst_start_doy && doy <= dst_start_doy + 238)
	code[2] = 1; // DST status at 00:00 UTC
      if(doy >= dst_start_doy && doy < dst_start_doy + 238)
	code[55] = 1; // DST status at 24:00 UTC
    }
    code[3] = Negative_leap_second_pending || Positive_leap_second_pending;

    // Year
    encode_bcd_le(code+4,year % 10); // Least significant digit
    encode_bcd_le(code+51,(year/10)%10); // Tens digit

    // Minute of hour, 0-59
    encode_bcd_le(code+10,minute%10); // Least significant digit
    encode_bcd_le(code+15,minute/10); // Most significant digit, extends into unused bit 18

    // Hour of day, 0-23
    encode_bcd_le(code+20,hour%10);   // Least significant digit
    encode_bcd_le(code+25,hour/10);   // Most significant digit, extends into unused bits 27-28

    // Day of year, 1-366
    encode_bcd_le(code+30,doy%10);    // Least significant digit
    encode_bcd_le(code+35,(doy/10)%10); // Middle digit
    encode_bcd_le(code+40,doy/100);   // High digit, extends into unused bits 42-43

    // UT1 offset, +/-0.0 through 0.7; adjusted after leap second
    code[50] = (dut1 > 0); // sign
    encode_bcd_le(code+56,abs(dut1));  // magnitude, extends into marker 59 and is ignored

    // Modulate time code onto 100 Hz subcarrier
    int s;
    for(s=1;s<61;s++){ // No subcarrier during second 0 (minute/hour beep)
      if((s % 10) == 9){
	add_tone(s*1000,s*1000+800,100,marker_high_amp);	 // 800 ms markers @ -15 dBFS
	add_tone(s*1000+800,s*1000+1000,100,marker_low_amp);     // rest of tone at -30 dBFS
      } else if(code[s]){
	add_tone(s*1000,s*1000+500,100,marker_high_amp);	 // 500 ms = 1 bit
	add_tone(s*1000+500,s*1000+1000,100,marker_low_amp);
      } else {
	add_tone(s*1000,s*1000+200,100,marker_high_amp);	 // 200 ms = 0 bit
	add_tone(s*1000+200,s*1000+1000,100,marker_low_amp);
      }
    }

    // Pre-empt with minute/hour beep and guard interval
    overlay_tone(0,800,minute == 0 ? hourbeep : tickfreq,tick_amp);
    overlay_silence(800,1000);
    
    // Pre-empt with second ticks and guard interval
    for(s=1;s<60;s++){
      if(s != 29 && s != 59){ // No ticks on 29 and 59 (or 60)
	overlay_silence(1000*s-10,1000*s);          // 10 ms of silence at end of previous second (OK because we start with s=1)
	overlay_tone(1000*s,1000*s+5,tickfreq,tick_amp); // 5 ms tick at 100% modulation on second
	overlay_silence(1000*s+5,1000*s+30);        // 25 ms of silence after the tick
	// Double ticks without guard time for UT1 offset
	if((dut1 > 0 && s >= 1 && s <= dut1)
	   || (-dut1 > 0 && s >= 9 && s <= 8-dut1)){
	  overlay_tone(1000*s+100,1000*s+105,tickfreq,tick_amp); // 5 ms second tick at 100 ms
	}	  
      }
    }
    if(Verbose){
      fprintf(stderr,"%d/%d/%d %02d:%02d:%02d\n",month,day,year,hour,minute,sec);
      int s;
      for(s=0;s<length;s++){
	if((s % 10) == 0 && s < 60)
	  fprintf(stderr,"%02d: ",s);
	if(s == 0)
	  fputc(' ',stderr);
	else if((s % 10) == 9)
	  fprintf(stderr,"M");
	else
	  fputc(code[s] ? '1' : '0',stderr);
	if(s < 59 && (s % 10 == 9))
	  fputc('\n',stderr);
      }
      fputc('\n',stderr);      
    }

    int samplenum = 0;
    if(!manual_time){
      // Find time interval since startup, trim that many samples from the beginning of the buffer so we are on time
      struct timeval tv;
      gettimeofday(&tv,NULL);
      int startup_delay = 1000000*(tv.tv_sec - startup_tv.tv_sec) +
	tv.tv_usec - startup_tv.tv_usec;
      
      if(Verbose)
	fprintf(stderr,"startup delay %'d usec\n",startup_delay);
      samplenum = (Samprate_ms * startup_delay) / 1000;
      manual_time = 1; // do this only first time
    }
    // Write the constructed buffer, minus startup delay plus however many seconds
    // have already elapsed since the minute. This happens only at startup;
    // on all subsequent minutes the entire buffer will be written
    fwrite(Audio+samplenum+sec*Samprate,sizeof(*Audio),
	   Samprate * (length-sec) - samplenum,stdout);

    if(length == 61){
      // Leap second just occurred in this last minute
      Positive_leap_second_pending = 0;
      dut1 += 10;
    } else if(length == 59){
      Negative_leap_second_pending = 0;
      dut1 -= 10;
    }
    // Advance to next minute
    sec = 0;
    if(++minute > 59){
      // New hour
      minute = 0;
      if(++hour > 23){
	// New day
	hour = 0;
	doy++;
	if(++day > ((month == 2 && is_leap_year(year))? 29 : Days_in_month[month])){
	  // New month
	  day = 1;
	  if(++month > 12){
	    // New year
	    month = 1;
	    ++year;
	    doy = 1;
	  }
	}
      }
    }
  }
}

