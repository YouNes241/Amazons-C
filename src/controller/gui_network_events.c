#include "gui_network_events.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include <stdio.h>

static GuiContext *g_gui_net_ctx = NULL;

static void on_gui_game_started (PlayerColor color, int board_size);
static void on_gui_move_received (Move m);
static void on_gui_invitation_received (/*int challenger_id*/);
static void on_gui_player_disconnected (/*const char *reason*/);

NetworkEvents
create_gui_network_events (GuiContext *ctx)
{
  g_gui_net_ctx = ctx;
  NetworkEvents events = { 0 };

  events.OnGameStarted = on_gui_game_started;
  events.OnMoveReceived = on_gui_move_received;
  events.OnInvitationReceived = on_gui_invitation_received;
  events.OnPlayerDisconnected = on_gui_player_disconnected;

  return events;
}

static void
on_gui_game_started (PlayerColor color, int board_size)
{
  if (!g_gui_net_ctx)
    return;

  g_gui_net_ctx->selfPlayerColor = color;
  g_gui_net_ctx->isNetworkMatch = true;
  g_gui_net_ctx->gameConfig->board_size = board_size;

  GtkWidget *boardGrid = GTK_WIDGET (
      g_object_get_data (G_OBJECT (g_gui_net_ctx->mainStack), "board_grid"));
  if (boardGrid)
    {
      setupBoard (g_gui_net_ctx, boardGrid);
    }

  gtk_stack_set_visible_child_name (g_gui_net_ctx->mainStack, "game_page");
}

static void
on_gui_move_received (Move m)
{
  if (!g_gui_net_ctx)
    return;

  guiHandlePlayerMove (g_gui_net_ctx, m);
}

static void on_gui_invitation_received (/*int challenger_id*/)
{
  if (!g_gui_net_ctx)
    return;

  g_print ("[GUI NETWORK] Auto accept activated... (WIP)\n");

  NetworkManager_SendAccept (g_gui_net_ctx->networkManager);
}

static void on_gui_player_disconnected (/*const char *reason*/)
{
  if (!g_gui_net_ctx)
    return;

  gtk_stack_set_visible_child_name (g_gui_net_ctx->mainStack,
                                    "join_server_page");
}