/*
 * Compile using:
 *   gcc -Wall gme-pulse.c -o gme-pulse -lgme -lpulse-simple
 */
#include <stdio.h>
#include <stdlib.h>

#include <gme/gme.h>
#include <pulse/simple.h>

#define SAMPLE_RATE 44100
#define BIT_RESOLUTION 16
#define CHANNELS 2
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
  pa_simple *s;
  pa_sample_spec spec;
  int pa_error;
  Music_Emu *emu;
  gme_info_t *info;
  gme_err_t gmeErr;
  int track;
  short audio_buffer[BUFFER_SIZE];

  if (argc < 2)
  {
    printf("USAGE: gme-pulse <game music file> [track number]\n");
    return 1;
  }

  gmeErr = gme_open_file(argv[1], &emu, SAMPLE_RATE);
  if (gmeErr)
  {
    printf("%s\n", gmeErr);
    return 1;
  }
  if (argc >= 3)
    track = atoi(argv[2]);
  else
    track = 0;
  if (track < 0 || track > gme_track_count(emu))
  {
    printf("there is no track %d; playing track 0 instead\n", track);
    track = 0;
  }
  gmeErr = gme_track_info(emu, &info, track);
  if (gmeErr)
  {
    printf("%s\n", gmeErr);
    gme_delete(emu);
    return 2;
  }

  spec.channels = CHANNELS;
  spec.rate = SAMPLE_RATE;
  spec.format = PA_SAMPLE_S16LE;
  s = pa_simple_new(NULL, "Game Music Emu", PA_STREAM_PLAYBACK, NULL,
    "Audio", &spec, NULL, NULL, NULL);

  if (!s)
  {
    printf("problem opening audio via PulseAudio\n");
    gme_delete(emu);
    return 3;
  }

  printf("system: %s\ngame: %s\nsong: %s\nlength: %d ms\nplay length: %d ms\nplaying track %d...\nCtrl-C to exit\n",
    info->system, info->game, info->song, info->length, info->play_length, track);
  gme_start_track(emu, track);

  do
  {
    gmeErr = gme_play(emu, BUFFER_SIZE, audio_buffer);
    if (gmeErr)
    {
      printf("%s\n", gmeErr);
      break;
    }
    pa_simple_write(s, audio_buffer, BUFFER_SIZE * CHANNELS, &pa_error);
  } while (gme_tell(emu) < info->play_length);

  pa_simple_drain(s, &pa_error);
  pa_simple_free(s);

  gme_free_info(info);
  gme_delete(emu);

  return 0;
}

