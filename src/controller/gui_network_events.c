#include "gui_network_events.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include <stdio.h>

static AppContext *g_gui_net_ctx = NULL;

static void on_gui_game_started (PlayerColor color, int board_size);
static void on_gui_move_received (Move m);
static void on_gui_invitation_received (int challenger_id);
static void on_gui_player_joined (const char *player_name);
static void on_gui_list_received (const char *list_text);
static void on_gui_game_error (const char *error_msg);
static void on_gui_player_disconnected (const char *reason);

NetworkEvents
create_gui_network_events (AppContext *ctx)
{
  g_gui_net_ctx = ctx;
  NetworkEvents events = { 0 };

  events.OnGameStarted = on_gui_game_started;
  events.OnMoveReceived = on_gui_move_received;
  events.OnInvitationReceived = on_gui_invitation_received;
  events.OnPlayerJoined = on_gui_player_joined;
  events.OnListReceived = on_gui_list_received;
  events.OnGameError = on_gui_game_error;
  events.OnPlayerDisconnected = on_gui_player_disconnected;

  return events;
}

static void
on_gui_game_started (PlayerColor color, int board_size)
{
  if (!g_gui_net_ctx)
    return;

  g_print ("\n[GUI NETWORK] Signal START reçu du serveur !\n");

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

  g_print ("[GUI NETWORK] Le plateau est prêt ! Tu joues les %s\n",
           color == PLAYER_WHITE ? "Blancs" : "Noirs");
}

static void
on_gui_move_received (Move m)
{
  if (!g_gui_net_ctx)
    return;

  g_print ("[GUI NETWORK] L'adversaire a joué.\n");
  guiHandlePlayerMove (g_gui_net_ctx, m);
}

static void
on_gui_invitation_received (int challenger_id)
{
  if (!g_gui_net_ctx)
    return;

  g_print ("\n[GUI NETWORK] !!! Invitation reçue du joueur %d !!!\n",
           challenger_id);
  g_print ("[GUI NETWORK] Acceptation automatique activée (WIP)...\n");

  NetworkManager_SendAccept (g_gui_net_ctx->networkManager);
}

static void
on_gui_player_joined (const char *player_name)
{
  g_print ("[GUI NETWORK LOBBY] Nouveau joueur connecté : %s\n", player_name);
}

static void
on_gui_list_received (const char *list_text)
{
  g_print ("\n=== SERVEUR LOBBY ===\n%s\n=====================\n", list_text);
}

static void
on_gui_game_error (const char *error_msg)
{
  g_print ("\n[GUI NETWORK ERREUR] Le serveur a renvoyé : %s\n",
           error_msg ? error_msg : "Inconnu");
}

static void
on_gui_player_disconnected (const char *reason)
{
  if (!g_gui_net_ctx)
    return;

  g_print ("\n[GUI NETWORK] Déconnecté : %s\n", reason ? reason : "Fermeture");
  g_gui_net_ctx->isNetworkMatch = false;

  gtk_stack_set_visible_child_name (g_gui_net_ctx->mainStack,
                                    "main_menu_page");
}