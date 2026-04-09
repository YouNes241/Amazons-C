/**
 * @file test_gui_controller.cpp
 * @brief Unit tests for gui_controller.c functions
 *
 * Tests cover GUI controller logic including AI turn management,
 * player move handling, and network operations.
 * Note: These tests mock GTK dependencies where possible.
 */

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>

extern "C"
{
#include "ai_factory.h"
#include "ai_interface.h"
#include "config.h"
#include "game_engine.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include "network_common.h"
}

/**
 * @brief Test fixture for GUI controller tests
 *
 * Sets up a minimal GuiContext without actual GTK widgets
 * for testing the logic-only functions.
 */
class GuiControllerTest : public ::testing::Test
{
protected:
  GuiContext *ctx;
  GameState game;
  game_config_t config;
  NetworkManager net_mgr;

  void
  SetUp () override
  {
    ctx = (GuiContext *)calloc (1, sizeof (GuiContext));
    ASSERT_NE (ctx, nullptr);

    memset (&game, 0, sizeof (GameState));
    memset (&config, 0, sizeof (game_config_t));
    memset (&net_mgr, 0, sizeof (NetworkManager));

    config.board_size = 10;
    game.size = 10;
    game_init (&game);

    ctx->gameState = &game;
    ctx->gameConfig = &config;
    ctx->networkManager = &net_mgr;
    ctx->isNetworkMatch = false;
    ctx->is_ai_thinking = false;
    ctx->selfPlayerColor = PLAYER_WHITE;
    ctx->currentPhase = PHASE_MOVE_QUEEN;
    ctx->isHintActive = false;
    ctx->is_ai_white_initialized = false;
    ctx->is_ai_black_initialized = false;

    // Set mainStack and mainWindow to NULL - we won't use GTK widgets
    ctx->mainStack = nullptr;
    ctx->mainWindow = nullptr;
    ctx->gameBoard = nullptr;
  }

  void
  TearDown () override
  {
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

    game_cleanup (&game);

    if (ctx)
      {
        free (ctx);
        ctx = nullptr;
      }
  }

  Move
  createLegalWhiteMove ()
  {
    Move m;
    m.queen_src = { 3, 0 }; // d1
    m.queen_dst = { 3, 1 }; // d2
    m.arrow_dst = { 3, 2 }; // d3
    return m;
  }

  Move
  createLegalBlackMove ()
  {
    Move m;
    m.queen_src = { 3, 9 }; // d10
    m.queen_dst = { 3, 8 }; // d9
    m.arrow_dst = { 3, 7 }; // d8
    return m;
  }

  Move
  createIllegalMove ()
  {
    Move m;
    m.queen_src = { 0, 0 };
    m.queen_dst = { 0, 1 };
    m.arrow_dst = { 0, 2 };
    return m;
  }
};

