#include "gui_network_events.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include <stdio.h>

static AppContext *g_gui_net_ctx = NULL;

static void
on_gui_game_started (PlayerColor color, int board_size)
{
  if (!g_gui_net_ctx)
    return;

  g_gui_net_ctx->selfPlayerColor = color;
  g_gui_net_ctx->isNetworkMatch = true;
  g_gui_net_ctx->gameConfig->board_size = board_size;

  gtk_stack_set_visible_child_name (g_gui_net_ctx->mainStack, "game_page");

  g_print ("[NETWORK] Game started! You are %s\n",
           color == PLAYER_WHITE ? "White" : "Black");
}

static void
on_gui_move_received (Move m)
{
  if (!g_gui_net_ctx)
    return;

  g_print ("[NETWORK] Opponent played.\n");
  guiHandlePlayerMove (g_gui_net_ctx, m);
}

// ADD OTHER CALLBACKS ! //

NetworkEvents
create_gui_network_events (AppContext *ctx)
{
  g_gui_net_ctx = ctx;
  NetworkEvents events = { 0 };
  events.OnGameStarted = on_gui_game_started;
  events.OnMoveReceived = on_gui_move_received;
  // LINK OTHER CALLBACKS ! //
  return events;
}