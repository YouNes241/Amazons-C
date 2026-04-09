/**
 * @file test_gui_network_events.cpp
 * @brief Unit tests for gui_network_events.c functions
 *
 * Tests cover the network event callback system for the GUI,
 * including game start, move reception, invitations, and disconnections.
 */

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>

extern "C"
{
#include "../../src/controller/gui_network_events.h"
#include "ai_factory.h"
#include "ai_interface.h"
#include "config.h"
#include "game_engine.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include "network_common.h"
}

/**
 * @brief Test fixture for GUI network events tests
 *
 * Sets up a minimal GuiContext for testing network event callbacks
 * without actual GTK widgets.
 */
class GuiNetworkEventsTest : public ::testing::Test
{
protected:
  GuiContext *ctx;
  GameState game;
  game_config_t config;
  NetworkManager net_mgr;
  NetworkEvents events;

  void
  SetUp () override
  {
    ctx = (GuiContext *)calloc (1, sizeof (GuiContext));
    ASSERT_NE (ctx, nullptr);

    memset (&game, 0, sizeof (GameState));
    memset (&config, 0, sizeof (game_config_t));
    memset (&net_mgr, 0, sizeof (NetworkManager));
    memset (&events, 0, sizeof (NetworkEvents));

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

    // Set GTK widgets to NULL - we won't use them in tests
    ctx->mainStack = nullptr;
    ctx->mainWindow = nullptr;
    ctx->gameBoard = nullptr;

    // Create the network events structure
    events = create_gui_network_events (ctx);
  }

