#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
init_default_config (game_config_t *config)
{
  config->board_size = DEFAULT_BOARD_SIZE;
  config->mode = MODE_INTERACTIVE;
  config->blitz_mode = false;
  config->time_per_player = DEFAULT_TIME_BLITZ;
  config->ai_black = false;
  config->ai_white = false;
  config->verbose = false;
  config->language = LANG_FR;
}

void
display_config (game_config_t *config)
{
  printf ("\n--- CONFIGURATION ACTUELLE ---\n");
  printf ("Mode de jeu       : ");
  switch (config->mode)
    {
    case MODE_INTERACTIVE:
      printf ("Interactif\n");
      break;
    case MODE_CONTEST:
      printf ("Contest\n");
      break;
    case MODE_GUI:
      printf ("GUI\n");
      break;
    default:
      printf ("Inconnu\n");
      break;
    }
  printf ("\n--- CONFIGURATION ACTUELLE ---\n");
  printf ("Taille du plateau : %d x %d\n", config->board_size,
          config->board_size);
  printf ("Mode Blitz        : %s\n",
          config->blitz_mode ? "Activé" : "Désactivé");
  printf ("Temps par joueur  : %d s\n", config->time_per_player);
  printf ("IA joueur Blanc        : %s\n", config->ai_white ? "Oui" : "Non");
  printf ("IA joueur Noir        : %s\n", config->ai_black ? "Oui" : "Non");
  printf ("Mode Verbose      : %s\n",
          config->verbose ? "Activé" : "Désactivé");
  printf ("Langue : %s\n",
          config->language == LANG_FR ? "Français" : "Anglais");
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
  char path[512];
  get_config_path (path, sizeof (path));

  FILE *file = fopen (path, "r");

  if (file == NULL)
    {
      file = fopen (path, "w");
      if (file != NULL)
        {
          fprintf (file, "# Fichier de configuration Amazons v%s\n", VERSION);
          fprintf (file, "[defaults]\n");

          fprintf (file, "board_size = %d\n", config->board_size);

          char *mode_str = (config->mode == MODE_CONTEST) ? "contest"
                           : (config->mode == MODE_GUI)   ? "gui"
                                                          : "interactive";
          fprintf (file, "mode = %s\n", mode_str);

          fprintf (file, "blitz = %s\n",
                   config->blitz_mode ? "true" : "false");
          fprintf (file, "timeout = %d\n", config->time_per_player);
          fprintf (file, "ai_white = %s\n",
                   config->ai_white ? "true" : "false");
          fprintf (file, "ai_black = %s\n",
                   config->ai_black ? "true" : "false");
          fprintf (file, "verbose = %s\n", config->verbose ? "true" : "false");

          fprintf (file, "language = %s\n",
                   (config->language == LANG_FR) ? "french" : "english");

          fclose (file);
          printf ("Note : Fichier .amazonsrc généré à partir de la "
                  "configuration initiale.\n");
        }
      return;
    }

  char line[128];

  while (fgets (line, sizeof (line), file))
    {
      char key[64], value[64];

      if (line[0] == '[' || line[0] == '\n' || line[0] == '#')
        continue;

      if (sscanf (line, "%s = %s", key, value) == 2)
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
              config->ai_white = (strcmp (value, "true") == 0);
            }
          else if (strcmp (key, "ai_black") == 0)
            {
              config->ai_black = (strcmp (value, "true") == 0);
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
          fprintf (stderr, "Avertissement : ligne invalide ignorée : %s",
                   line);
        }
    }
  fclose (file);
}