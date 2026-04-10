/**
 * @file test_gui_controller.cpp
 * @brief Unit tests for gui_controller.c functions
 *
 * Tests cover GUI controller logic including AI turn management,
 * player move handling, and network operations.
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
        g_setenv ("GTK_A11Y", "none", TRUE);
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

    // --- GTK MOCK SETUP ---
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
      ctx->ai_white.vtable->clean (&ctx->ai_white);

    if (ctx->is_ai_black_initialized && ctx->ai_black.vtable
        && ctx->ai_black.vtable->clean)
      ctx->ai_black.vtable->clean (&ctx->ai_black);

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
    m.queen_src.row = 0;
    m.queen_dst.col = 3;
    m.queen_dst.row = 1;
    m.arrow_dst.col = 3;
    m.arrow_dst.row = 2;
    return m;
  }

  Move
  createIllegalMove ()
  {
    Move m;
    m.queen_src.col = 0;
    m.queen_src.row = 0;
    m.queen_dst.col = 0;
    m.queen_dst.row = 1;
    m.arrow_dst.col = 0;
    m.arrow_dst.row = 2;
    return m;
  }

  /**
   * Helper pour construire un plateau "Game Over" parfait et 100% légal.
   * Tout est bloqué, sauf un unique mouvement pour le joueur gagnant.
   */
  void
  setupGameOverBoard (PlayerColor winner)
  {
    for (int i = 0; i < 100; i++)
      ctx->gameState->board[i / 10][i % 10] = CONTENT_ARROW;

    if (winner == PLAYER_WHITE)
      {
        ctx->gameState->board[0][0] = CONTENT_BLACK_QUEEN;
        ctx->gameState->board[0][9] = CONTENT_BLACK_QUEEN;
        ctx->gameState->board[9][0] = CONTENT_BLACK_QUEEN;
        ctx->gameState->board[9][9] = CONTENT_BLACK_QUEEN;

        ctx->gameState->board[5][5] = CONTENT_WHITE_QUEEN;
        ctx->gameState->current_player = PLAYER_WHITE;
      }
    else
      {
        ctx->gameState->board[0][0] = CONTENT_WHITE_QUEEN;
        ctx->gameState->board[0][9] = CONTENT_WHITE_QUEEN;
        ctx->gameState->board[9][0] = CONTENT_WHITE_QUEEN;
        ctx->gameState->board[9][9] = CONTENT_WHITE_QUEEN;

        ctx->gameState->board[5][5] = CONTENT_BLACK_QUEEN;
        ctx->gameState->current_player = PLAYER_BLACK;
      }

    // On libère un petit chemin diagonal pour le coup fatal
    ctx->gameState->board[4][4] = CONTENT_EMPTY;
    ctx->gameState->board[3][3] = CONTENT_EMPTY;
  }

  // Le coup fatal universel associé à la fonction ci-dessus
  Move
  getFatalMove ()
  {
    Move m;
    m.queen_src.col = 5;
    m.queen_src.row = 5;
    m.queen_dst.col = 4;
    m.queen_dst.row = 4;
    m.arrow_dst.col = 3;
    m.arrow_dst.row = 3;
    return m;
  }
};

bool GuiControllerTest::gtk_initialized = false;

