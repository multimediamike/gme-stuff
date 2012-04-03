/*
 * Compile using:
 *   gcc -g -Wall gme-sdl.c -o gme-sdl `pkg-config --cflags --libs sdl` -lgme
 */
#include <stdio.h>
#include <stdlib.h>

#include <gme/gme.h>
#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

#define SAMPLE_RATE 44100
#define BIT_RESOLUTION 16
#define CHANNELS 2
#define BUFFER_SIZE 1024
#define AUDIO_QUEUE_SIZE 5 
/* this is because there are only 10 number keys */
#define MAX_VOICES 10

typedef struct
{
  int timestamp;  /* in milliseconds */
  short audio_buffer[BUFFER_SIZE];
} audio_buffer;

static int audio_queue_head = 0;
static int audio_queue_count = 0;
static SDL_mutex *audio_queue_mutex;
static audio_buffer audio_queue[AUDIO_QUEUE_SIZE];

void gme_feedaudio(void *unused, Uint8 *stream, int len)
{
  SDL_LockMutex(audio_queue_mutex);

  if (audio_queue_count == 0)
  {
    SDL_UnlockMutex(audio_queue_mutex);
    return;
  }

  SDL_MixAudio(stream, (uint8_t *)(audio_queue[audio_queue_head].audio_buffer), 
    len, SDL_MIX_MAXVOLUME);
  audio_queue_head = (audio_queue_head + 1) % AUDIO_QUEUE_SIZE;
  audio_queue_count--;

  SDL_UnlockMutex(audio_queue_mutex);
}

int main(int argc, char *argv[])
{
  SDL_Surface *screen;
  SDL_AudioSpec fmt;
  SDL_AudioSpec actual_fmt;
  SDL_Event event;
  int keyPressActive;
  Music_Emu *emu;
  gme_info_t *info;
  gme_err_t gmeErr;
  int track;
  int i, j;
  int voice_flags[MAX_VOICES];
  int finished;

  if (argc < 2)
  {
    printf("USAGE: gme-pulse <game music file> [track number]\n");
    return 1;
  }

  /* initialize the engine based on the file parameter */
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

  /* initialize the audio queue before launching the audio thread */
  gme_start_track(emu, track);
  for (i = 0; i < AUDIO_QUEUE_SIZE; i++)
  {
    audio_queue[i].timestamp = gme_tell(emu);
    gmeErr = gme_play(emu, BUFFER_SIZE, audio_queue[i].audio_buffer);
    if (gmeErr)
    {
      printf("%s\n", gmeErr);
      break;
    }
  }
  audio_queue_count = AUDIO_QUEUE_SIZE;  /* starts out full */

  /* initialize SDL audio and start playing */
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0)
  {
    printf ("could not initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  fmt.freq = SAMPLE_RATE;
  fmt.format = AUDIO_S16;
  fmt.channels = CHANNELS;
  fmt.samples = BUFFER_SIZE;
  fmt.callback = gme_feedaudio;
  fmt.userdata = NULL;

  /* open audio */
  if (SDL_OpenAudio(&fmt, &actual_fmt) < 0)
  {
    printf("could not open SDL audio: %s\n", SDL_GetError());
    exit(1);
  }
  audio_queue_mutex = SDL_CreateMutex();

  /* create video window */
  screen = SDL_SetVideoMode(512, 256, 32, SDL_SWSURFACE);
  if (screen == NULL)
  {
    printf("could not set video mode: %s\n", SDL_GetError());
    exit(1);
  }
  SDL_WM_SetCaption("SDL Game Music Emu", NULL);

  printf("system: %s\ngame: %s\nsong: %s\nlength: %d ms\nplay length: %d ms\nplaying track %d...\nCtrl-C to exit\n",
    info->system, info->game, info->song, info->length, info->play_length, track);
  for (i = 0; i < gme_voice_count(emu); i++)
    printf("voice %d: %s\n", i, gme_voice_name(emu, i));
  memset(voice_flags, 0, MAX_VOICES * sizeof(int));

  SDL_PauseAudio(0);
  finished = 0;
  keyPressActive = 0;
  while (!finished)
  {
    SDL_Delay(1);

    SDL_PollEvent(&event);
    if (keyPressActive && event.type == SDL_KEYUP)
      keyPressActive = 0;

    if (!keyPressActive && event.type == SDL_KEYDOWN)
    {
      keyPressActive = 1;

      switch (event.key.keysym.sym)
      {
        case SDLK_ESCAPE:
        case SDLK_q:
          finished = 1;
          break;

        case SDLK_0:
        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
        case SDLK_8:
        case SDLK_9:
          i = event.key.keysym.sym - SDLK_0;
          voice_flags[i] ^= 1;
          gme_mute_voice(emu, i, voice_flags[i]);
          break;

        case SDLK_LEFT:
        case SDLK_RIGHT:
          break;

        default:
          break;
      }
    }

    /* check if it's time to generate more audio */
    SDL_LockMutex(audio_queue_mutex);
    for (i = 0; i < AUDIO_QUEUE_SIZE - audio_queue_count; i++)
    {
      j = (audio_queue_head + audio_queue_count + i) % AUDIO_QUEUE_SIZE;
      audio_queue[j].timestamp = gme_tell(emu);
      gmeErr = gme_play(emu, BUFFER_SIZE, audio_queue[j].audio_buffer);
      if (gmeErr)
      {
        printf("%s\n", gmeErr);
        break;
      }
    }
    audio_queue_count = AUDIO_QUEUE_SIZE; /* maxed out again */
    SDL_UnlockMutex(audio_queue_mutex);
  }

  SDL_CloseAudio();

  gme_free_info(info);
  gme_delete(emu);

  return 0;
}

