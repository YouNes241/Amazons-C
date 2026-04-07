#include "config.h"
#include "controller.h"
#include "gui_application.h"
#include "i18n.h"
#include "input.h"
#include "network_common.h"
#include "utils.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static NetworkManager *g_daemon_net_manager = NULL;

void
handle_shutdown_signal (int sig)
{
  (void)sig;
  if (g_daemon_net_manager)
    {

      NetworkManager_ShutdownServer (g_daemon_net_manager);
    }
  exit (EXIT_SUCCESS);
}

int
parse_options (int argc, char *argv[], game_config_t *config)
{
  bool time_arg_passed = false;
  int opt;
  int option_index = 0;
  bool d_short_passed = false;
  static struct option long_options[]
      = { { "size", required_argument, 0, 'n' },
          { "blitz", no_argument, 0, 'b' },
          { "time", required_argument, 0, 't' },
          { "contest", no_argument, 0, 'c' },
          { "gui", no_argument, 0, 'g' },
          { "ai", optional_argument, 0, 'a' },
          { "debug", no_argument, 0, 'd' },
          { "server", optional_argument, 0, 's' },
          { "daemon", no_argument, 0, 1000 },
          { "verbose", no_argument, 0, 'v' },
          { "help", no_argument, 0, 'h' },
          { "version", no_argument, 0, 'V' },
          { 0, 0, 0, 0 } };

  init_default_config (config);

  load_config_file (config);

  // getopt_long parses command-line arguments and compares them to the options
  // defined in long_options. It takes argc and argv as arguments (the standard
  // main function parameters representing the number of arguments and the
  // arguments themselves), plus a string "n:bt:cga::dvVh" indicating the
  // short options accepted by the program
  while ((opt = getopt_long (argc, argv, "n:bt:cga::s::dvVh", long_options,
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
          time_arg_passed = true;
          int minutes = atoi (optarg);
          config->time_per_player = minutes * 60;

          if (minutes <= 0 || minutes > 60)
            {
              fprintf (stderr,
                       _ ("ERROR : Time must be between 1 and 60 minutes.\n"));
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
          // By default, if the option is present, activate AI for Black
          config->ai_white = false;
          config->ai_black = true;

          // optarg contains the argument if user specified -aW or --ai=W
          if (optarg != NULL)
            {
              if (strcmp (optarg, "W") == 0)
                {
                  config->ai_white = true;
                  config->ai_black = false;
                }
              else if (strcmp (optarg, "B") == 0)
                {
                  config->ai_white = false;
                  config->ai_black = true;
                }
              else if (strcmp (optarg, "A") == 0)
                {
                  config->ai_white = true;
                  config->ai_black = true;
                }
              else
                {
                  if (strcmp (optarg, "emon") == 0)
                    {
                      fprintf (stderr,
                               "Error : To launch daemon mode, use the option "
                               "'--daemon' or '-d'.\n");
                      return EXIT_FAILURE;
                    }
                }
            }
          break;
        case 's':
          config->start_server = true;
          if (optarg)
            {
              config->server_port = atoi (optarg);
            }
          else if (optind < argc && argv[optind][0] != '-')
            {
              config->server_port = atoi (argv[optind++]);
            }
          else
            {
              config->server_port = 12345;
            }
          break;

        case 1000:
          config->daemon_mode = true;
          break;

        case 'd':

          d_short_passed = true;
          break;
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
          printf ("  -t, --time T     Temps de réflexion par joueur en "
                  "minutes (défaut: 30)\n");
          printf ("  -c, --contest   Lancer en mode contest\n");
          printf ("  -g, --gui       Lancer en mode graphique\n");
          printf ("  -a, --ai [C]    Activer l'IA\n");
          printf ("  -d, --debug     Afficher les messages de debug\n");
          printf ("  -s, --server [P] Lancer en tant que serveur (Port "
                  "optionnel)\n");
          printf ("  -d, --daemon    Lancer le serveur en arrière-plan "
                  "(Headless)\n");
          printf ("  -v, --verbose   Activer le mode bavard\n");
          printf ("  -h, --help      Afficher cette aide\n");
          printf ("  -V, --version   Affiche la version du jeu\n");
          return EXIT_SUCCESS;

        default:
          return EXIT_FAILURE;
        }
    }

  if (time_arg_passed && !config->blitz_mode)
    {
      printf (
          _ ("Warning: --time ignored because blitz mode is not active.\n"));
      config->time_per_player = DEFAULT_TIME_BLITZ;
    }

  if (optind < argc)
    {
      fprintf (stderr, "Erreur : argument inconnu '%s'\n", argv[optind]);
      return EXIT_FAILURE;
    }

  if (d_short_passed)
    {
      if (config->start_server)
        {
          // Si le serveur est demandé, -d devient Daemon
          config->daemon_mode = true;
        }
      else
        {
          config->verbose = true;
        }
    }
  return EXIT_SUCCESS;
}

int
main (int argc, char *argv[])
{
  char *env_lang = getenv ("LANG");
  if (env_lang != NULL)
    {
      if (strncmp (env_lang, "fr", 2) != 0 && strncmp (env_lang, "en", 2) != 0
          && strncmp (env_lang, "C", 1) != 0)
        {
          printf ("Warning: The requested language '%s' is not supported.\n",
                  env_lang);
          printf ("The game will start in English (default).\n\n");
        }
    }

  setlocale (LC_ALL, "");

  bindtextdomain ("amazons", "./locale");

  textdomain ("amazons");

  game_config_t *config = malloc (sizeof (game_config_t));
  config->start_server = false;
  config->daemon_mode = false;
  config->server_port = 12345;
  seed_xoshiro (time (NULL));

  int result = parse_options (argc, argv, config);

  if (result != EXIT_SUCCESS)
    {
      free (config);
      return result;
    }

  if (config->daemon_mode)
    {
      printf ("\n[DAEMON] Démarrage du serveur en mode Headless...\n");
      printf ("[DAEMON] Le terminal va vous rendre la main. Tapez 'pkill "
              "amazons-c' plus tard pour l'arrêter.\n");

      if (daemon (1, 0) == -1)
        {
          perror ("Erreur lors de la création du daemon");
          return EXIT_FAILURE;
        }
      NetworkManager net_manager;
      NetworkEvents callbacks = { 0 }; // no use for callbacks in daemon mode,
                                       // but we need to pass something valid
      NetworkManager_Init (&net_manager, callbacks);
      g_daemon_net_manager = &net_manager;
      signal (SIGINT,
              handle_shutdown_signal); // capture the Ctrl+C signal to allow
                                       // graceful shutdown in daemon mode
      signal (SIGTERM,
              handle_shutdown_signal); // Capture SIGTERM for graceful shutdown
                                       // when using 'pkill'

      if (NetworkManager_ServerStart (&net_manager, config->server_port) == 0)
        {
          while (1)
            {
              NetworkManager_Tick (&net_manager);
              usleep (50000);
            }
        }
      free (config);
      return EXIT_SUCCESS;
    }

  int status = EXIT_SUCCESS;

  if (config->mode == MODE_INTERACTIVE)
    {
      run_interactive_mode (config);
    }
  else if (config->mode == MODE_GUI)
    {
      run_gui_mode (config);
    }

  free (config);

  return status;
}