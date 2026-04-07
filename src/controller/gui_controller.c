/**
 * @file gui_controller.c
 * @brief Controller dedicated to Graphics mode (GTK).
 */
#include "ai_factory.h"
#include "ai_interface.h"
#include "controller.h"
#include "game_engine.h"
#include "gui_application.h"
#include "gui_utils.h"
#include <gtk/gtk.h>

/** @name Forward Declarations
 */
///@{
void guiHandlePlayerMove (AppContext *ctx, Move move);
///@}

/** @name Global Variables
 * @brief GUI AI instance and state
 */
///@{
static AI_Interface g_gui_ai;             ///< Global AI interface for GUI
static bool g_gui_ai_initialized = false; ///< AI initialization flag
///@}

static bool
is_it_ai_turn (game_config_t *config, PlayerColor current_p)
{
  if (current_p == PLAYER_WHITE)
    return config->ai_white;
  if (current_p == PLAYER_BLACK)
    return config->ai_black;
  return false;
}

static gboolean
guiAIPlayTurn (gpointer user_data)
{
  AppContext *ctx = (AppContext *)user_data;

  g_print ("\n[AI] Thinking...\n");
  Move ai_move = g_gui_ai.vtable->get_best_move (&g_gui_ai, *(ctx->gameState));

  guiHandlePlayerMove (ctx, ai_move);

  return G_SOURCE_REMOVE;
}
void
guiHandlePlayerMove (AppContext *ctx, Move move)
{
  if (is_move_legal (ctx->gameState, move))
    {
      apply_move (ctx->gameState, move);

      clearHighlights (ctx);
      drawBoard (ctx);
      ctx->currentPhase = PHASE_MOVE_QUEEN;

      if (is_game_over (ctx->gameState, ctx->gameState->current_player))
        {
          PlayerColor winner = get_winner (ctx->gameState);
          const char *winnerStr = (winner == PLAYER_WHITE) ? "White" : "Black";

          g_print ("\n GAME OVER ! \n%s won !\n", winnerStr);

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

      if (g_gui_ai_initialized
          && is_it_ai_turn (ctx->gameConfig, ctx->gameState->current_player))
        {
          g_timeout_add (50, guiAIPlayTurn, ctx);
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
guiStartNewGame (AppContext *ctx)
{
  if (g_gui_ai_initialized && g_gui_ai.vtable && g_gui_ai.vtable->clean)
    {
      g_gui_ai.vtable->clean (&g_gui_ai);
    }

  g_gui_ai_initialized = false;

  if (ctx->gameConfig->ai_white || ctx->gameConfig->ai_black)
    {
      ConfigMap empty_params = { .list = NULL, .count = 0 };
      g_gui_ai = create_ai ("random", empty_params);
      g_gui_ai_initialized = (g_gui_ai.vtable != NULL);

      if (g_gui_ai_initialized
          && is_it_ai_turn (ctx->gameConfig, ctx->gameState->current_player))
        {
          g_timeout_add (50, guiAIPlayTurn, ctx);
        }
    }
}

void
run_gui_mode (game_config_t *config)
{
  GtkApplication *app = createApplication (config);
  runApplication (app, 0, NULL);

  if (g_gui_ai_initialized && g_gui_ai.vtable && g_gui_ai.vtable->clean)
    {
      g_gui_ai.vtable->clean (&g_gui_ai);
    }
}
