/**
 * @file gui_controller.c
 * @brief Controller dedicated to Graphics mode (GTK).
 */
#include "ai_factory.h"
#include "ai_interface.h"
#include "controller.h"
#include "game_engine.h"
#include "gui_application.h"
#include "gui_network_events.h"
#include "gui_utils.h"
#include <gtk/gtk.h>

/** @name Forward Declarations
 */
///@{
void guiHandlePlayerMove (GuiContext *ctx, Move move);
static gboolean guiAIPlayTurn (gpointer user_data);
static gboolean gui_network_tick_loop (gpointer user_data);
static bool is_it_ai_turn (game_config_t *config, PlayerColor current_p);

///@}

/** @name Global Variables
 * @brief GUI AI instances and state
 */
///@{
static AI_Interface g_gui_ai_white;
static bool g_gui_ai_white_initialized = false;

static AI_Interface g_gui_ai_black;
static bool g_gui_ai_black_initialized = false;
///@}

void
guiHandlePlayerMove (GuiContext *ctx, Move move)
{
  if (is_move_legal (ctx->gameState, move))
    {
      apply_move (ctx->gameState, move);

      clearHintHighlights (ctx);
      ctx->isHintActive = false;

      if (ctx->isNetworkMatch
          && ctx->gameState->current_player != ctx->selfPlayerColor)
        {
          NetworkManager_SendMove (ctx->networkManager, move);
        }

      clearHighlights (ctx);
      drawBoard (ctx);
      updateTurnDisplay (ctx);
      ctx->currentPhase = PHASE_MOVE_QUEEN;

      if (is_game_over (ctx->gameState, ctx->gameState->current_player))
        {
          PlayerColor winner = get_winner (ctx->gameState);
          const char *winnerStr = (winner == PLAYER_WHITE) ? "White" : "Black";

          g_print ("\n GAME OVER ! \n%s won !\n", winnerStr);

          if (ctx->isNetworkMatch)
            {
              bool i_won = (winner == ctx->selfPlayerColor);
              NetworkManager_SendGameOver (ctx->networkManager, i_won);
              ctx->isNetworkMatch = false;
            }

#if GTK_CHECK_VERSION(4, 10, 0)
          GtkAlertDialog *dialog
              = gtk_alert_dialog_new ("GAME OVER ! \n%s won !", winnerStr);
          gtk_alert_dialog_show (dialog, GTK_WINDOW (ctx->mainWindow));
          g_object_unref (dialog);
#else
          GtkWidget *dialog = gtk_message_dialog_new (
              ctx->mainWindow,
              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "GAME OVER ! \n%s won !",
              winnerStr);

          g_signal_connect_swapped (dialog, "response",
                                    G_CALLBACK (gtk_window_destroy), dialog);
          gtk_window_present (GTK_WINDOW (dialog));
#endif

          return;
        }

      bool is_white_turn = (ctx->gameState->current_player == PLAYER_WHITE);
      bool current_ai_init = is_white_turn ? g_gui_ai_white_initialized
                                           : g_gui_ai_black_initialized;

      if (current_ai_init
          && is_it_ai_turn (ctx->gameConfig, ctx->gameState->current_player))
        {
          g_timeout_add (GUI_AI_DELAY, guiAIPlayTurn, ctx);
        }
    }
  else
    {
      g_print ("Error: Engine rejected the move.\n");
      clearHighlights (ctx);
      drawBoard (ctx);
      ctx->currentPhase = PHASE_MOVE_QUEEN;
    }
}

