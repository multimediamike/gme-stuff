/*
 * Compile using:
 *   gcc -g -Wall gme-sdl.c -o gme-sdl `sdl-config --cflags --libs` -lgme
 */
#include <stdio.h>
#include <stdlib.h>

#include <gme/gme.h>
#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2

#define BUFFER_SIZE (SAMPLE_RATE * CHANNELS)
#define PERIOD_SIZE (BUFFER_SIZE / 10)
/* arbitrary limit: voices correspond to numbers 1-9 on keyboard */
#define MAX_VOICES 9
#define FRAME_RATE 30
#define WIDTH 512
#define HEIGHT 256
#define CAPTION_STRING_LEN 100

static SDL_mutex *audio_mutex;
static short audio_buffer[BUFFER_SIZE];
static unsigned int audio_start = 0;
static unsigned int audio_end = 0;

void gme_feedaudio(void *unused, Uint8 *stream, int req_len_in_bytes)
{
  unsigned int actual_len;
  int buffer_start_in_bytes, buffer_end_in_bytes;

  SDL_LockMutex(audio_mutex);

  /* if there is no audio ready to feed, leave */
  if (audio_start >= audio_end)
  {
    SDL_UnlockMutex(audio_mutex);
    return;
  }

  buffer_start_in_bytes = (audio_start % BUFFER_SIZE) * 2;
  buffer_end_in_bytes = (audio_end % BUFFER_SIZE ) * 2;

  /* decide how much to feed */
  if (buffer_start_in_bytes > buffer_end_in_bytes)
    actual_len = BUFFER_SIZE * 2 - buffer_start_in_bytes;
  else
    actual_len = buffer_end_in_bytes - buffer_start_in_bytes;
  if (actual_len > req_len_in_bytes)
    actual_len = req_len_in_bytes;

  /* feed it */
  SDL_MixAudio(stream, (uint8_t *)(&audio_buffer[buffer_start_in_bytes / 2]), 
    actual_len, SDL_MIX_MAXVOLUME);
  audio_start += actual_len / 2;

  SDL_UnlockMutex(audio_mutex);
}

