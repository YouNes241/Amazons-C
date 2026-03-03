#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "input.h"
#include <readline/readline.h>
#include <readline/history.h>

void
init_default_config (game_config_t *config)
{
  config->board_size = DEFAULT_BOARD_SIZE;
  config->mode = MODE_INTERACTIVE;
  config->blitz_mode = false;
  config->time_per_player = DEFAULT_TIME_BLITZ;
  config->ai_enabled = false;
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
  printf ("IA activée        : %s\n",
          config->ai_enabled ? "Oui" : "Non");
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
          
          char *mode_str = (config->mode == MODE_CONTEST) ? "contest" : (config->mode == MODE_GUI) ? "gui" : "interactive";
          fprintf (file, "mode = %s\n", mode_str);

          fprintf (file, "blitz = %s\n", config->blitz_mode ? "true" : "false");
          fprintf (file, "timeout = %d\n", config->time_per_player);
          fprintf (file, "ai_enabled = %s\n", config->ai_enabled ? "true" : "false");
          fprintf (file, "verbose = %s\n", config->verbose ? "true" : "false");

          fprintf (file, "language = %s\n", (config->language == LANG_FR) ? "french" : "english");

          fclose (file);
          printf ("Note : Fichier .amazonsrc généré à partir de la configuration initiale.\n");
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
          else if (strcmp (key, "ai_enabled") == 0)
            {
              config->ai_enabled = (strcmp (value, "true") == 0);
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
          fprintf (stderr, "Avertissement : ligne invalide ignorée : %s", line);
        }
    }
  fclose (file);
}


static char *commands_list[] = {
  "new", "help", "quit", "load", "save", "pause", 
  "hint", "undo", "redo", "show", "set", "details", NULL
};

char *
command_generator (const char *text, int state)
{
  static int list_index, len;
  char *name;

  /* Si c'est le début d'une nouvelle recherche */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* On fouille dans la liste tant qu'il y a des mots */
  while ((name = commands_list[list_index++]))
    {
      /* Si le début du mot correspond à ce que l'utilisateur a tapé */
      if (strncmp (name, text, len) == 0)
        {
          /* On renvoie une copie. Readline l'affichera et fera le 'free' */
          return strdup (name);
        }
    }

  /* On n'a plus rien trouvé qui correspond */
  return NULL;
}

char **
amazons_completion (const char *text, int start, int end)
{
  /* Empêche Readline de chercher des fichiers si on ne trouve pas de commande */
  rl_attempted_completion_over = 1;

  /* On ne veut compléter que le premier mot de la ligne */
  /* Si start == 0, l'utilisateur est au tout début de sa saisie */
  if (start == 0)
    {
      return rl_completion_matches (text, command_generator);
    }
  
  return NULL;
}


