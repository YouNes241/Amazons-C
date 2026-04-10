/** @file main.c
 * @brief Main entry point for the Amazons game.
 */

#include "cli_parser.h"
#include "config.h"
#include "controller.h"
#include "gui_application.h"
#include "i18n.h"
#include "input.h"
#include "network_common.h"
#include "utils.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static NetworkManager *g_daemon_net_manager = NULL;

/** @brief Handles the shutdown signal for the daemon.
 * @param sig The signal number.
 */
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

/** @brief Main entry point for the Amazons game.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return The exit status of the program.
 */
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