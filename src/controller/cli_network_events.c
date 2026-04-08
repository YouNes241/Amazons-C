/**
 * @file cli_network_events.c
 * @brief Implementation of network event handlers for the CLI in the Amazons
 * game.
 *
 * This file contains the implementation of callback functions that handle
 * various network events such as receiving a move, game start, player
 * disconnection, etc. These handlers are designed to update the game state and
 * user interface accordingly when playing in network mode through the
 * command-line interface (CLI).
 */

#include "cli_network_events.h"
#include "cli_commands.h"
#include "cli_view.h"
#include "game_engine.h"
#include <stdio.h>

// Module-local global pointer to access callbacks
static CliContext *g_net_ctx = NULL;

/** @name Network Callback Handlers
 */
///@{

/**
 * @brief Called when the game starts
 */
static void
on_game_started (PlayerColor color, int board_size)
{
  if (!g_net_ctx)
    return;
  g_net_ctx->my_role = color;
  g_net_ctx->in_network_match = true;
  g_net_ctx->turn_start_time = time (NULL);

  g_net_ctx->current_game->size = board_size;
  game_init (g_net_ctx->current_game);

  printf (_ ("\n[GAME] The game begins! You play the %s.\n"),
          (color == PLAYER_WHITE) ? _ ("White") : _ ("Black"));
  cli_display_board (g_net_ctx->current_game);
  refresh_prompt (g_net_ctx);
}

static void
on_network_move_received (Move m)
{
  if (!g_net_ctx)
    return;
  if (is_move_legal (g_net_ctx->current_game, m))
    {
      printf ("\n");
      apply_move (g_net_ctx->current_game, m);
      printf (_ ("[NETWORK] The opponent played !\n"));

      // Reset the timer since it is now our turn
      g_net_ctx->timeout_sent = false;
      g_net_ctx->turn_start_time = time (NULL);

      cli_display_board (g_net_ctx->current_game);
      refresh_prompt (g_net_ctx);
    }
}

/**
 * @brief Called when the network informs us that the opponent has timed out
 */
static void
on_network_timeout_received ()
{
  if (!g_net_ctx)
    return;
  printf (
      _ ("\n[NETWORK] The opponent has exceeded their time limit! It's your "
         "turn to play!.\n"));

  // Update the current player locally
  g_net_ctx->current_game->current_player
      = (g_net_ctx->current_game->current_player == PLAYER_WHITE)
            ? PLAYER_BLACK
            : PLAYER_WHITE;

  // Reset our timer to begin our own 60 second turn
  g_net_ctx->timeout_sent = false;
  g_net_ctx->turn_start_time = time (NULL);

  // Refresh the display
  cli_display_board (g_net_ctx->current_game);
  refresh_prompt (g_net_ctx);
}

static void
on_player_joined (const char *name)
{
  (void)name;
  if (!g_net_ctx)
    return;
  refresh_prompt (g_net_ctx);
}

static void
on_player_disconnected (const char *reason)
{
  if (!g_net_ctx)
    return;
  if (g_net_ctx->net_manager->current_mode == NET_MODE_NONE)
    {

      g_net_ctx->in_network_match = false;
      g_net_ctx->in_main_menu = true;
      g_net_ctx->is_away = false;

      if (reason && strlen (reason) > 0)
        {
          printf (_ ("\n[SYSTEM] Connection lost: %s\n"), reason);
        }
    }
  else
    {
      if (g_net_ctx->in_network_match)
        {
          printf (_ ("\n[LOBBY] %s\n"), reason);
        }
      g_net_ctx->in_network_match = false;
    }
  refresh_prompt (g_net_ctx);
}

static void
on_game_error (const char *err)
{

  if (!g_net_ctx)
    return;
  printf (_ ("\n[SERVER] %s\n"), err ? err : "Unknown error");
  refresh_prompt (g_net_ctx);
}

static void
on_invitation_received (int challenger_id)
{
  if (!g_net_ctx)
    return;
  printf (_ ("\n[LOBBY] Player %d invited you to a game!\n"), challenger_id);
  printf (_ ("Type 'accept' to play, or 'decline' to refuse.\n"));
  refresh_prompt (g_net_ctx);
}
static void
on_invitation_declined (const char *message)
{
  if (!g_net_ctx)
    return;

  printf (_ ("\n[LOBBY] %s\n"), message); ///< Displays the decline message

  refresh_prompt (g_net_ctx);
}

static void
on_list_received (const char *list_text)
{
  if (!g_net_ctx)
    return;
  printf (_ ("\n=== LOBBY ===\n%s\n===============\n"), list_text);
  refresh_prompt (g_net_ctx);
}

///@}

/**
 * @brief Creates and returns a NetworkEvents structure configured for CLI
 * callbacks
 */
NetworkEvents
create_cli_network_events (CliContext *ctx)
{
  g_net_ctx = ctx; ///< Link the context for callback access

  NetworkEvents events = { 0 };
  events.OnMoveReceived = on_network_move_received;
  events.OnGameStarted = on_game_started;
  events.OnPlayerJoined = on_player_joined;
  events.OnGameError = on_game_error;
  events.OnInvitationReceived = on_invitation_received;
  events.OnInvitationDeclined = on_invitation_declined;
  events.OnPlayerDisconnected = on_player_disconnected;
  events.OnTimeoutReceived = on_network_timeout_received;
  events.OnListReceived = on_list_received;
  return events;
}