int main(int argc, char *argv[])
{
  SDL_Surface *screen;
  SDL_AudioSpec fmt;
  SDL_AudioSpec actual_fmt;
  SDL_Event event;
  unsigned int *pixels;
  int keyPressActive;
  Music_Emu *emu;
  gme_info_t *info;
  gme_err_t gmeErr;
  int track;
  int i;
  int voice_flags[MAX_VOICES];
  int finished;
  int ms_to_update_video;
  int frame_counter;
  unsigned int pixel;
  Uint32 base_clock;
  Uint32 current_tick;
  int audio_started;
  short *viz_buffer;
  int print_metadata = 1;
  char caption_string[CAPTION_STRING_LEN];

  unsigned char r_color, g_color, b_color;
  char r_inc, g_inc, b_inc;

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
    track = 1;
  if (track < 1 || track > gme_track_count(emu))
  {
    printf("there is no track %d; playing track 1 instead\n", track);
    track = 1;
  }
  gme_start_track(emu, track - 1);

  /* initialize SDL audio and start playing */
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0)
  {
    printf ("could not initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  fmt.freq = SAMPLE_RATE;
  fmt.format = AUDIO_S16;
  fmt.channels = CHANNELS;
  fmt.samples = 512;
  fmt.callback = gme_feedaudio;
  fmt.userdata = NULL;

  /* open audio */
  if (SDL_OpenAudio(&fmt, &actual_fmt) < 0)
  {
    printf("could not open SDL audio: %s\n", SDL_GetError());
    exit(1);
  }
  audio_mutex = SDL_CreateMutex();

  /* create video window */
  screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, SDL_SWSURFACE);
  if (screen == NULL)
  {
    printf("could not set video mode: %s\n", SDL_GetError());
    exit(1);
  }
  pixels = (unsigned int *)screen->pixels;

  memset(voice_flags, 0, MAX_VOICES * sizeof(int));

  /* initialize the visualization matters */
  frame_counter = 0;
  ms_to_update_video = frame_counter * 1000 / FRAME_RATE;
  r_color = g_color = b_color = 250;
  srand((argc << 24) | (argv[1][0] << 16) | strlen(argv[1]));
  r_inc = -1 * (rand() % 3 + 1);
  g_inc = -1 * (rand() % 3 + 1);
  b_inc = -1 * (rand() % 3 + 1);

  finished = 0;
  keyPressActive = 0;
  audio_started = 0;
  base_clock = SDL_GetTicks();
  while (!finished)
  {
    if (print_metadata)
    {
      gmeErr = gme_track_info(emu, &info, track - 1);
      snprintf(caption_string, CAPTION_STRING_LEN, "%s - %s (Game Music Emu)",
        info->game, info->song);
      SDL_WM_SetCaption(caption_string, NULL);
      printf("Playing track %d / %d\n", track, gme_track_count(emu));
      printf("system: %s\ngame: %s\nsong: %s\nlength: %d ms\nplay length: %d ms\n",
        info->system, info->game, info->song, info->length, info->play_length);
      for (i = 1; i <= gme_voice_count(emu); i++)
        printf("voice %d: %s\n", i, gme_voice_name(emu, i - 1));
      if (gme_track_count(emu) > 1)
        printf("  press left or right to change tracks\n");
      printf("  press number keys to toggle voices\n");
      printf("  press ESC or q to exit\n\n");
      gme_free_info(info);
      print_metadata = 0;
    }

    /* check if it's time to generate more audio */
    SDL_LockMutex(audio_mutex);
    if ((audio_end - audio_start) < (PERIOD_SIZE / 2))
    {
      gmeErr = gme_play(emu, PERIOD_SIZE, &audio_buffer[audio_end % BUFFER_SIZE]);
      if (gmeErr)
      {
        SDL_UnlockMutex(audio_mutex);
        printf("%s\n", gmeErr);
        break;
      }
      audio_end += PERIOD_SIZE;
    }
    SDL_UnlockMutex(audio_mutex);

    if (!audio_started)
    {
      audio_started = 1;
      SDL_PauseAudio(0);
    }

    /* see if it's time to update the visualization */
    current_tick = SDL_GetTicks() - base_clock;
    if (current_tick >= ms_to_update_video)
    {
      /* decide on a pixel color */
      pixel = (r_color << 16) | (g_color << 8) | (b_color << 0);
      r_color += r_inc;
      if (r_color <  64 || r_color > 250)
        r_inc *= -1;
      g_color += g_inc;
      if (g_color < 192 || g_color > 250)
        g_inc *= -1;
      b_color += b_inc;
      if (b_color < 128 || b_color > 250)
        b_inc *= -1;

      if (SDL_MUSTLOCK(screen))
        SDL_LockSurface(screen);

      memset(pixels, 0, WIDTH * HEIGHT * sizeof(unsigned int));
      viz_buffer = &audio_buffer[(frame_counter % FRAME_RATE) * BUFFER_SIZE / FRAME_RATE];
      for (i = 0; i < WIDTH * CHANNELS; i++)
      {
        if (i & 1)  /* right channel data */
          pixels[WIDTH * ((192 - (viz_buffer[i] / 512)) - 1) + (i >> 1)] = pixel;
        else        /* left channel data */
          pixels[WIDTH * (64 - (viz_buffer[i] / 512)) + (i >> 1)] = pixel;
      }
      for (i = 0; i < WIDTH; i++)
        pixels[128 * WIDTH + i] = 0xFFFFFFFF;

      if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);
      SDL_UpdateRect(screen, 0, 0, 0, 0);

      frame_counter++;
      ms_to_update_video = frame_counter * 1000 / FRAME_RATE;
    }

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

        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
        case SDLK_8:
        case SDLK_9:
          i = event.key.keysym.sym - SDLK_1;
          if (i < gme_voice_count(emu))
          {
            voice_flags[i] ^= 1;
            gme_mute_voice(emu, i, voice_flags[i]);
          }
          break;

        case SDLK_LEFT:
        case SDLK_RIGHT:
          /* don't change tracks unless there are multiple tracks */
          if (gme_track_count(emu) <= 1)
            break;

          if (event.key.keysym.sym == SDLK_LEFT)
            i = -1;
          else
            i = 1;
          track += i;
          if (track > gme_track_count(emu))
            track = 1;
          if (track < 1)
            track = gme_track_count(emu);
          gme_start_track(emu, track - 1);
          print_metadata = 1;
          break;

        default:
          break;
      }
    }

    SDL_Delay(1);
  }

  SDL_CloseAudio();

  gme_delete(emu);

  return 0;
}

