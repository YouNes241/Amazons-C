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
#include <string>

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
 */
class GuiControllerTest : public ::testing::Test
{
protected:
  GuiContext *ctx;
  GameState game;
  game_config_t config;
  NetworkManager net_mgr;
  static bool gtk_initialized;

  static void
  SetUpTestSuite ()
  {
    if (!gtk_initialized)
      {
        gtk_init ();
        gtk_initialized = true;
      }
  }

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

    // --- GTK MOCK SETUP (Required to test guiHandlePlayerMove) ---
    ctx->mainWindow = GTK_WINDOW (gtk_window_new ());
    ctx->mainStack = GTK_STACK (gtk_stack_new ());

    GtkWidget *turnLabel = gtk_label_new ("Turn");
    g_object_set_data (G_OBJECT (ctx->mainStack), "turn_label", turnLabel);

    int boardSize = config.board_size * config.board_size;
    ctx->gameBoard = (GtkWidget **)calloc (boardSize, sizeof (GtkWidget *));
    for (int i = 0; i < boardSize; i++)
      {
        ctx->gameBoard[i] = gtk_button_new ();
        GtkWidget *lbl = gtk_label_new ("");
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "cell_label", lbl);
      }
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

    if (ctx->gameBoard)
      {
        free (ctx->gameBoard);
        ctx->gameBoard = nullptr;
      }

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
    m.queen_src.col = 3;
    m.queen_src.row = 0; // d1
    m.queen_dst.col = 3;
    m.queen_dst.row = 1; // d2
    m.arrow_dst.col = 3;
    m.arrow_dst.row = 2; // d3
    return m;
  }

  Move
  createLegalBlackMove ()
  {
    Move m;
    m.queen_src.col = 3;
    m.queen_src.row = 9; // d10
    m.queen_dst.col = 3;
    m.queen_dst.row = 8; // d9
    m.arrow_dst.col = 3;
    m.arrow_dst.row = 7; // d8
    return m;
  }

  Move
  createIllegalMove ()
  {
    Move m;
    m.queen_src.col = 0;
    m.queen_src.row = 0; // No queen at a1
    m.queen_dst.col = 0;
    m.queen_dst.row = 1;
    m.arrow_dst.col = 0;
    m.arrow_dst.row = 2;
    return m;
  }
};

bool GuiControllerTest::gtk_initialized = false;

/* ==========================================================================
 * guiHandlePlayerMove TESTS (High Coverage target)
 * ==========================================================================
 */

TEST_F (GuiControllerTest, HandlePlayerMove_IllegalMove)
{
  Move illegal = createIllegalMove ();
  ctx->currentPhase = PHASE_SHOOT_ARROW;

  testing::internal::CaptureStdout ();
  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, illegal));
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_NE (output.find ("Error: Engine rejected the move."),
             std::string::npos);
  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);
}

TEST_F (GuiControllerTest, HandlePlayerMove_LegalMove)
{
  Move legal = createLegalWhiteMove ();

  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, legal));

  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_BLACK);
}

TEST_F (GuiControllerTest, HandlePlayerMove_NetworkMatchSendsMove)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_WHITE;

  Move legal = createLegalWhiteMove ();

  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, legal));
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_BLACK);
}

TEST_F (GuiControllerTest, HandlePlayerMove_GameOver)
{
  for (int i = 0; i < 100; i++)
    ctx->gameState->board[i / 10][i % 10] = CONTENT_EMPTY;

  ctx->gameState->board[0][0] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[0][1] = CONTENT_ARROW;
  ctx->gameState->board[1][0] = CONTENT_ARROW;

  ctx->gameState->board[0][9] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[0][8] = CONTENT_ARROW;
  ctx->gameState->board[1][9] = CONTENT_ARROW;
  ctx->gameState->board[1][8] = CONTENT_ARROW;

  ctx->gameState->board[9][0] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[8][0] = CONTENT_ARROW;
  ctx->gameState->board[9][1] = CONTENT_ARROW;
  ctx->gameState->board[8][1] = CONTENT_ARROW;

  ctx->gameState->board[9][9] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[9][8] = CONTENT_ARROW;
  ctx->gameState->board[8][9] = CONTENT_ARROW;
  ctx->gameState->board[8][8] = CONTENT_ARROW;

  ctx->gameState->board[5][5] = CONTENT_BLACK_QUEEN;
  ctx->gameState->board[5][6] = CONTENT_BLACK_QUEEN;
  ctx->gameState->board[6][5] = CONTENT_BLACK_QUEEN;
  ctx->gameState->board[6][6] = CONTENT_BLACK_QUEEN;

  ctx->gameState->current_player = PLAYER_BLACK;

  Move final_move;
  final_move.queen_src.col = 5;
  final_move.queen_src.row = 5;
  final_move.queen_dst.col = 4;
  final_move.queen_dst.row = 4;
  final_move.arrow_dst.col = 1;
  final_move.arrow_dst.row = 1;

  testing::internal::CaptureStdout ();
  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, final_move));
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_NE (output.find ("Black won"), std::string::npos);
}

TEST_F (GuiControllerTest, HandlePlayerMove_GameOverNetwork)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_BLACK;

  for (int i = 0; i < 100; i++)
    ctx->gameState->board[i / 10][i % 10] = CONTENT_EMPTY;

  ctx->gameState->board[0][0] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[0][1] = CONTENT_ARROW;
  ctx->gameState->board[1][0] = CONTENT_ARROW;

  ctx->gameState->board[0][9] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[0][8] = CONTENT_ARROW;
  ctx->gameState->board[1][9] = CONTENT_ARROW;
  ctx->gameState->board[1][8] = CONTENT_ARROW;

  ctx->gameState->board[9][0] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[8][0] = CONTENT_ARROW;
  ctx->gameState->board[9][1] = CONTENT_ARROW;
  ctx->gameState->board[8][1] = CONTENT_ARROW;

  ctx->gameState->board[9][9] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[9][8] = CONTENT_ARROW;
  ctx->gameState->board[8][9] = CONTENT_ARROW;
  ctx->gameState->board[8][8] = CONTENT_ARROW;

  ctx->gameState->board[5][5] = CONTENT_BLACK_QUEEN;
  ctx->gameState->board[5][6] = CONTENT_BLACK_QUEEN;
  ctx->gameState->board[6][5] = CONTENT_BLACK_QUEEN;
  ctx->gameState->board[6][6] = CONTENT_BLACK_QUEEN;

  ctx->gameState->current_player = PLAYER_BLACK;

  Move final_move;
  final_move.queen_src.col = 5;
  final_move.queen_src.row = 5;
  final_move.queen_dst.col = 4;
  final_move.queen_dst.row = 4;
  final_move.arrow_dst.col = 1;
  final_move.arrow_dst.row = 1;

  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, final_move));

  EXPECT_FALSE (ctx->isNetworkMatch);
}

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

TEST_F (GuiControllerTest, StartNewGame_WithBlackAI)
{
  config.ai_white_cfg.enabled = false;
  config.ai_black_cfg.enabled = true;
  strncpy (config.ai_black_cfg.name, "random", MAX_AI_STR_LEN);

  guiStartNewGame (ctx);

  EXPECT_FALSE (ctx->is_ai_white_initialized);
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