*Note*: this repository was originally posted as `ka9q/WWV`, but the original repository is gone.

# WWV

WWV/WWVH emulator. Full format including 100 Hz timecode. Speech by local OS's synthesizer; OSX is best

This started as a quick hack but ended up fully functional. The program generates raw 16-bit linear PCM mono audio at a
48 kHz sample rate on standard output. So to hear it, pipe it into your local audio player, e.g.,

wwvsim | play -t raw -b 16 -e signed-integer -r 48000 -c 1 -

Or for the WWVH format:

wwvsim -H | play -t raw -b 16 -e signed-integer -r 48000 -c 1 -

Without arguments, it generates a WWV program using your local computer's clock. Run wwvsim with a bogus argument,
e.g., 'wwvsim -?' to get a list of command line arguments.

The program relies on the speech synthesizer bundled with the local system for voice announcements. This seems to
work best on Mac OSX, which has the 'say' program. On Linux, the 'espeak' program is used, but it sounds much worse.
Hopefully there are some better open source speech synthesizers out there.

The program tries to synchronize the output to the correct time but there are invariably errors due to buffering in
your computer's audio system. It also relies on the accuracy of your computer's clock, which should be controlled
by the Network Time Protocol. So this program is really a novelty; don't use it for anything really precise. That's
what the original WWV and WWVH are for!

