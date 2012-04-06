/*
 * ALSA code mostly cribbed from:
 *   http://equalarea.com/paul/alsa-audio.html
 * 
 * Compile using:
 *   gcc -Wall gme-alsa.c -o gme-alsa -lgme -lasound
 */
#include <stdio.h>
#include <stdlib.h>

#include <gme/gme.h>

#ifdef __linux__
#include <alsa/asoundlib.h>
#else
#error This program is designed for Linux (uses Linux-only APIs)
#endif

#define SAMPLE_RATE 44100
#define BIT_RESOLUTION 16
#define CHANNELS 2
#define BUFFER_SIZE 1024
#define ALSA_DEVICE "default"

int main(int argc, char *argv[])
{
  snd_pcm_t *playback_handle;
  snd_pcm_hw_params_t *hw_params;
  int err;
  Music_Emu *emu;
  gme_info_t *info;
  gme_err_t gmeErr;
  int track;
  short audio_buffer[BUFFER_SIZE];
  unsigned int sample_rate;

  if (argc < 2)
  {
    printf("USAGE: gme-alsa <game music file> [track number]\n");
    return 1;
  }

  /* open ALSA */
  err = snd_pcm_open(&playback_handle, ALSA_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0)
  {
    printf("cannot open audio device %s (%s)\n", ALSA_DEVICE,
      snd_strerror(err));
    exit(1);
  }

  err = snd_pcm_hw_params_malloc(&hw_params);
  if (err < 0)
  {
    printf("cannot allocate hardware parameter structure (%s)\n",
      snd_strerror(err));
    exit(1);
  }

  err = snd_pcm_hw_params_any(playback_handle, hw_params);
  if (err < 0)
  {
    printf("cannot initialize hardware parameter structure (%s)\n",
      snd_strerror(err));
    exit(1);
  }

  err = snd_pcm_hw_params_set_access(playback_handle, hw_params, 
    SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0)
  {
    printf("cannot set access type (%s)\n",
      snd_strerror(err));
    exit(1);
  }

  err = snd_pcm_hw_params_set_format(playback_handle, hw_params,
    SND_PCM_FORMAT_S16_LE);
  if (err < 0)
  {
    printf("cannot set sample format (%s)\n",
      snd_strerror(err));
    exit(1);
  }

  sample_rate = SAMPLE_RATE;
  err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params,
    &sample_rate, 0);
  if (err < 0)
  {
    printf("cannot set sample rate (%s)\n",
      snd_strerror(err));
    exit(1);
  }
  if (sample_rate != SAMPLE_RATE)
    printf("requested %d Hz; got %d Hz (not a problem)\n",
      SAMPLE_RATE, sample_rate);

  err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, CHANNELS);
  if (err < 0)
  {
    printf("cannot set channel count (%s)\n",
      snd_strerror(err));
    exit(1);
  }

  err = snd_pcm_hw_params(playback_handle, hw_params);
  if (err < 0)
  {
    printf("cannot set parameters (%s)\n",
      snd_strerror(err));
    exit(1);
  }

  snd_pcm_hw_params_free(hw_params);

  /* initialize GME based on parameter; open GME after opening ALSA since
   * ALSA might return a different sample rate */
  gmeErr = gme_open_file(argv[1], &emu, sample_rate);
  if (gmeErr)
  {
    printf("%s\n", gmeErr);
    snd_pcm_close(playback_handle);
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
    snd_pcm_close(playback_handle);
    return 2;
  }

  /* get readto play */
  err = snd_pcm_prepare(playback_handle);
  if (err < 0)
  {
    printf("cannot prepare audio interface for use (%s)\n",
      snd_strerror(err));
    exit(1);
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
    err = snd_pcm_writei(playback_handle, audio_buffer, BUFFER_SIZE / 2);
    if (err != (BUFFER_SIZE / 2))
    {
      printf("write to audio interface failed (%s)\n",
        snd_strerror(err));
      exit(1);
    }
  } while (gme_tell(emu) < info->play_length);

  snd_pcm_close(playback_handle);

  gme_free_info(info);
  gme_delete(emu);

  return 0;
}

