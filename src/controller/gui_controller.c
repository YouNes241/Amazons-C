/**
 * @file gui_controller.c
 * @brief Implementation of the GUI controller logic.
 *
 * @details This file serves as the bridge between the GTK graphical interface
 * and the core game engine. It handles player move execution, triggers
 * background AI calculations, processes game-over states, and manages the
 * network event loop for the GUI mode.
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

/** * @name Forward Declarations
 * @brief Internal static functions used for asynchronous operations and
 * checks.
 * @{
 */
static gboolean guiAIPlayTurn (gpointer user_data);
static gboolean gui_network_tick_loop (gpointer user_data);
static bool is_it_ai_turn (game_config_t *config, PlayerColor current_p);
static gpointer guiAIWorkerThread (gpointer user_data);
static gboolean guiAIApplyMove (gpointer user_data);
/** @} */

/**
 * @brief Checks if the current turn belongs to an AI and triggers it.
 *
 * @details Verifies if the active player is controlled by an AI. If so, and
 * the UI is not already locked by an AI thinking phase, it schedules the
 * AI turn execution after a short UI delay (`GUI_AI_DELAY`).
 *
 * @param ctx Pointer to the main GUI context structure.
 */
void
guiCheckAndTriggerAI (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);
  assert (ctx->gameState != NULL);

  // Network matches strictly rely on incoming packets, not local AI triggers
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

/**
 * @brief Handles a move submitted by the local player or the network.
 *
 * @details Validates the requested move against the game engine. If valid,
 * it applies the move, updates the visual board, forwards the move to the
 * network server (if applicable), and checks for game-over conditions.
 * Finally, it delegates the next turn to the AI if needed.
 *
 * @param ctx Pointer to the main GUI context structure.
 * @param move The requested move (Queen movement + Arrow shot).
 */
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

      // Forward the move to the network if we are playing remotely
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
              ctx->isNetworkMatch = false; // Close the network match state
            }

          guiShowPopup (ctx, msg);
          return;
        }

      // Move was successful and game continues, check if an AI must play next
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

/**
 * @brief Starts a new local game session.
 *
 * @details Resets the AI thinking flags, cleans up any previously running AI
 * instances, and creates new AI instances based on the current configuration
 * parameters. It then checks if the first turn belongs to an AI.
 *
 * @param ctx Pointer to the main GUI context structure.
 */
void
guiStartNewGame (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);
  assert (ctx->gameState != NULL);

  ctx->is_ai_thinking = false;

  // Clean up existing AI instances to prevent memory leaks
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

  // Initialize White AI if enabled
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

  // Initialize Black AI if enabled
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

/**
 * @brief Shuts down active network sessions and resets the network manager.
 *
 * @details Cleans up sockets and active connections, then re-initializes
 * the network manager with the GUI callbacks to prepare it for future lobbies.
 *
 * @param ctx Pointer to the main GUI context structure.
 */
void
guiDisconnectAndRestartNetwork (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->networkManager != NULL);

  NetworkManager_Cleanup (ctx->networkManager);
  NetworkManager_Init (ctx->networkManager, create_gui_network_events (ctx));

  ctx->isNetworkMatch = false;
}

/**
 * @brief Primary entry point for the GUI application mode.
 *
 * @details Bootstraps the GTK application, initializes the global GUI context,
 * prepares the network subsystem, starts the non-blocking network tick loop,
 * and transfers execution to the GTK main event loop.
 *
 * @param config Pointer to the game configuration loaded from CLI/File.
 */
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

  // Schedule the network tick loop to run continuously in the background
  g_timeout_add (GUI_NETWORK_DELAY, gui_network_tick_loop, ctx);

  // Blocks until the GTK application is closed by the user
  runApplication (app, 0, NULL);

  // Cleanup after GTK loop exits
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

/**
 * @brief Safely applies the move calculated by the background AI thread.
 *
 * @details This function is invoked on the main GTK thread via `g_idle_add`
 * after the AI worker thread finishes its calculation. It verifies the game
 * state hasn't changed (e.g., interrupted by network) before applying the
 * move.
 *
 * @param user_data Pointer to the `AIWorkerData` structure.
 * @return G_SOURCE_REMOVE to ensure this idle callback only fires once.
 */
static gboolean
guiAIApplyMove (gpointer user_data)
{
  AIWorkerData *data = (AIWorkerData *)user_data;
  GuiContext *ctx = data->ctx;

  ctx->is_ai_thinking = false;

  // Only apply if the match hasn't switched to network mode and it's still the
  // AI's turn
  if (!ctx->isNetworkMatch && ctx->gameState
      && ctx->gameState->current_player == data->state_copy.current_player)
    {
      guiHandlePlayerMove (ctx, data->best_move);
    }

  free (data);
  return G_SOURCE_REMOVE;
}

/**
 * @brief Evaluates whether the given player is controlled by an AI.
 *
 * @param config Pointer to the game configuration.
 * @param current_p The color of the player to check.
 * @return True if the player is configured as an AI, false if Human.
 */
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

/**
 * @brief Non-blocking loop processing incoming network events for the GUI.
 *
 * @details Registered as a GTK timeout. It periodically calls the core
 * network tick function to handle buffers, lobbies, and moves seamlessly.
 *
 * @param user_data Pointer to the main GUI context structure.
 * @return G_SOURCE_CONTINUE to keep the timer active.
 */
static gboolean
gui_network_tick_loop (gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->networkManager != NULL);

  NetworkManager_Tick (ctx->networkManager);

  return G_SOURCE_CONTINUE;
}

/**
 * @brief Prepares and launches the AI calculation in a separate thread.
 *
 * @details Extracts a safe copy of the current game state to prevent data
 * races, packages it into `AIWorkerData`, and spawns a background thread so
 * the GTK UI remains responsive during heavy AI processing.
 *
 * @param user_data Pointer to the main GUI context structure.
 * @return G_SOURCE_REMOVE to prevent the initial timeout trigger from
 * repeating.
 */
static gboolean
guiAIPlayTurn (gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);

  // Abort if the game state shifted to a network match or human turn during
  // the delay
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
  data->state_copy = *(ctx->gameState); // Thread-safe deep copy of the board
  data->ai = current_ai;

  g_thread_new ("ai_thread", guiAIWorkerThread, data);

  return G_SOURCE_REMOVE;
}

/**
 * @brief Background worker thread routine for AI logic.
 *
 * @details Computes the optimal move using the AI engine. Once finished,
 * it schedules the execution of `guiAIApplyMove` back onto the main GTK
 * thread.
 *
 * @param user_data Pointer to the `AIWorkerData` structure.
 * @return Always returns NULL.
 */
static gpointer
guiAIWorkerThread (gpointer user_data)
{
  AIWorkerData *data = (AIWorkerData *)user_data;

  // Execute the heavy AI algorithm in the background
  data->best_move
      = data->ai->vtable->get_best_move (data->ai, data->state_copy);

  // Safely hand the result back to the GTK main thread
  g_idle_add (guiAIApplyMove, data);

  return NULL;
}