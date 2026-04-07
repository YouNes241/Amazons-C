/**
 * @file cli_controller.c
 * @brief Implementation of the main CLI controller for the Amazons game.
 *
 * This file contains the implementation of the main loop and logic for the
 * command-line interface (CLI) version of the Amazons game. It manages user
 * input, game state updates, AI moves, and network interactions when in
 * multiplayer mode. The controller uses the readline library for input
 * handling and supports both local and networked gameplay.
 */

#include "cli_controller.h"
#include "ai_factory.h"
#include "ai_interface.h"
#include "cli_commands.h"
#include "cli_network_events.h"
#include "cli_view.h"
#include "controller.h"
#include "game_engine.h"
#include "i18n.h"
#include "input.h"
#include "network_common.h"
#include <poll.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef UNIT_TESTING
#define TEST_STATIC
#else
#define TEST_STATIC static
#endif
/* ============================================================================
 * Global Variables
 * ============================================================================
 */
TEST_STATIC CliContext *g_cli_ctx = NULL;
TEST_STATIC bool
is_it_ai_turn (game_config_t *config, PlayerColor current_p)
{
  if (current_p == PLAYER_WHITE)
    return config->ai_white;
  if (current_p == PLAYER_BLACK)
    return config->ai_black;
  return false;
}

/** @name LINEHANDLER Readline Callback
 */
///@{
TEST_STATIC void
linehandler (char *line)
{
  if (line == NULL || !g_cli_ctx)
    return;

  printf ("\n"); // Visually validate the entry

  if (line[0] != '\0')
    {
      add_history (line);
      rl_replace_line ("", READLINE_NO_CLEAR);
      handle_command (line, g_cli_ctx);
    }
  else
    {
      rl_replace_line ("", READLINE_NO_CLEAR);
    }
  free (line);

  if (g_cli_ctx->game_running)
    {
      // 1. The plateau is displayed at the end of the processing.
      if (!g_cli_ctx->in_main_menu)
        {
          cli_display_board (g_cli_ctx->current_game);
        }

      // 2. We are updating the text of the prompt
      update_prompt_string (g_cli_ctx);
      rl_on_new_line ();
    }
}
///@}

/** @name RUN_INTERACTIVE_MODE Main Interactive Game Loop
 */
