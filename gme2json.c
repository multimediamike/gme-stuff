/*
 * To compile:
 *   gcc -Wall gme2json.c -o gme2json -lgme
 */
#include <stdio.h>
#include <gme/gme.h>

/* Print a string surrounded by quotes while escaping any quotes encountered
 * within the string. */
void escape_print(const char *str)
{
  int i = 0;

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
  printf("%s\"dumper\": ",    indent);  escape_print(info->dumper);     printf("\n");
}

int main(int argc, char *argv[])
{
  Music_Emu *emu;
  gme_info_t *info;
  int track_count;
  gme_err_t err;
  int i;

  if (argc < 2)
  {
    printf("USAGE: gme2json <file>\n");
    return 1;
  }

  /* ask the library to only open the file for informational purposes */
  err = gme_open_file(argv[1], &emu, gme_info_only);
  if (err)
  {
    printf("%s: %s\n", argv[1], err);
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
  printf("  ],\n");
  err = gme_track_info(emu, &info, 0);
  if (err)
  {
    printf("ERROR\n");
    return 2;
  }
  print_meta_strings(info, "  ");
  gme_free_info(info);
  printf("}\n");

  gme_delete(emu);

  return 0;
}

