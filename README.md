These are some tools for interacting with the Game Music Emu (GME) library.

The GME library lives at:

http://code.google.com/p/game-music-emu/

This repository contains several players based on GME.

# Player Clients

The files *gme-alsa.c* and *gme-pulse.c* are command line players that output
audio via the ALSA and PulseAudio APIs, respectively. Both of these programs
only work on Linux.

The file *gme-sdl.c* is an SDL-based player that provides an oscilloscope
visualization while playing the audio. This version should work on any
platform that supports SDL.

# Utilities

*gme2json.c* is a utility that outputs the metadata of a GME-compatible file
as JSON data.

*repack-rsn.py* repacks an RSN file (SNES SPC files in a RAR archive) into
a .gamemusic file.

*repack-vgm-7z.py repacks a 7z archive file containing VGM files into .gamemusic
file.

# Author

Mike Melanson (mike -at- multimedia.cx)