///@{
void
run_interactive_mode (game_config_t *config)
{
  GameState current_game;
  current_game.size = config->board_size;
  game_init (&current_game);

  NetworkManager net_manager;
  CliContext *ctx = (CliContext *)calloc (1, sizeof (CliContext));
  if (!ctx)
    {
      fprintf (stderr, "[ERROR] Failed to allocate memory for CLI context.\n");
      exit (EXIT_FAILURE);
    }
  NetworkManager_Init (&net_manager, create_cli_network_events (ctx));

  ctx->current_game = &current_game;
  ctx->config = config;
  ctx->net_manager = &net_manager;
  ctx->game_running = true;
  ctx->in_network_match = false;
  ctx->in_main_menu = true;
  ctx->my_role = PLAYER_WHITE;
  ctx->turn_start_time = time (NULL);
  ctx->timeout_limit = 60;
  ctx->timeout_sent = false;

  g_cli_ctx = ctx; // Set the global context pointer for callbacks

  rl_attempted_completion_function = amazons_completion;
  cli_clear_screen ();

  // Startup initialization
  if (config->start_server)
    {
      if (NetworkManager_ServerStart (ctx->net_manager, config->server_port)
          == 0)
        {
          ctx->in_main_menu = false; // Switch to Lobby mode
          printf (_ ("[INFO] Server started directly from command line on "
                     "port %d.\n"),
                  config->server_port);
        }
      else
        {

          ctx->in_main_menu = true;
          cli_display_main_menu ();
          printf (_ ("[ERROR] Failed to start server on port %d. It might be "
                     "in use.\n"),
                  config->server_port);
          printf (_ ("[INFO] Redirected to local menu type help for more "
                     "information.\n"));
        }
    }
  else
    {
      cli_display_main_menu (); // Normal mode initialization
    }

  rl_callback_handler_install ("", linehandler);
  refresh_prompt (ctx);

  struct pollfd fds[STDIN_FD_COUNT];
  fds[STDIN_FD_INDEX].fd = STDIN_FILENO;
  fds[STDIN_FD_INDEX].events = POLLIN;

  if ((config->ai_white || config->ai_black))
    {
      Param p_mini[] = { { (char *)"depth", (char *)"4" },
                         { (char *)"evaluator", (char *)"qd&kd" } };
      ConfigMap cm_mini = { .list = p_mini, .count = 2 };
      ctx->ai = create_ai ("minimax", cm_mini);
      ctx->ai_initialized = (ctx->ai.vtable != NULL);
    }

  while (ctx->game_running)
    {
      if (is_game_over (ctx->current_game, ctx->current_game->current_player))
        {
          cli_display_board (ctx->current_game);
          PlayerColor winner = get_winner (ctx->current_game);
          printf (_ ("\n GAME OVER ! \n"));
          printf (_ ("Player %s is blocked.\n"),
                  (ctx->current_game->current_player == PLAYER_WHITE
                       ? _ ("White")
                       : _ ("Black")));
          printf (_ ("Victory of the %s !\n"),
                  (winner == PLAYER_WHITE ? _ ("Whites") : _ ("Blacks")));

          if (ctx->net_manager->current_mode != NET_MODE_NONE)
            {

              bool i_won = (winner == ctx->my_role);
              NetworkManager_SendGameOver (ctx->net_manager, i_won);
              ctx->in_network_match = false;
              game_init (ctx->current_game);
              printf (_ ("\n[LOBBY] Match ended. Returning to Lobby.\n"));
              refresh_prompt (ctx);
            }
          else
            {

              ctx->game_running = false;
              break;
            }
        }
      NetworkManager_Tick (ctx->net_manager);

      if (ctx->ai_initialized
          && is_it_ai_turn (ctx->config, ctx->current_game->current_player))
        {

          bool allowed_to_play = true;

          if (ctx->net_manager->current_mode != NET_MODE_NONE)
            {

              if (!ctx->in_network_match)
                allowed_to_play = false;

              if (ctx->current_game->current_player != ctx->my_role)
                allowed_to_play = false;
            }

          if (allowed_to_play)
            {
              printf (_ ("\n[AI] Thinking...\n"));
              Move ai_move = ctx->ai.vtable->get_best_move (
                  &ctx->ai, *ctx->current_game);

              if (is_move_legal (ctx->current_game, ai_move))
                {
                  apply_move (ctx->current_game, ai_move);

                  if (ctx->net_manager->current_mode != NET_MODE_NONE)
                    {
                      NetworkManager_SendMove (ctx->net_manager, ai_move);
                    }

                  ctx->timeout_sent = false;
                  ctx->turn_start_time = time (NULL);

                  cli_display_board (ctx->current_game);
                  refresh_prompt (ctx);
                }
              else
                {
                  printf (_ ("\n[ERROR] AI suggested an illegal move or is "
                             "blocked.\n"));
                  printf (_ ("Game ended.\n"));
                  ctx->game_running = false;
                }
            }
        }

      // --- SYNCHRONIZED TIMEOUT LOGIC ---
      if (ctx->in_network_match
          && ctx->current_game->current_player == ctx->my_role)
        {

          time_t now = time (NULL);
          if (difftime (now, ctx->turn_start_time) >= ctx->timeout_limit
              && !ctx->timeout_sent)
            {
              printf (_ ("\n[TIMEOUT] Time's up! Passing the turn to your "
                         "adversary..\n"));

              // Turn switch
              ctx->current_game->current_player
                  = (ctx->current_game->current_player == PLAYER_WHITE)
                        ? PLAYER_BLACK
                        : PLAYER_WHITE;

              // Network send
              NetworkManager_SendTimeout (ctx->net_manager);

              ctx->timeout_sent = true;
              ctx->turn_start_time = now;

              cli_display_board (ctx->current_game);
              refresh_prompt (ctx);
            }
        }
      else if (ctx->in_network_match) // Time only resets when you play.
        {
          ctx->timeout_sent = false;
          ctx->turn_start_time = time (NULL);
        }

      if (poll (fds, STDIN_FD_COUNT, POLL_STDIN_TIMEOUT_MS) > 0
          && (fds[STDIN_FD_INDEX].revents & POLLIN))
        rl_callback_read_char ();
    }

  rl_callback_handler_remove ();
  if (ctx->ai_initialized && ctx->ai.vtable->clean)
    {
      ctx->ai.vtable->clean (&ctx->ai);
    }
  NetworkManager_Cleanup (ctx->net_manager);
  free (ctx);
  g_cli_ctx = NULL;
}
///@}
