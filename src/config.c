#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_SIZE 512
#define LINE_SIZE 128
#define KEY_SIZE 64
#define VAL_SIZE 64

void
init_default_config (game_config_t *config)
{
  config->board_size = DEFAULT_BOARD_SIZE;
  config->mode = MODE_INTERACTIVE;
  config->blitz_mode = false;
  config->time_per_player = DEFAULT_TIME_BLITZ;
  config->verbose = false;
  config->language = LANG_FR;

  config->ai_white_cfg.enabled = false;
  config->ai_white_cfg.param_count = 0;
  strncpy (config->ai_white_cfg.name, "minimax", MAX_AI_STR_LEN - 1);

  config->ai_black_cfg.enabled = false;
  config->ai_black_cfg.param_count = 0;
  strncpy (config->ai_black_cfg.name, "minimax", MAX_AI_STR_LEN - 1);
}

void
display_config (game_config_t *config)
{
  printf ("\n--- CURRENT CONFIGURATION ---\n");
  printf ("Game mode         : ");
  switch (config->mode)
    {
    case MODE_INTERACTIVE:
      printf ("Interactive\n");
      break;
    case MODE_CONTEST:
      printf ("Contest\n");
      break;
    case MODE_GUI:
      printf ("GUI\n");
      break;
    default:
      printf ("Unknown\n");
      break;
    }
  printf ("\n--- CURRENT CONFIGURATION ---\n");
  printf ("Board size        : %d x %d\n", config->board_size,
          config->board_size);
  printf ("Blitz mode        : %s\n",
          config->blitz_mode ? "Enabled" : "Disabled");
  printf ("Time per player   : %d s\n", config->time_per_player);

  if (config->ai_white_cfg.enabled)
    printf ("White player AI   : Yes (%s, %d parameter(s))\n",
            config->ai_white_cfg.name, config->ai_white_cfg.param_count);
  else
    printf ("White player AI   : No\n");

  if (config->ai_black_cfg.enabled)
    printf ("Black player AI   : Yes (%s, %d parameter(s))\n",
            config->ai_black_cfg.name, config->ai_black_cfg.param_count);
  else
    printf ("Black player AI   : No\n");

  printf ("Verbose mode      : %s\n",
          config->verbose ? "Enabled" : "Disabled");
  printf ("Language          : %s\n",
          config->language == LANG_FR ? "French" : "English");
  printf ("------------------------------\n");
}

void
get_config_path (char *path, size_t size)
{
  /* getenv cherche la valeur de la variable d'environnement HOME */
  char *home_dir = getenv ("HOME");

  if (home_dir == NULL)
    {
      snprintf (path, size, ".amazonsrc");
    }
  else
    {
      snprintf (path, size, "%s/.amazonsrc", home_dir);
    }
}

void
load_config_file (game_config_t *config)
{
  char path[PATH_SIZE];
  get_config_path (path, sizeof (path));

  FILE *file = fopen (path, "r");

  if (file == NULL)
    {
      file = fopen (path, "w");
      if (file != NULL)
        {
          fprintf (file, "# Amazons configuration file v%s\n", VERSION);
          fprintf (file, "[defaults]\n");

          fprintf (file, "board_size = %d\n", config->board_size);

          char *mode_str = (config->mode == MODE_CONTEST) ? "contest"
                           : (config->mode == MODE_GUI)   ? "gui"
                                                          : "interactive";
          fprintf (file, "mode = %s\n", mode_str);

          fprintf (file, "blitz = %s\n",
                   config->blitz_mode ? "true" : "false");
          fprintf (file, "timeout = %d\n", config->time_per_player);

          // Save AI name, else false
          fprintf (file, "ai_white = %s\n",
                   config->ai_white_cfg.enabled ? config->ai_white_cfg.name
                                                : "false");
          fprintf (file, "ai_black = %s\n",
                   config->ai_black_cfg.enabled ? config->ai_black_cfg.name
                                                : "false");

          fprintf (file, "verbose = %s\n", config->verbose ? "true" : "false");

          fprintf (file, "language = %s\n",
                   (config->language == LANG_FR) ? "french" : "english");

          fclose (file);
          printf ("Note: .amazonsrc file generated from the "
                  "initial configuration.\n");
        }
      return;
    }

  char line[LINE_SIZE];

  while (fgets (line, sizeof (line), file))
    {
      char key[KEY_SIZE], value[VAL_SIZE];

      if (line[0] == '[' || line[0] == '\n' || line[0] == '#')
        continue;

      if (sscanf (line, "%63s = %63s", key, value) == 2)
        {
          if (strcmp (key, "board_size") == 0)
            {
              int val = atoi (value);
              if (val >= MIN_BOARD_SIZE && val <= MAX_BOARD_SIZE)
                config->board_size = val;
            }
          else if (strcmp (key, "mode") == 0)
            {
              if (strcmp (value, "interactive") == 0)
                config->mode = MODE_INTERACTIVE;
              else if (strcmp (value, "contest") == 0)
                config->mode = MODE_CONTEST;
              else if (strcmp (value, "gui") == 0)
                config->mode = MODE_GUI;
            }
          else if (strcmp (key, "blitz") == 0)
            {
              config->blitz_mode = (strcmp (value, "true") == 0);
            }
          else if (strcmp (key, "timeout") == 0)
            {
              config->time_per_player = atoi (value);
            }
          else if (strcmp (key, "ai_white") == 0)
            {
              if (strcmp (value, "false") == 0)
                {
                  config->ai_white_cfg.enabled = false;
                }
              else
                {
                  config->ai_white_cfg.enabled = true;

                  if (strcmp (value, "true") == 0)
                    {
                      snprintf (config->ai_white_cfg.name, MAX_AI_STR_LEN,
                                "minimax");
                    }
                  else
                    {
                      snprintf (config->ai_white_cfg.name, MAX_AI_STR_LEN,
                                "%.31s", value);
                    }
                }
            }
          else if (strcmp (key, "ai_black") == 0)
            {
              if (strcmp (value, "false") == 0)
                {
                  config->ai_black_cfg.enabled = false;
                }
              else
                {
                  config->ai_black_cfg.enabled = true;
                  if (strcmp (value, "true") == 0)
                    {
                      snprintf (config->ai_black_cfg.name, MAX_AI_STR_LEN,
                                "minimax");
                    }
                  else
                    {
                      snprintf (config->ai_black_cfg.name, MAX_AI_STR_LEN,
                                "%.31s", value);
                    }
                }
            }
          else if (strcmp (key, "verbose") == 0)
            {
              config->verbose = (strcmp (value, "true") == 0);
            }
          else if (strcmp (key, "language") == 0)
            {
              if (strcmp (value, "french") == 0)
                config->language = LANG_FR;
              else if (strcmp (value, "english") == 0)
                config->language = LANG_EN;
            }
        }
      else
        {
          fprintf (stderr, "Warning: invalid line ignored: %s", line);
        }
    }
  fclose (file);
}
