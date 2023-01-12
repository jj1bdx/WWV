*Note*: this repository was originally posted as `ka9q/WWV`, but the original repository is gone.

# WWV

WWV/WWVH emulator. Full format including 100 Hz timecode. Speech using bundled voice data
from [kalafut's WWV simulator](https://github.com/kalafut/wwv).

This started as a quick hack but ended up fully functional. The program generates raw 16-bit linear PCM mono audio at a
16 kHz sample rate on standard output. So to hear it, pipe it into your local audio player, e.g.,
```
wwvsim | aplay
```

Or for the WWVH format:

```
wwvsim -H | aplay
```
Without arguments, it generates a WWV program using your local computer's clock. Run wwvsim with a bogus argument,
e.g., 'wwvsim -?' to get a list of command line arguments.

The program relies on pre-generated speech audio to make the announcements.

The program tries to synchronize the output to the correct time but there are invariably errors due to buffering in
your computer's audio system. It also relies on the accuracy of your computer's clock, which should be controlled
by the Network Time Protocol. So this program is really a novelty; don't use it for anything really precise. That's
what the original WWV and WWVH are for!

## Build
Just run make -j4. It will take a while for the pre-generated voice data to be compiled in.

## Hourly program details
In addition to the time and station ID, there are other announcments made throughout every hour.
- HamSCI announcement on minute 3 on WWVH and minute 4 of WWV
- HamSCI test signal on minute 8 on WWV and minute 48 of WWVH
- DoD M.A.R.S. announcements on minute 10 of WWV and minute 50 on WWVH
- (custom) 3G shutdown announcement on minutes 14 & 44 on WWV and 16 & 16 on WWVH (will be used for future program or removed completely)
- Geophysical alerts on minute 18 of WWV and minute 45 on WWVH
- Telephone availability for WWVH on minutes 47 & 52