void
guiStartNewGame (GuiContext *ctx)
{
  if (g_gui_ai_white_initialized && g_gui_ai_white.vtable
      && g_gui_ai_white.vtable->clean)
    {
      g_gui_ai_white.vtable->clean (&g_gui_ai_white);
    }
  if (g_gui_ai_black_initialized && g_gui_ai_black.vtable
      && g_gui_ai_black.vtable->clean)
    {
      g_gui_ai_black.vtable->clean (&g_gui_ai_black);
    }

  g_gui_ai_white_initialized = false;
  g_gui_ai_black_initialized = false;

  if (ctx->gameConfig->ai_white_cfg.enabled)
    {
      Param params[MAX_AI_PARAMS];
      for (int i = 0; i < ctx->gameConfig->ai_white_cfg.param_count; i++)
        {
          params[i].key = ctx->gameConfig->ai_white_cfg.param_keys[i];
          params[i].value = ctx->gameConfig->ai_white_cfg.param_vals[i];
        }
      ConfigMap cm = { .count = ctx->gameConfig->ai_white_cfg.param_count,
                       .list = params };
      g_gui_ai_white = create_ai (ctx->gameConfig->ai_white_cfg.name, cm);
      g_gui_ai_white_initialized = (g_gui_ai_white.vtable != NULL);
    }

  if (ctx->gameConfig->ai_black_cfg.enabled)
    {
      Param params[MAX_AI_PARAMS];
      for (int i = 0; i < ctx->gameConfig->ai_black_cfg.param_count; i++)
        {
          params[i].key = ctx->gameConfig->ai_black_cfg.param_keys[i];
          params[i].value = ctx->gameConfig->ai_black_cfg.param_vals[i];
        }
      ConfigMap cm = { .count = ctx->gameConfig->ai_black_cfg.param_count,
                       .list = params };
      g_gui_ai_black = create_ai (ctx->gameConfig->ai_black_cfg.name, cm);
      g_gui_ai_black_initialized = (g_gui_ai_black.vtable != NULL);
    }

  bool is_white_turn = (ctx->gameState->current_player == PLAYER_WHITE);
  bool current_ai_init = is_white_turn ? g_gui_ai_white_initialized
                                       : g_gui_ai_black_initialized;

  if (current_ai_init
      && is_it_ai_turn (ctx->gameConfig, ctx->gameState->current_player))
    {
      g_timeout_add (GUI_AI_DELAY, guiAIPlayTurn, ctx);
    }
}

void
guiDisconnectAndRestartNetwork (GuiContext *ctx)
{
  if (ctx && ctx->networkManager)
    {
      NetworkManager_Cleanup (ctx->networkManager);
      NetworkManager_Init (ctx->networkManager,
                           create_gui_network_events (ctx));
    }

  ctx->isNetworkMatch = false;
}

void
run_gui_mode (game_config_t *config)
{
  GtkApplication *app = createApplication (config);

  GuiContext *ctx
      = (GuiContext *)g_object_get_data (G_OBJECT (app), "GuiContext");

  ctx->networkManager = malloc (sizeof (NetworkManager));
  ctx->isNetworkMatch = false;
  ctx->selfPlayerColor = PLAYER_WHITE;

  NetworkManager_Init (ctx->networkManager, create_gui_network_events (ctx));

  g_timeout_add (GUI_NETWORK_DELAY, gui_network_tick_loop, ctx);

  runApplication (app, 0, NULL);

  if (g_gui_ai_white_initialized && g_gui_ai_white.vtable
      && g_gui_ai_white.vtable->clean)
    {
      g_gui_ai_white.vtable->clean (&g_gui_ai_white);
    }
  if (g_gui_ai_black_initialized && g_gui_ai_black.vtable
      && g_gui_ai_black.vtable->clean)
    {
      g_gui_ai_black.vtable->clean (&g_gui_ai_black);
    }

  NetworkManager_Cleanup (ctx->networkManager);
  free (ctx->networkManager);
}

static bool
is_it_ai_turn (game_config_t *config, PlayerColor current_p)
{
  if (current_p == PLAYER_WHITE)
    return config->ai_white_cfg.enabled;
  if (current_p == PLAYER_BLACK)
    return config->ai_black_cfg.enabled;
  return false;
}

static gboolean
gui_network_tick_loop (gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  if (ctx && ctx->networkManager)
    {
      NetworkManager_Tick (ctx->networkManager);
    }
  return G_SOURCE_CONTINUE;
}

static gboolean
guiAIPlayTurn (gpointer user_data)
{
  AppContext *ctx = (AppContext *)user_data;

  g_print ("\n[AI] Thinking...\n");

  bool is_white_turn = (ctx->gameState->current_player == PLAYER_WHITE);
  AI_Interface *current_ai = is_white_turn ? &g_gui_ai_white : &g_gui_ai_black;

  Move ai_move
      = current_ai->vtable->get_best_move (current_ai, *(ctx->gameState));

  guiHandlePlayerMove (ctx, ai_move);

  return G_SOURCE_REMOVE;
}