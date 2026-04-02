#include "gui_network_events.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include <stdarg.h>
#include <stdio.h>

static GuiContext *g_gui_net_ctx = NULL;

static void on_gui_game_started (PlayerColor color, int board_size);
static void on_gui_move_received (Move m);
static void on_gui_invitation_received ();
static void on_gui_player_disconnected (const char *reason);
static void on_gui_player_joined (const char *player_name);

NetworkEvents
create_gui_network_events (GuiContext *ctx)
{
  g_gui_net_ctx = ctx;
  NetworkEvents events = { 0 };

  events.OnGameStarted = on_gui_game_started;
  events.OnMoveReceived = on_gui_move_received;
  events.OnInvitationReceived = on_gui_invitation_received;
  events.OnPlayerDisconnected = on_gui_player_disconnected;
  events.OnPlayerJoined = on_gui_player_joined;

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

static void
on_gui_invitation_received ()
{
  if (!g_gui_net_ctx)
    return;

  g_print ("[GUI NETWORK] Auto accept activated... (WIP)\n");

  NetworkManager_SendAccept (g_gui_net_ctx->networkManager);
}

static void
on_gui_player_disconnected (const char *reason)
{
  if (!g_gui_net_ctx)
    return;

  if (g_gui_net_ctx->networkManager->current_mode == NET_MODE_SERVER)
    {
      gui_log_to_server_console (g_gui_net_ctx, "[-] Player disconnected: %s",
                                 reason ? reason : "Unknown");
    }
  else if (g_gui_net_ctx->networkManager->current_mode == NET_MODE_CLIENT)
    {
      g_print ("[NETWORK] Opponent disconnected. Returning to Lobby.\n");
      g_gui_net_ctx->isNetworkMatch = false;
      gtk_stack_set_visible_child_name (g_gui_net_ctx->mainStack,
                                        "client_panel_page");

#if GTK_CHECK_VERSION(4, 10, 0)
      GtkAlertDialog *dialog = gtk_alert_dialog_new (
          "Opponent disconnected. Returning to Lobby.");
      gtk_alert_dialog_show (dialog, GTK_WINDOW (g_gui_net_ctx->mainWindow));
      g_object_unref (dialog);
#else
      GtkWidget *dialog = gtk_message_dialog_new (
          GTK_WINDOW (g_gui_net_ctx->mainWindow),
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
          GTK_BUTTONS_OK, "Opponent disconnected. Returning to Lobby.");
      g_signal_connect_swapped (dialog, "response",
                                G_CALLBACK (gtk_window_destroy), dialog);
      gtk_window_present (GTK_WINDOW (dialog));
#endif
    }
}

static void
on_gui_player_joined (const char *player_name)
{
  if (!g_gui_net_ctx)
    return;

  if (g_gui_net_ctx->networkManager->current_mode == NET_MODE_SERVER)
    {
      gui_log_to_server_console (g_gui_net_ctx, "[+] Player connected: %s",
                                 player_name);
    }
}