/* ==========================================================================
 * guiHandlePlayerMove TESTS
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

TEST_F (GuiControllerTest, HandlePlayerMove_GameOver_BlackWins)
{
  setupGameOverBoard (PLAYER_BLACK);

  testing::internal::CaptureStdout ();
  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, getFatalMove ()));
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_NE (output.find ("Black won"), std::string::npos);
}

TEST_F (GuiControllerTest, HandlePlayerMove_GameOver_WhiteWins)
{
  setupGameOverBoard (PLAYER_WHITE);

  testing::internal::CaptureStdout ();
  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, getFatalMove ()));
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_NE (output.find ("White won"), std::string::npos);
}

TEST_F (GuiControllerTest, HandlePlayerMove_GameOverNetwork_IWon)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_WHITE; // Je suis Blanc
  setupGameOverBoard (PLAYER_WHITE);   // Blanc Gagne

  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, getFatalMove ()));
  EXPECT_FALSE (ctx->isNetworkMatch); // Le match est clôturé
}

TEST_F (GuiControllerTest, HandlePlayerMove_GameOverNetwork_ILost)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_WHITE; // Je suis Blanc
  setupGameOverBoard (PLAYER_BLACK);   // Noir Gagne

  EXPECT_NO_THROW (guiHandlePlayerMove (ctx, getFatalMove ()));
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
  guiCheckAndTriggerAI (ctx);
  EXPECT_FALSE (ctx->is_ai_thinking);
}

TEST_F (GuiControllerTest, CheckAndTriggerAI_SetsThinkingLock)
{
  ctx->gameState->current_player = PLAYER_WHITE;
  config.ai_white_cfg.enabled = true;

  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };
  ctx->ai_white = create_ai ("random", cm);
  ctx->is_ai_white_initialized = (ctx->ai_white.vtable != nullptr);

  guiCheckAndTriggerAI (ctx);
  EXPECT_TRUE (ctx->is_ai_thinking);
}

TEST_F (GuiControllerTest, CheckAndTriggerAI_BlackTurn)
{
  ctx->gameState->current_player = PLAYER_BLACK;
  config.ai_black_cfg.enabled = true;

  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };
  ctx->ai_black = create_ai ("random", cm);
  ctx->is_ai_black_initialized = (ctx->ai_black.vtable != nullptr);

  guiCheckAndTriggerAI (ctx);
  EXPECT_TRUE (ctx->is_ai_thinking);
}

TEST_F (GuiControllerTest, CheckAndTriggerAI_AIDisabledForCurrentPlayer)
{
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

TEST_F (GuiControllerTest, StartNewGame_WithAIParams_Both)
{
  // Test d'injection de paramètres poussés
  config.ai_white_cfg.enabled = true;
  strncpy (config.ai_white_cfg.name, "random", MAX_AI_STR_LEN);
  config.ai_white_cfg.param_count = 1;
  strncpy (config.ai_white_cfg.param_keys[0], "depth", MAX_AI_STR_LEN);
  strncpy (config.ai_white_cfg.param_vals[0], "1", MAX_AI_STR_LEN);

  config.ai_black_cfg.enabled = true;
  strncpy (config.ai_black_cfg.name, "random", MAX_AI_STR_LEN);
  config.ai_black_cfg.param_count = 1;
  strncpy (config.ai_black_cfg.param_keys[0], "depth", MAX_AI_STR_LEN);
  strncpy (config.ai_black_cfg.param_vals[0], "1", MAX_AI_STR_LEN);

  EXPECT_NO_THROW (guiStartNewGame (ctx));
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
  guiStartNewGame (ctx);
  EXPECT_FALSE (ctx->is_ai_thinking);
}

/* ==========================================================================
 * NETWORK TESTS
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
 * ASYNCHRONOUS GTK EVENTS TESTS
 * ==========================================================================
 */

TEST_F (GuiControllerTest, AsyncAI_InterruptByNetwork)
{
  ctx->gameState->current_player = PLAYER_WHITE;
  config.ai_white_cfg.enabled = true;

  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };
  ctx->ai_white = create_ai ("random", cm);
  ctx->is_ai_white_initialized = (ctx->ai_white.vtable != nullptr);

  guiCheckAndTriggerAI (ctx);
  EXPECT_TRUE (ctx->is_ai_thinking);

  // Un paquet réseau arrive pendant que l'IA s'apprête à jouer !
  ctx->isNetworkMatch = true;

  int max_retries = 100;
  while (ctx->is_ai_thinking && max_retries > 0)
    {
      g_usleep (10000);
      while (g_main_context_pending (nullptr))
        g_main_context_iteration (nullptr, FALSE);
      max_retries--;
    }

  // Le timer a été interrompu et supprimé (G_SOURCE_REMOVE) sans appliquer le
  // coup
  EXPECT_FALSE (ctx->is_ai_thinking);
  EXPECT_EQ (ctx->gameState->current_player,
             PLAYER_WHITE); // Le tour n'a pas changé
}

TEST_F (GuiControllerTest, AsyncAI_Execution_BlackTurn)
{
  ctx->gameState->current_player = PLAYER_BLACK;
  config.ai_black_cfg.enabled = true;

  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };
  ctx->ai_black = create_ai ("random", cm);
  ctx->is_ai_black_initialized = (ctx->ai_black.vtable != nullptr);

  guiCheckAndTriggerAI (ctx);
  EXPECT_TRUE (ctx->is_ai_thinking);

  int max_retries = 100;
  while (ctx->is_ai_thinking && max_retries > 0)
    {
      g_usleep (10000);
      while (g_main_context_pending (nullptr))
        g_main_context_iteration (nullptr, FALSE);
      max_retries--;
    }

  // L'IA Noire a joué, le tour passe aux Blancs
  EXPECT_FALSE (ctx->is_ai_thinking);
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_WHITE);
}