int
main (int argc, char *argv[])
{
  game_config_t config;
  int opt;
  int option_index = 0;

  
  static struct option long_options[] =
    {
      {"size",    required_argument, 0, 'n'},
      {"blitz",   no_argument,       0, 'b'},
      {"time",    required_argument, 0, 't'},
      {"contest", no_argument,       0, 'c'},
      {"gui",     no_argument,       0, 'g'},
      {"ai",      optional_argument, 0, 'a'},
      {"debug",   no_argument,       0, 'd'},
      {"verbose", no_argument,       0, 'v'},
      {"help",    no_argument,       0, 'h'},
      {"version",    no_argument,       0, 'V'},
      {0, 0, 0, 0}
    };

  init_default_config (&config);

  load_config_file (&config);

  // getopt_long parcourt les arguments de la ligne de commande et les compare aux options définies dans long_options. 
  //il prend en arguments argc et argv, qui sont les paramètres standard de la fonction main représentant le nombre d'arguments et les arguments eux-mêmes.
  //ainsi que une chaîne de caractères "n:bt:cga::dvVh" qui indique les options courtes acceptées par le programme.
  while ((opt = getopt_long (argc, argv, "n:bt:cga::dvVh", long_options, &option_index)) != -1)
    {
      switch (opt)
        {
        case 'n':
          config.board_size = atoi (optarg);
          if (config.board_size < MIN_BOARD_SIZE || config.board_size > MAX_BOARD_SIZE)
            {
              fprintf (stderr, "Erreur : La taille du plateau doit etre entre %d et %d.\n", 
                       MIN_BOARD_SIZE, MAX_BOARD_SIZE);
              return EXIT_FAILURE;
            }
          break;

        case 'b':
          config.blitz_mode = true;
          break;

        case 't':
          config.time_per_player = atoi (optarg);
          if (config.time_per_player <= 0 || config.time_per_player > 1800) 
            { 
              fprintf (stderr, "Erreur : Le temps par joueur doit etre un entier positif entre 1 et 1800 secondes.\n"); 
              return EXIT_FAILURE; 
            }
          break;

        case 'c':
          config.mode = MODE_CONTEST;
          break;

        case 'g':
          config.mode = MODE_GUI;
          break;

        case 'a':
          config.ai_enabled = true;
          break;

        case 'd':
        case 'v':
          
          config.verbose = true;
          break;

        case 'V':
          printf("amazons-c version %s\n", VERSION);
          return EXIT_SUCCESS;

        case 'h':
          printf ("Usage: ./amazons [OPTIONS]\n");
          printf ("  -n, --size N    Taille du plateau (%d-%d, defaut: %d)\n", 
                  MIN_BOARD_SIZE, MAX_BOARD_SIZE, DEFAULT_BOARD_SIZE);
          printf ("  -b, --blitz     Activer le mode blitz\n");
          printf ("  -t, --time T    Temps par joueur en secondes\n");
          printf ("  -c, --contest   Lancer en mode contest\n");
          printf ("  -g, --gui       Lancer en mode graphique\n");
          printf ("  -a, --ai [C]    Activer l'IA\n");
          printf ("  -d, --debug     Afficher les messages de debug\n");
          printf ("  -v, --verbose   Activer le mode bavard\n");
          printf ("  -h, --help      Afficher cette aide\n");
          printf ("  -V, --version   Affiche la version du jeu\n");
          return EXIT_SUCCESS;

          default:
          return EXIT_FAILURE;
        }
    }


  if (optind < argc)
    {
      fprintf (stderr, "Erreur : argument inconnu '%s'\n", argv[optind]);
      return EXIT_FAILURE;
    }

  rl_attempted_completion_function = amazons_completion;
    char* input_buffer; //buffer pour contenir les commandes de l'utilisateur
    bool game_running = true;
    pos_t move_pos; //structure pour stocker les coordonnées d'un coup  (temporaire)

while (game_running)
    {
      input_buffer = readline (">> ");

      /* Si l'utilisateur fait Ctrl+D, readline renvoie NULL */
      if (input_buffer == NULL)
        {
          printf ("\nSortie du programme.\n");
          break;
        }

      /* Si la chaîne est vide, on libère et on continue */
      if (input_buffer[0] == '\0')
        {
          free (input_buffer);
          continue;
        }

      /* On ajoute la commande à l'historique*/
      add_history (input_buffer);

      if (strcmp (input_buffer, "quit") == 0)
        {
          game_running = false;
        }
      else if (strcmp (input_buffer, "pause") == 0)
        {
          printf ("Jeu en pause...\n");
        }
      else if (strcmp (input_buffer, "hint") == 0)
        {
          printf ("Conseil : (IA en cours de calcul...)\n");
        }
      else if (strncmp (input_buffer, "help", 4) == 0)
        {
          printf ("Aide : new, help, quit, load, save, pause, hint, undo, redo, show, set\n");
        }
      else if (strncmp (input_buffer, "new", 3) == 0)
        {
          printf ("Nouvelle partie : %s\n", &input_buffer[3]);
        }
      else if (strncmp (input_buffer, "load ", 5) == 0)
        {
          printf ("Chargement de : %s\n", &input_buffer[5]);
        }
      else if (strncmp (input_buffer, "save ", 5) == 0)
        {
          printf ("Sauvegarde dans : %s\n", &input_buffer[5]);
        }
      else if (strncmp (input_buffer, "undo", 4) == 0)
        {
          printf ("Annulation : %s\n", &input_buffer[4]);
        }
      else if (strncmp (input_buffer, "redo", 4) == 0)
        {
          printf ("Action rejouée : %s\n", &input_buffer[4]);
        }
      else if (strncmp (input_buffer, "show ", 5) == 0)
        {
          printf ("Affichage de : %s\n", &input_buffer[5]);
        }
      else if (strncmp (input_buffer, "set ", 4) == 0)
        {
          printf ("Configuration : %s\n", &input_buffer[4]);
        }
      else if (strncmp (input_buffer, "details", 7) == 0)
        {
          display_config (&config);
        }
      else
        {
          /* On vérifie si c'est un mouvement (ex: e2-e4) */
          if (parse_coordinates (input_buffer, &move_pos) == 0)
            {
              printf ("Coup valide reçu.\n");
            }
          else
            {
              fprintf (stderr, "Erreur : commande inconnue '%s'\n", input_buffer);
            }
        }

      free (input_buffer);
    }

  return EXIT_SUCCESS;
}