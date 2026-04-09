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

typedef enum
{
  OPT_DAEMON = 1000,
  OPT_AI_WHITE,            // 1001
  OPT_AI_BLACK,            // 1002
  OPT_AI_MODE,             // 1003
  OPT_AI_MINIMAX_DEPTH,    // 1004
  OPT_AI_MINIMAX_SCORING,  // 1005
  OPT_AI_MCTS_SELECTION,   // 1006
  OPT_AI_TIME,             // 1007
  OPT_AI_MINIMAX_FWPRUNING // 1008
} CliOption;

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

// Utility function to easily add a dynamic parameter to the current AI
static void
add_ai_param (ai_config_t *ai, const char *key, const char *val)
{
  if (ai->param_count < MAX_AI_PARAMS)
    {
      strncpy (ai->param_keys[ai->param_count], key, MAX_AI_STR_LEN - 1);
      ai->param_keys[ai->param_count][MAX_AI_STR_LEN - 1] = '\0';

      strncpy (ai->param_vals[ai->param_count], val, MAX_AI_STR_LEN - 1);
      ai->param_vals[ai->param_count][MAX_AI_STR_LEN - 1] = '\0';

      ai->param_count++;
    }
}

int
parse_options (int argc, char *argv[], game_config_t *config)
{
  bool time_arg_passed = false;
  int opt;
  int option_index = 0;
  bool d_short_passed = false;

  // Default target is the black AI (human plays white by default)
  ai_config_t *current_ai = &config->ai_black_cfg;

  static struct option long_options[] = {
    { "size", required_argument, 0, 'n' },
    { "blitz", no_argument, 0, 'b' },
    { "time", required_argument, 0, 't' },
    { "contest", no_argument, 0, 'c' },
    { "gui", no_argument, 0, 'g' },
    { "ai-white", no_argument, 0, OPT_AI_WHITE },
    { "ai-black", no_argument, 0, OPT_AI_BLACK },
    { "ai-mode", required_argument, 0, OPT_AI_MODE },
    { "ai-minimax-depth", required_argument, 0, OPT_AI_MINIMAX_DEPTH },
    { "ai-minimax-scoring", required_argument, 0, OPT_AI_MINIMAX_SCORING },
    { "ai-minimax-fwpruning", required_argument, 0, OPT_AI_MINIMAX_FWPRUNING },
    { "ai-mcts-selection", required_argument, 0, OPT_AI_MCTS_SELECTION },
    { "ai-time", required_argument, 0, OPT_AI_TIME },
    { "debug", no_argument, 0, 'd' },
    { "server", optional_argument, 0, 's' },
    { "daemon", no_argument, 0, OPT_DAEMON },
    { "verbose", no_argument, 0, 'v' },
    { "help", no_argument, 0, 'h' },
    { "version", no_argument, 0, 'V' },
    { 0, 0, 0, 0 }
  };

  init_default_config (config);
  load_config_file (config);

  while ((opt = getopt_long (argc, argv, "n:bt:cgs::dvVh", long_options,
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

        case OPT_AI_WHITE:
          current_ai = &config->ai_white_cfg;
          current_ai->enabled = true;
          break;

        case OPT_AI_BLACK:
          current_ai = &config->ai_black_cfg;
          current_ai->enabled = true;
          break;

        case OPT_AI_MODE:
          current_ai->enabled = true;

          strncpy (current_ai->name, optarg, MAX_AI_STR_LEN - 1);
          current_ai->name[MAX_AI_STR_LEN - 1] = '\0';

          if (strcmp (optarg, "iterative") == 0)
            {
              add_ai_param (current_ai, "iterative_deepening", "1");
            }
          break;
        case OPT_AI_MINIMAX_DEPTH:
          current_ai->enabled = true;
          add_ai_param (current_ai, "depth", optarg);
          break;

        case OPT_AI_MINIMAX_SCORING:
          current_ai->enabled = true;
          add_ai_param (current_ai, "evaluator", optarg);
          break;

        case OPT_AI_MINIMAX_FWPRUNING:
          current_ai->enabled = true;
          if (strcmp (optarg, "0") == 0 || strcasecmp (optarg, "false") == 0
              || strcasecmp (optarg, "disabled") == 0)
            {
              add_ai_param (current_ai, "forward_pruning", "0");
            }
          else
            {
              add_ai_param (current_ai, "forward_pruning", "1");
            }
          break;

        case OPT_AI_MCTS_SELECTION:
          current_ai->enabled = true;
          add_ai_param (current_ai, "selection", optarg);
          break;

        case OPT_AI_TIME:
          current_ai->enabled = true;
          {
            int time_sec = atoi (optarg);
            char time_ms_str[32];
            snprintf (time_ms_str, sizeof (time_ms_str), "%d",
                      time_sec * 1000);
            add_ai_param (current_ai, "time_limit", time_ms_str);
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
          free (config);
          exit (EXIT_SUCCESS);

        case 'h':
          printf ("Usage: ./amazons [OPTIONS]\n");
          printf ("  -n, --size N    Board size (%d-%d, default: %d)\n",
                  MIN_BOARD_SIZE, MAX_BOARD_SIZE, DEFAULT_BOARD_SIZE);
          printf ("  -b, --blitz     Enable blitz mode\n");
          printf ("  -t, --time T    Thinking time per player in minutes "
                  "(default: 30)\n");
          printf ("  -c, --contest   Launch in contest mode\n");
          printf ("  -g, --gui       Launch in graphical mode\n");

          printf ("\n  --- AI Options ---\n");
          printf ("  --ai-white      Apply the following AI options to the "
                  "White player\n");
          printf ("  --ai-black      Apply the following AI options to the "
                  "Black player\n");
          printf ("  --ai-mode M     M = minimax, iterative, mcts\n");
          printf ("  --ai-minimax-depth D   Maximum depth for minimax\n");
          printf ("  --ai-minimax-scoring S Evaluation function (me, rth, "
                  "qdkd)\n");
          printf ("  --ai-mcts-selection S  MCTS selection (uct, ml)\n");
          printf ("  --ai-time T     AI thinking time in seconds\n");

          printf ("\n  --- Other ---\n");
          printf ("  -d, --debug     Display debug messages\n");
          printf ("  -s, --server [P] Launch as server (optional Port)\n");
          printf ("  --daemon        Launch server in background "
                  "(Headless)\n");
          printf ("  -v, --verbose   Enable verbose mode\n");
          printf ("  -h, --help      Display this help\n");
          printf ("  -V, --version   Display game version\n");
          free (config);
          exit (EXIT_SUCCESS);

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
      printf ("\n[DAEMON] Starting server in Headless mode...\n");
      printf ("[DAEMON] The terminal will return control to you. Type 'pkill "
              "amazons' later to stop it.\n");

      if (daemon (1, 0) == -1)
        {
          perror ("Error creating daemon");
          return EXIT_FAILURE;
        }
      NetworkManager net_manager;
      NetworkEvents callbacks = { 0 };
      NetworkManager_Init (&net_manager, callbacks);
      g_daemon_net_manager = &net_manager;
      signal (SIGINT, handle_shutdown_signal);
      signal (SIGTERM, handle_shutdown_signal);

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
