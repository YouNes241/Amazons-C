#include "config.h"
#include "controller.h"
#include "gui_application.h"
#include "input.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
parse_options (int argc, char *argv[], game_config_t *config)
{
  int opt;
  int option_index = 0;

  static struct option long_options[]
      = { { "size", required_argument, 0, 'n' },
          { "blitz", no_argument, 0, 'b' },
          { "time", required_argument, 0, 't' },
          { "contest", no_argument, 0, 'c' },
          { "gui", no_argument, 0, 'g' },
          { "ai", optional_argument, 0, 'a' },
          { "debug", no_argument, 0, 'd' },
          { "verbose", no_argument, 0, 'v' },
          { "help", no_argument, 0, 'h' },
          { "version", no_argument, 0, 'V' },
          { 0, 0, 0, 0 } };

  init_default_config (config);

  load_config_file (config);

  // getopt_long parcourt les arguments de la ligne de commande et les compare
  // aux options définies dans long_options.
  // il prend en arguments argc et argv, qui sont les paramètres standard de la
  // fonction main représentant le nombre d'arguments et les arguments
  // eux-mêmes. ainsi que une chaîne de caractères "n:bt:cga::dvVh" qui indique
  // les options courtes acceptées par le programme.
  while ((opt = getopt_long (argc, argv, "n:bt:cga::dvVh", long_options,
                             &option_index))
         != -1)
    {
      switch (opt)
        {
        case 'n':
          config->board_size = atoi (optarg);
          if (config->board_size < MIN_BOARD_SIZE
              || config->board_size > MAX_BOARD_SIZE)
            {
              fprintf (
                  stderr,
                  "Erreur : La taille du plateau doit etre entre %d et %d.\n",
                  MIN_BOARD_SIZE, MAX_BOARD_SIZE);
              return EXIT_FAILURE;
            }
          break;

        case 'b':
          config->blitz_mode = true;
          break;

        case 't':
          config->time_per_player = atoi (optarg);
          if (config->time_per_player <= 0 || config->time_per_player > 1800)
            {
              fprintf (stderr, "Erreur : Le temps par joueur doit etre un "
                               "entier positif entre 1 et 1800 secondes.\n");
              return EXIT_FAILURE;
            }
          break;

        case 'c':
          config->mode = MODE_CONTEST;
          break;

        case 'g':
          config->mode = MODE_GUI;
          break;

        case 'a':
          config->ai_enabled = true;
          break;

        case 'd':
        case 'v':

          config->verbose = true;
          break;

        case 'V':
          printf ("amazons-c version %s\n", VERSION);
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
  return EXIT_SUCCESS;
}

int
main (int argc, char *argv[])
{
  game_config_t *config = malloc (sizeof (game_config_t));

  int result = parse_options (argc, argv, config);

  if (result != EXIT_SUCCESS)
    {
      free (config);
      return result;
    }

  int status = EXIT_SUCCESS;

  if (config->mode == MODE_INTERACTIVE)
    {
      run_interactive_mode (config);
    }
  else if (config->mode == MODE_GUI)
    {
      GtkApplication *app = createApplication (config);
      status = runApplication (app, 0, NULL);
    }

  free (config);

  return status;
}