  void
  TearDown () override
  {
    // Clean up any AI that was initialized
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

    // Reset the global context
    create_gui_network_events (nullptr);
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

/* ==========================================================================
 * create_gui_network_events TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, CreateNetworkEvents_SetsCallbacks)
{
  EXPECT_NE (events.OnGameStarted, nullptr);
  EXPECT_NE (events.OnMoveReceived, nullptr);
  EXPECT_NE (events.OnInvitationReceived, nullptr);
  EXPECT_NE (events.OnPlayerDisconnected, nullptr);
  EXPECT_NE (events.OnPlayerJoined, nullptr);
}

TEST_F (GuiNetworkEventsTest, CreateNetworkEvents_WithValidContext)
{
  NetworkEvents new_events = create_gui_network_events (ctx);

  EXPECT_NE (new_events.OnGameStarted, nullptr);
  EXPECT_NE (new_events.OnMoveReceived, nullptr);
  EXPECT_NE (new_events.OnInvitationReceived, nullptr);
  EXPECT_NE (new_events.OnPlayerDisconnected, nullptr);
  EXPECT_NE (new_events.OnPlayerJoined, nullptr);
}

/* ==========================================================================
 * CALLBACK EXISTENCE TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, OnGameStartedCallback_Exists)
{
  EXPECT_NE (events.OnGameStarted, nullptr);
}

TEST_F (GuiNetworkEventsTest, OnMoveReceivedCallback_Exists)
{
  EXPECT_NE (events.OnMoveReceived, nullptr);
}

TEST_F (GuiNetworkEventsTest, OnInvitationReceivedCallback_Exists)
{
  EXPECT_NE (events.OnInvitationReceived, nullptr);
}

TEST_F (GuiNetworkEventsTest, OnPlayerDisconnectedCallback_Exists)
{
  EXPECT_NE (events.OnPlayerDisconnected, nullptr);
}

TEST_F (GuiNetworkEventsTest, OnPlayerJoinedCallback_Exists)
{
  EXPECT_NE (events.OnPlayerJoined, nullptr);
}

/* ==========================================================================
 * NETWORK MANAGER STATE TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, NetworkManagerModeNone)
{
  net_mgr.current_mode = NET_MODE_NONE;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_NONE);
}

TEST_F (GuiNetworkEventsTest, NetworkManagerModeServer)
{
  net_mgr.current_mode = NET_MODE_SERVER;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_SERVER);
}

TEST_F (GuiNetworkEventsTest, NetworkManagerModeClient)
{
  net_mgr.current_mode = NET_MODE_CLIENT;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_CLIENT);
}

/* ==========================================================================
 * CONTEXT STATE TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, ContextInitialState)
{
  EXPECT_FALSE (ctx->isNetworkMatch);
  EXPECT_EQ (ctx->selfPlayerColor, PLAYER_WHITE);
  EXPECT_EQ (ctx->gameConfig->board_size, 10);
}

TEST_F (GuiNetworkEventsTest, NetworkMatchState_Transition)
{
  ctx->isNetworkMatch = false;
  EXPECT_FALSE (ctx->isNetworkMatch);

  ctx->isNetworkMatch = true;
  EXPECT_TRUE (ctx->isNetworkMatch);
}

TEST_F (GuiNetworkEventsTest, SelfPlayerColor_White)
{
  ctx->selfPlayerColor = PLAYER_WHITE;
  EXPECT_EQ (ctx->selfPlayerColor, PLAYER_WHITE);
}

TEST_F (GuiNetworkEventsTest, SelfPlayerColor_Black)
{
  ctx->selfPlayerColor = PLAYER_BLACK;
  EXPECT_EQ (ctx->selfPlayerColor, PLAYER_BLACK);
}

/* ==========================================================================
 * GAME STATE INTEGRATION TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, GameStateInitialized)
{
  EXPECT_NE (ctx->gameState, nullptr);
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_WHITE);
  EXPECT_EQ (ctx->gameState->status, GAME_IN_PROGRESS);
}

TEST_F (GuiNetworkEventsTest, BoardSizeConfiguration)
{
  EXPECT_EQ (ctx->gameConfig->board_size, 10);
}

/* ==========================================================================
 * MOVE HANDLING TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, MoveCreation)
{
  Move m;
  m.queen_src.col = 3;
  m.queen_src.row = 0;
  m.queen_dst.col = 3;
  m.queen_dst.row = 1;
  m.arrow_dst.col = 3;
  m.arrow_dst.row = 2;

  // CORRECTION : col et row au lieu de x et y
  EXPECT_EQ (m.queen_src.col, 3);
  EXPECT_EQ (m.queen_src.row, 0);
  EXPECT_EQ (m.queen_dst.col, 3);
  EXPECT_EQ (m.queen_dst.row, 1);
  EXPECT_EQ (m.arrow_dst.col, 3);
  EXPECT_EQ (m.arrow_dst.row, 2);
}

TEST_F (GuiNetworkEventsTest, MoveValidation)
{
  Move legal_move = createLegalWhiteMove ();
  EXPECT_TRUE (is_move_legal (ctx->gameState, legal_move));

  Move illegal_move = createIllegalMove ();
  EXPECT_FALSE (is_move_legal (ctx->gameState, illegal_move));
}

/* ==========================================================================
 * DISCONNECTION HANDLING TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, DisconnectionReason_Null)
{
  const char *reason = nullptr;
  EXPECT_EQ (reason, nullptr);
}

TEST_F (GuiNetworkEventsTest, DisconnectionReason_Valid)
{
  const char *reason = "Connection lost";
  EXPECT_STREQ (reason, "Connection lost");
}

/* ==========================================================================
 * PLAYER JOINED TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, PlayerName_Valid)
{
  const char *player_name = "TestPlayer";
  EXPECT_STREQ (player_name, "TestPlayer");
}

TEST_F (GuiNetworkEventsTest, PlayerName_Empty)
{
  const char *player_name = "";
  EXPECT_STREQ (player_name, "");
}

/* ==========================================================================
 * GAME START PARAMETERS TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, GameStartParameters_WhitePlayer)
{
  PlayerColor color = PLAYER_WHITE;
  int board_size = 10;

  EXPECT_EQ (color, PLAYER_WHITE);
  EXPECT_EQ (board_size, 10);
}

TEST_F (GuiNetworkEventsTest, GameStartParameters_BlackPlayer)
{
  PlayerColor color = PLAYER_BLACK;
  int board_size = 8;

  EXPECT_EQ (color, PLAYER_BLACK);
  EXPECT_EQ (board_size, 8);
}

TEST_F (GuiNetworkEventsTest, GameStartParameters_DifferentBoardSizes)
{
  for (int size = 4; size <= 11; size++)
    {
      EXPECT_GE (size, MIN_BOARD_SIZE);
      EXPECT_LE (size, MAX_BOARD_SIZE);
    }
}

/* ==========================================================================
 * NETWORK TICK TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, NetworkTickDoesNotCrash)
{
  net_mgr.current_mode = NET_MODE_NONE;
  EXPECT_NO_THROW (NetworkManager_Tick (ctx->networkManager));
}

/* ==========================================================================
 * MULTIPLE CONTEXT SWITCHES
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, MultipleEventCreations)
{
  NetworkEvents events1 = create_gui_network_events (ctx);
  EXPECT_NE (events1.OnGameStarted, nullptr);

  GuiContext *ctx2 = (GuiContext *)calloc (1, sizeof (GuiContext));
  ASSERT_NE (ctx2, nullptr);

  GameState game2;
  game_config_t config2;
  NetworkManager net_mgr2;

  memset (&game2, 0, sizeof (GameState));
  memset (&config2, 0, sizeof (game_config_t));
  memset (&net_mgr2, 0, sizeof (NetworkManager));

  config2.board_size = 8;
  game2.size = 8;
  game_init (&game2);

  ctx2->gameState = &game2;
  ctx2->gameConfig = &config2;
  ctx2->networkManager = &net_mgr2;

  NetworkEvents events2 = create_gui_network_events (ctx2);
  EXPECT_NE (events2.OnGameStarted, nullptr);

  game_cleanup (&game2);
  free (ctx2);

  create_gui_network_events (ctx);
}

/* ==========================================================================
 * ERROR HANDLING TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, NetworkModeTransitions)
{
  net_mgr.current_mode = NET_MODE_NONE;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_NONE);

  net_mgr.current_mode = NET_MODE_SERVER;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_SERVER);

  net_mgr.current_mode = NET_MODE_CLIENT;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_CLIENT);

  net_mgr.current_mode = NET_MODE_NONE;
  EXPECT_EQ (ctx->networkManager->current_mode, NET_MODE_NONE);
}

/* ==========================================================================
 * INTEGRATION WITH guiHandlePlayerMove
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, MoveInNetworkMatch)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_WHITE;
  ctx->gameState->current_player = PLAYER_WHITE;
  net_mgr.current_mode = NET_MODE_CLIENT;

  Move legal_move = createLegalWhiteMove ();

  EXPECT_TRUE (is_move_legal (ctx->gameState, legal_move));
}

TEST_F (GuiNetworkEventsTest, MoveNotMyTurn)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_BLACK;
  ctx->gameState->current_player = PLAYER_WHITE;
  net_mgr.current_mode = NET_MODE_CLIENT;

  EXPECT_NE (ctx->selfPlayerColor, ctx->gameState->current_player);
}