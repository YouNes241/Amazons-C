/**
 * @file gui_network_events.c
 * @brief Implementation of GUI network event handlers.
 *
 * @details This file implements the callback functions that are triggered
 * by the networking subsystem. It bridges network events (like a game
 * starting, a player joining, or receiving a move) to the GTK graphical
 * interface, ensuring the UI updates accordingly.
 */

#include "gui_network_events.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * @brief Global static pointer to the main GUI context.
 * @details Required because network callbacks are mapped via function pointers
 * without a user_data parameter, so they need a way to access the UI state.
 */
static GuiContext *g_gui_net_ctx = NULL;

/* Static callback prototypes */
static void on_gui_game_started (PlayerColor color, int board_size);
static void on_gui_move_received (Move m);
static void on_gui_invitation_received (int from_player_id);
static void on_gui_player_disconnected (const char *reason);
static void on_gui_player_joined (const char *player_name);

/**
 * @brief Creates and initializes the network event callbacks for the GUI.
 *
 * @param ctx Pointer to the main GUI context. If NULL, resets the global
 * context.
 * @return A populated NetworkEvents structure containing the mapped GUI
 * callbacks.
 */
NetworkEvents
create_gui_network_events (GuiContext *ctx)
{
  g_gui_net_ctx = ctx;
  NetworkEvents events = { 0 };

  if (ctx == NULL)
    {
      return events;
    }

  events.OnGameStarted = on_gui_game_started;
  events.OnMoveReceived = on_gui_move_received;
  events.OnInvitationReceived = on_gui_invitation_received;
  events.OnPlayerDisconnected = on_gui_player_disconnected;
  events.OnPlayerJoined = on_gui_player_joined;

  return events;
}

/**
 * @brief Callback triggered when a network game officially starts.
 *
 * @details Updates the local player's color, sets the network match flag,
 * initializes the board widget to the agreed size, and switches the UI stack
 * to the active game view.
 *
 * @param color The color assigned to the local player.
 * @param board_size The dimension of the game board.
 */
static void
on_gui_game_started (PlayerColor color, int board_size)
{
  assert (g_gui_net_ctx != NULL);
  assert (g_gui_net_ctx->gameConfig != NULL);
  assert (g_gui_net_ctx->mainStack != NULL);

  g_gui_net_ctx->selfPlayerColor = color;
  g_gui_net_ctx->isNetworkMatch = true;
  g_gui_net_ctx->gameConfig->board_size = board_size;

  GtkWidget *boardGrid = GTK_WIDGET (
      g_object_get_data (G_OBJECT (g_gui_net_ctx->mainStack), "board_grid"));
  assert (boardGrid != NULL);

  setupBoard (g_gui_net_ctx, boardGrid);

  gtk_stack_set_visible_child_name (g_gui_net_ctx->mainStack, "game_page");
}

/**
 * @brief Callback triggered when a move is received from the opponent.
 *
 * @details Delegates the received move to the main GUI controller logic
 * to validate and apply it to the board visually.
 *
 * @param m The move structure (Queen movement + Arrow shot) received.
 */
static void
on_gui_move_received (Move m)
{
  assert (g_gui_net_ctx != NULL);

  guiHandlePlayerMove (g_gui_net_ctx, m);
}

/**
 * @brief Callback triggered when a match invitation is received in the lobby.
 *
 * @details Currently implements a Work-In-Progress (WIP) auto-accept
 * mechanism.
 * * @param from_player_id The unique network ID of the player sending the
 * invite.
 */
static void
on_gui_invitation_received (int from_player_id)
{
  assert (g_gui_net_ctx != NULL);
  assert (g_gui_net_ctx->networkManager != NULL);

  (void)from_player_id; /* Suppress unused parameter warning for now */

  g_print ("[GUI NETWORK] Auto accept activated... (WIP)\n");

  NetworkManager_SendAccept (g_gui_net_ctx->networkManager);
}

/**
 * @brief Callback triggered when a player disconnects from the network
 * session.
 *
 * @details If acting as a server, logs the disconnection to the server
 * console. If acting as a client, terminates the current match, returns the
 * user to the lobby panel, and displays a disconnection popup.
 *
 * @param reason A string detailing why the disconnection occurred (e.g.,
 * "Timeout").
 */
static void
on_gui_player_disconnected (const char *reason)
{
  assert (g_gui_net_ctx != NULL);
  assert (g_gui_net_ctx->networkManager != NULL);

  if (g_gui_net_ctx->networkManager->current_mode == NET_MODE_SERVER)
    {
      gui_log_to_server_console (g_gui_net_ctx, "[-] Player disconnected: %s",
                                 reason ? reason : "Unknown");
    }
  else if (g_gui_net_ctx->networkManager->current_mode == NET_MODE_CLIENT)
    {
      assert (g_gui_net_ctx->mainStack != NULL);

      g_print ("[NETWORK] Opponent disconnected. Returning to Lobby.\n");

      g_gui_net_ctx->isNetworkMatch = false;
      gtk_stack_set_visible_child_name (g_gui_net_ctx->mainStack,
                                        "client_panel_page");

      guiShowPopup (g_gui_net_ctx,
                    "Opponent disconnected. Returning to Lobby.");
    }
}

/**
 * @brief Callback triggered when a new player joins the server.
 *
 * @details Only applicable when hosting a server. Logs the newly connected
 * player's name to the visual server console.
 *
 * @param player_name The username of the player who just joined.
 */
static void
on_gui_player_joined (const char *player_name)
{
  assert (g_gui_net_ctx != NULL);
  assert (g_gui_net_ctx->networkManager != NULL);

  if (g_gui_net_ctx->networkManager->current_mode == NET_MODE_SERVER)
    {
      gui_log_to_server_console (g_gui_net_ctx, "[+] Player connected: %s",
                                 player_name);
    }
}