/* ==========================================================================
 * guiCheckAndTriggerAI TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, CheckAndTriggerAI_ReturnsEarlyInNetworkMatch)
{
  ctx->isNetworkMatch = true;
  ctx->is_ai_white_initialized = true;
  config.ai_white_cfg.enabled = true;

  guiCheckAndTriggerAI (ctx);

  EXPECT_FALSE (ctx->is_ai_thinking);
}

TEST_F (GuiControllerTest, CheckAndTriggerAI_NoAIInitialized)
{
  ctx->isNetworkMatch = false;
  ctx->is_ai_white_initialized = false;
  ctx->is_ai_black_initialized = false;
  config.ai_white_cfg.enabled = false;
  config.ai_black_cfg.enabled = false;

  guiCheckAndTriggerAI (ctx);
  EXPECT_FALSE (ctx->is_ai_thinking);
}

TEST_F (GuiControllerTest, CheckAndTriggerAI_SetsThinkingLock)
{
  ctx->isNetworkMatch = false;
  ctx->gameState->current_player = PLAYER_WHITE;
  config.ai_white_cfg.enabled = true;

  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };
  ctx->ai_white = create_ai ("random", cm);
  ctx->is_ai_white_initialized = (ctx->ai_white.vtable != nullptr);

  ctx->is_ai_thinking = false;

  guiCheckAndTriggerAI (ctx);

  EXPECT_TRUE (ctx->is_ai_thinking);
}

TEST_F (GuiControllerTest, CheckAndTriggerAI_AIDisabledForCurrentPlayer)
{
  ctx->isNetworkMatch = false;
  ctx->gameState->current_player = PLAYER_WHITE;
  config.ai_white_cfg.enabled = false;
  config.ai_black_cfg.enabled = true;

  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };
  ctx->ai_black = create_ai ("random", cm);
  ctx->is_ai_black_initialized = (ctx->ai_black.vtable != nullptr);

  guiCheckAndTriggerAI (ctx);

  EXPECT_FALSE (ctx->is_ai_thinking);
}

/* ==========================================================================
 * guiStartNewGame TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, StartNewGame_CleansUpOldAI)
{
  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };

  config.ai_white_cfg.enabled = true;
  strncpy (config.ai_white_cfg.name, "random", MAX_AI_STR_LEN);
  ctx->ai_white = create_ai ("random", cm);
  ctx->is_ai_white_initialized = (ctx->ai_white.vtable != nullptr);

  config.ai_black_cfg.enabled = true;
  strncpy (config.ai_black_cfg.name, "random", MAX_AI_STR_LEN);
  ctx->ai_black = create_ai ("random", cm);
  ctx->is_ai_black_initialized = (ctx->ai_black.vtable != nullptr);

  guiStartNewGame (ctx);

  EXPECT_TRUE (ctx->is_ai_white_initialized);
  EXPECT_TRUE (ctx->is_ai_black_initialized);
}

TEST_F (GuiControllerTest, StartNewGame_NoAIEnabled)
{
  config.ai_white_cfg.enabled = false;
  config.ai_black_cfg.enabled = false;

  guiStartNewGame (ctx);

  EXPECT_FALSE (ctx->is_ai_white_initialized);
  EXPECT_FALSE (ctx->is_ai_black_initialized);
}

TEST_F (GuiControllerTest, StartNewGame_ResetsThinkingFlag)
{
  ctx->is_ai_thinking = true;
  config.ai_white_cfg.enabled = false;
  config.ai_black_cfg.enabled = false;

  guiStartNewGame (ctx);

  EXPECT_FALSE (ctx->is_ai_thinking);
}

TEST_F (GuiControllerTest, StartNewGame_WithAIParams)
{
  config.ai_white_cfg.enabled = true;
  strncpy (config.ai_white_cfg.name, "minimax", MAX_AI_STR_LEN);
  config.ai_white_cfg.param_count = 1;
  strncpy (config.ai_white_cfg.param_keys[0], "depth", MAX_AI_STR_LEN);
  strncpy (config.ai_white_cfg.param_vals[0], "1", MAX_AI_STR_LEN);

  guiStartNewGame (ctx);

  EXPECT_TRUE (ctx->is_ai_white_initialized);
}

/* ==========================================================================
 * guiDisconnectAndRestartNetwork TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, DisconnectAndRestartNetwork_CleansUpNetwork)
{
  ctx->isNetworkMatch = true;
  net_mgr.current_mode = NET_MODE_CLIENT;

  guiDisconnectAndRestartNetwork (ctx);

  EXPECT_FALSE (ctx->isNetworkMatch);
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_NONE);
}

TEST_F (GuiControllerTest, DisconnectAndRestartNetwork_NullContextCrashes)
{
  EXPECT_DEATH (guiDisconnectAndRestartNetwork (nullptr), "");
}

/* ==========================================================================
 * GAME STATE TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, GameStateInitialization)
{
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_WHITE);
  EXPECT_EQ (ctx->gameState->status, GAME_IN_PROGRESS);
  EXPECT_EQ (ctx->gameState->size, 10);
}

/* ==========================================================================
 * MOVE VALIDATION TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, MoveValidation_LegalMove)
{
  Move m = createLegalWhiteMove ();
  EXPECT_TRUE (is_move_legal (ctx->gameState, m));
}

TEST_F (GuiControllerTest, MoveValidation_IllegalMove)
{
  Move m = createIllegalMove ();
  EXPECT_FALSE (is_move_legal (ctx->gameState, m));
}

TEST_F (GuiControllerTest, ApplyMove_ChangesPlayer)
{
  Move m = createLegalWhiteMove ();
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_WHITE);

  apply_move (ctx->gameState, m);

  EXPECT_EQ (ctx->gameState->current_player, PLAYER_BLACK);
}

/* ==========================================================================
 * NETWORK STATE TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, NetworkManagerModes)
{
  EXPECT_EQ (net_mgr.current_mode, NET_MODE_NONE);

  net_mgr.current_mode = NET_MODE_CLIENT;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_CLIENT);

  net_mgr.current_mode = NET_MODE_SERVER;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_SERVER);
}

/* ==========================================================================
 * GAME OVER DETECTION TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, GameOverDetection_NotOver)
{
  EXPECT_FALSE (is_game_over (ctx->gameState, PLAYER_WHITE));
  EXPECT_FALSE (is_game_over (ctx->gameState, PLAYER_BLACK));
}

TEST_F (GuiControllerTest, GameOverDetection_AllBlocked)
{
  for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 10; j++)
        {
          ctx->gameState->board[i][j] = CONTENT_ARROW;
        }
    }

  EXPECT_TRUE (is_game_over (ctx->gameState, PLAYER_WHITE));
  EXPECT_TRUE (is_game_over (ctx->gameState, PLAYER_BLACK));
}

TEST_F (GuiControllerTest, GetWinner_WhiteWins)
{
  ctx->gameState->current_player = PLAYER_BLACK;
  ctx->gameState->status = GAME_OVER_WHITE_WINS;

  for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 10; j++)
        {
          ctx->gameState->board[i][j] = CONTENT_ARROW;
        }
    }

  EXPECT_EQ (get_winner (ctx->gameState), PLAYER_WHITE);
}

TEST_F (GuiControllerTest, GetWinner_BlackWins)
{
  ctx->gameState->current_player = PLAYER_WHITE;
  ctx->gameState->status = GAME_OVER_BLACK_WINS;

  for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 10; j++)
        {
          ctx->gameState->board[i][j] = CONTENT_ARROW;
        }
    }

  EXPECT_EQ (get_winner (ctx->gameState), PLAYER_BLACK);
}

/* ==========================================================================
 * AI MOVE GENERATION TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, AIGeneratesLegalMove)
{
  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };

  AI_Interface ai = create_ai ("random", cm);
  ASSERT_NE (ai.vtable, nullptr);

  ctx->gameState->current_player = PLAYER_WHITE;
  Move ai_move = ai.vtable->get_best_move (&ai, *ctx->gameState);

  EXPECT_TRUE (is_move_legal (ctx->gameState, ai_move));

  if (ai.vtable->clean)
    {
      ai.vtable->clean (&ai);
    }
}