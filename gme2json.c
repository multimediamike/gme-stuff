/*
 * To compile:
 *   gcc -Wall gme2json.c -o gme2json -lgme
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gme/gme.h>

#define CONTAINER_STRING "Game Music Files"
#define CONTAINER_STRING_SIZE 16

/* Print a string surrounded by quotes while escaping any quotes or
 * backslashes encountered within the string. */
void escape_print(const char *str)
{
  int i = 0;

  if (!strlen(str))
  {
    printf("null");
    return;
  }

  putchar('"');
  while (str[i])
  {
    if ((str[i] == '"') || (str[i] == '\\'))
      putchar('\\');
    putchar(str[i]);
    i++;
  }
  putchar('"');
}

void print_meta_strings(gme_info_t *info, const char *indent)
{
  printf("%s\"system\": ",    indent);  escape_print(info->system);     printf(",\n");
  printf("%s\"game\": ",      indent);  escape_print(info->game);       printf(",\n");
  printf("%s\"song\": ",      indent);  escape_print(info->song);       printf(",\n");
  printf("%s\"author\": ",    indent);  escape_print(info->author);     printf(",\n");
  printf("%s\"copyright\": ", indent);  escape_print(info->copyright);  printf(",\n");
  printf("%s\"comment\": ",   indent);  escape_print(info->comment);    printf(",\n");
  printf("%s\"dumper\": ",    indent);  escape_print(info->dumper);     printf(",\n");

  printf("%s\"length\": %d,\n",       indent, info->length);
  printf("%s\"intro_length\": %d,\n", indent, info->intro_length);
  printf("%s\"loop_length\": %d,\n",  indent, info->loop_length);
  printf("%s\"play_length\": %d\n",   indent, info->play_length);
}

int load_conventional_gme_file(char *filename)
{
  Music_Emu *emu;
  gme_info_t *info;
  int track_count;
  gme_err_t err;
  int i;

  /* ask the library to only open the file for informational purposes */
  err = gme_open_file(filename, &emu, gme_info_only);
  if (err)
  {
    printf("%s: %s\n", filename, err);
    return 2;
  }

  track_count = gme_track_count(emu);
  printf("{\n");
  printf("  \"track_count\": %d,\n", track_count);
  printf("  \"tracks\":\n  [\n");
  for (i = 0; i < track_count; i++)
  {
    err = gme_track_info(emu, &info, i);
    if (err)
    {
      printf("ERROR\n");
      return 2;
    }
    printf("    {\n");
    print_meta_strings(info, "      ");
    printf("    }");
    if (i < track_count - 1)
      printf(",");
    printf("\n");
    gme_free_info(info);
  }
  printf("  ]\n");
  printf("}\n");

  gme_delete(emu);

  return 0;  /* success */
}

int load_gamemusic_container_file(char *filename)
{
  Music_Emu *emu;
  gme_info_t *info;
  int track_count;
  gme_err_t err;
  int i;
  FILE *f;
  unsigned char *filedata;
  size_t filesize;
  unsigned int offset;
  int index;

  f = fopen(filename, "rb");
  if (!f)
  {
    perror(filename);
    return 2;
  }
  fseek(f, 0, SEEK_END);
  filesize = ftell(f);
  fseek(f, 0, SEEK_SET);

  filedata = (unsigned char*)malloc(filesize);
  if (!filedata)
  {
    printf("failed to allocate memory\n");
    fclose(f);
    return 3;
  }

  if (fread(filedata, filesize, 1, f) != 1)
  {
    printf("failed to read data\n");
    fclose(f);
    return 3;
  }
  fclose(f);

  index = 0x10;
  track_count = (filedata[index + 0] << 24) | (filedata[index + 1] << 16) |
                (filedata[index + 2] <<  8) |  filedata[index + 3];

  index += 4;
  printf("{\n");
  printf("  \"track_count\": %d,\n", track_count);
  printf("  \"tracks\":\n  [\n");
  for (i = 0; i < track_count; i++)
  {
    offset = (filedata[index + 0] << 24) | (filedata[index + 1] << 16) |
             (filedata[index + 2] <<  8) |  filedata[index + 3];
    index += 4;

    /* open file in memory; cheat with the length -- just tell GME it can
     * read until the end of the buffer */
    err = gme_open_data(&filedata[offset], filesize - offset, &emu, gme_info_only);
    if (err)
    {
      printf("%s: %s\n", filename, err);
      free(filedata);
      return 2;
    }

    err = gme_track_info(emu, &info, 0);
    if (err)
    {
      printf("ERROR\n");
      free(filedata);
      return 2;
    }
    printf("    {\n");
    print_meta_strings(info, "      ");
    printf("    }");
    if (i < track_count - 1)
      printf(",");
    printf("\n");
    gme_free_info(info);

    gme_delete(emu);
  }

  printf("  ]\n");
  printf("}\n");
  free(filedata);

  return 0;  /* success */
}

int main(int argc, char *argv[])
{
  char signature_check[CONTAINER_STRING_SIZE];
  FILE *f;

  if (argc < 2)
  {
    printf("USAGE: gme2json <file>\n");
    return 1;
  }

  /* first, check if it's a special .gamemusic container */
  f = fopen(argv[1], "rb");
  if (!f)
  {
    perror(argv[1]);
    return 2;
  }
  if (fread(signature_check, CONTAINER_STRING_SIZE, 1, f) != 1)
  {
    perror(argv[1]);
    return 2;
  }
  fclose(f);

  if (strncmp(signature_check, CONTAINER_STRING, CONTAINER_STRING_SIZE) == 0)
    return load_gamemusic_container_file(argv[1]);
  else
    return load_conventional_gme_file(argv[1]);
}

