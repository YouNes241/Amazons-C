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
#include <assert.h>
#include <gtk/gtk.h>
#include <stdlib.h>

/** @name Forward Declarations
 */
///@{
static gboolean guiAIPlayTurn (gpointer user_data);
static gboolean gui_network_tick_loop (gpointer user_data);
static bool is_it_ai_turn (game_config_t *config, PlayerColor current_p);
static gpointer guiAIWorkerThread (gpointer user_data);
///@}

void
guiCheckAndTriggerAI (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);
  assert (ctx->gameState != NULL);

  if (ctx->isNetworkMatch)
    return;

  bool is_white_turn = (ctx->gameState->current_player == PLAYER_WHITE);
  bool current_ai_init = is_white_turn ? ctx->is_ai_white_initialized
                                       : ctx->is_ai_black_initialized;

  if (current_ai_init
      && is_it_ai_turn (ctx->gameConfig, ctx->gameState->current_player))
    {
      if (!ctx->is_ai_thinking)
        {
          ctx->is_ai_thinking = true;
          g_timeout_add (GUI_AI_DELAY, guiAIPlayTurn, ctx);
        }
    }
}

void
guiHandlePlayerMove (GuiContext *ctx, Move move)
{
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameConfig != NULL);

  if (is_move_legal (ctx->gameState, move))
    {
      apply_move (ctx->gameState, move);

      clearHintHighlights (ctx);
      ctx->isHintActive = false;

      if (ctx->isNetworkMatch
          && ctx->gameState->current_player != ctx->selfPlayerColor)
        {
          assert (ctx->networkManager != NULL);

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

          char msg[128];
          snprintf (msg, sizeof (msg), "GAME OVER ! \n%s won !", winnerStr);
          g_print ("\n %s \n", msg);

          if (ctx->isNetworkMatch)
            {
              assert (ctx->networkManager != NULL);

              bool i_won = (winner == ctx->selfPlayerColor);
              NetworkManager_SendGameOver (ctx->networkManager, i_won);
              ctx->isNetworkMatch = false;
            }

          guiShowPopup (ctx, msg);
          return;
        }

      guiCheckAndTriggerAI (ctx);
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
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);
  assert (ctx->gameState != NULL);

  ctx->is_ai_thinking = false;

  if (ctx->is_ai_white_initialized && ctx->ai_white.vtable
      && ctx->ai_white.vtable->clean)
    {
      ctx->ai_white.vtable->clean (&ctx->ai_white);
    }
  if (ctx->is_ai_black_initialized && ctx->ai_black.vtable
      && ctx->ai_black.vtable->clean)
    {
      ctx->ai_black.vtable->clean (&ctx->ai_black);
    }

  ctx->is_ai_white_initialized = false;
  ctx->is_ai_black_initialized = false;

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
      ctx->ai_white = create_ai (ctx->gameConfig->ai_white_cfg.name, cm);
      ctx->is_ai_white_initialized = (ctx->ai_white.vtable != NULL);
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
      ctx->ai_black = create_ai (ctx->gameConfig->ai_black_cfg.name, cm);
      ctx->is_ai_black_initialized = (ctx->ai_black.vtable != NULL);
    }

  guiCheckAndTriggerAI (ctx);
}

void
guiDisconnectAndRestartNetwork (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->networkManager != NULL);

  NetworkManager_Cleanup (ctx->networkManager);
  NetworkManager_Init (ctx->networkManager, create_gui_network_events (ctx));

  ctx->isNetworkMatch = false;
}

void
run_gui_mode (game_config_t *config)
{
  assert (config != NULL);

  GtkApplication *app = createApplication (config);
  assert (app != NULL);

  GuiContext *ctx
      = (GuiContext *)g_object_get_data (G_OBJECT (app), "GuiContext");
  assert (ctx != NULL);

  ctx->networkManager = malloc (sizeof (NetworkManager));
  assert (ctx->networkManager != NULL);

  ctx->isNetworkMatch = false;
  ctx->is_ai_thinking = false;
  ctx->selfPlayerColor = PLAYER_WHITE;

  NetworkManager_Init (ctx->networkManager, create_gui_network_events (ctx));

  g_timeout_add (GUI_NETWORK_DELAY, gui_network_tick_loop, ctx);

  runApplication (app, 0, NULL);

  if (ctx->is_ai_white_initialized && ctx->ai_white.vtable
      && ctx->ai_white.vtable->clean)
    {
      ctx->ai_white.vtable->clean (&ctx->ai_white);
    }
  if (ctx->is_ai_black_initialized && ctx->ai_black.vtable
      && ctx->ai_black.vtable->clean)
    {
      ctx->ai_black.vtable->clean (&ctx->ai_black);
    }

  NetworkManager_Cleanup (ctx->networkManager);
  free (ctx->networkManager);
}

static gboolean
guiAIApplyMove (gpointer user_data)
{
  AIWorkerData *data = (AIWorkerData *)user_data;
  GuiContext *ctx = data->ctx;

  ctx->is_ai_thinking = false;

  if (!ctx->isNetworkMatch && ctx->gameState
      && ctx->gameState->current_player == data->state_copy.current_player)
    {
      guiHandlePlayerMove (ctx, data->best_move);
    }

  free (data);
  return G_SOURCE_REMOVE;
}

static bool
is_it_ai_turn (game_config_t *config, PlayerColor current_p)
{
  assert (config != NULL);

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
  assert (ctx != NULL);
  assert (ctx->networkManager != NULL);

  NetworkManager_Tick (ctx->networkManager);

  return G_SOURCE_CONTINUE;
}

static gboolean
guiAIPlayTurn (gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);

  if (ctx->isNetworkMatch
      || !is_it_ai_turn (ctx->gameConfig, ctx->gameState->current_player))
    {
      ctx->is_ai_thinking = false;

      return G_SOURCE_REMOVE;
    }

  bool is_white_turn = (ctx->gameState->current_player == PLAYER_WHITE);
  AI_Interface *current_ai = is_white_turn ? &ctx->ai_white : &ctx->ai_black;

  if (current_ai == NULL || current_ai->vtable == NULL)
    {
      ctx->is_ai_thinking = false;
      return G_SOURCE_REMOVE;
    }

  ctx->is_ai_thinking = true;

  AIWorkerData *data = malloc (sizeof (AIWorkerData));
  assert (data != NULL);
  data->ctx = ctx;
  data->state_copy = *(ctx->gameState);
  data->ai = current_ai;

  g_thread_new ("ai_thread", guiAIWorkerThread, data);

  return G_SOURCE_REMOVE;
}

static gpointer
guiAIWorkerThread (gpointer user_data)
{
  AIWorkerData *data = (AIWorkerData *)user_data;

  data->best_move
      = data->ai->vtable->get_best_move (data->ai, data->state_copy);

  g_idle_add (guiAIApplyMove, data);

  return NULL;
}