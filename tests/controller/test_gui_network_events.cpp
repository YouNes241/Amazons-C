/**
 * @file test_gui_network_events.cpp
 * @brief Unit tests for GUI network event callbacks.
 *
 * @details This test suite verifies the behavior of the network event handlers
 * defined in `gui_network_events.c`. It tests how the UI reacts to network
 * events such as a game starting, receiving a move, invitations, and player
 * connections/disconnections.
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
 * @class GuiNetworkEventsTest
 * @brief Test fixture for testing GUI network events.
 *
 * @details Initializes a mock GUI context, a dynamic game state, and a minimal
 * GTK widget tree (Stack, Windows, Grids) to safely simulate network events
 * without requiring a physical display.
 */
class GuiNetworkEventsTest : public ::testing::Test
{
protected:
  GuiContext *ctx;        /**< Pointer to the main GUI context. */
  game_config_t config;   /**< Game configuration instance. */
  NetworkManager net_mgr; /**< Network manager instance. */
  NetworkEvents events;   /**< Network event callbacks structure. */
  static bool
      gtk_initialized; /**< Flag ensuring GTK is initialized only once. */

  /**
   * @brief Sets up the test suite before any tests run.
   * Initializes GTK in headless mode to prevent CI environment crashes.
   */
  static void
  SetUpTestSuite ()
  {
    if (!gtk_initialized)
      {
        g_setenv ("GTK_A11Y", "none",
                  TRUE); // Anti-crash for headless environments
        gtk_init ();
        gtk_initialized = true;
      }
  }

  /**
   * @brief Prepares the environment before each test.
   * Allocates memory, initializes default game settings, and sets up mock GTK
   * widgets.
   */
  void
  SetUp () override
  {
    ctx = (GuiContext *)calloc (1, sizeof (GuiContext));
    ASSERT_NE (ctx, nullptr);

    // Dynamic allocation ensures setupBoard() can safely call free() without
    // crashing.
    ctx->gameState = (GameState *)calloc (1, sizeof (GameState));
    ASSERT_NE (ctx->gameState, nullptr);

    memset (&config, 0, sizeof (game_config_t));
    memset (&net_mgr, 0, sizeof (NetworkManager));

    config.board_size = 10;

    ctx->gameState->size = 10;
    game_init (ctx->gameState);

    ctx->gameConfig = &config;
    ctx->networkManager = &net_mgr;
    ctx->isNetworkMatch = false;
    ctx->is_ai_thinking = false;
    ctx->selfPlayerColor = PLAYER_WHITE;
    ctx->currentPhase = PHASE_MOVE_QUEEN;
    ctx->isHintActive = false;
    ctx->is_ai_white_initialized = false;
    ctx->is_ai_black_initialized = false;

    // Minimal GTK simulation setup
    ctx->mainStack = GTK_STACK (gtk_stack_new ());
    ctx->mainWindow = GTK_WINDOW (gtk_window_new ());

    GtkWidget *boardGrid = gtk_grid_new ();
    g_object_set_data (G_OBJECT (ctx->mainStack), "board_grid", boardGrid);

    GtkWidget *turnLabel = gtk_label_new ("Turn");
    g_object_set_data (G_OBJECT (ctx->mainStack), "turn_label", turnLabel);

    GtkWidget *consoleView = gtk_text_view_new ();
    g_object_set_data (G_OBJECT (ctx->mainStack), "server_console_view",
                       consoleView);

    GtkWidget *game_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *client_panel_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_stack_add_named (ctx->mainStack, game_page, "game_page");
    gtk_stack_add_named (ctx->mainStack, client_panel_page,
                         "client_panel_page");

    events = create_gui_network_events (ctx);
  }

  /**
   * @brief Cleans up the environment after each test.
   * Frees allocated memory and destroys AI instances.
   */
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

    if (ctx->gameState)
      {
        game_cleanup (ctx->gameState);
        free (ctx->gameState);
        ctx->gameState = nullptr;
      }

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

    // Reset the global network event context
    create_gui_network_events (nullptr);
  }
};

bool GuiNetworkEventsTest::gtk_initialized = false;

/* ==========================================================================
 * CALLBACK EXECUTION TESTS
 * ==========================================================================
 */

TEST_F (GuiNetworkEventsTest, Execute_OnGameStarted)
{
  EXPECT_NO_THROW (events.OnGameStarted (PLAYER_BLACK, 8));

  EXPECT_EQ (ctx->selfPlayerColor, PLAYER_BLACK);
  EXPECT_TRUE (ctx->isNetworkMatch);
  EXPECT_EQ (ctx->gameConfig->board_size, 8);

  // Verify that the UI successfully switched to the game page
  EXPECT_STREQ (gtk_stack_get_visible_child_name (ctx->mainStack),
                "game_page");
}

TEST_F (GuiNetworkEventsTest, Execute_OnMoveReceived)
{
  // Setup a running network game
  events.OnGameStarted (PLAYER_WHITE, 10);

  Move m;
  m.queen_src.col = 3;
  m.queen_src.row = 9;
  m.queen_dst.col = 3;
  m.queen_dst.row = 8;
  m.arrow_dst.col = 3;
  m.arrow_dst.row = 7;

  ctx->gameState->current_player = PLAYER_BLACK;

  EXPECT_NO_THROW (events.OnMoveReceived (m));

  // Verify the turn shifted to White after Black's move was processed
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_WHITE);
}

TEST_F (GuiNetworkEventsTest, Execute_OnInvitationReceived)
{
  // Simulates receiving an invitation from player ID 42
  EXPECT_NO_THROW (events.OnInvitationReceived (42));
}

TEST_F (GuiNetworkEventsTest, Execute_OnPlayerDisconnected_ClientMode)
{
  net_mgr.current_mode = NET_MODE_CLIENT;
  ctx->isNetworkMatch = true;

  EXPECT_NO_THROW (events.OnPlayerDisconnected ("Timeout"));

  // The match should be immediately terminated and UI returned to the lobby
  EXPECT_FALSE (ctx->isNetworkMatch);
  EXPECT_STREQ (gtk_stack_get_visible_child_name (ctx->mainStack),
                "client_panel_page");
}

TEST_F (GuiNetworkEventsTest, Execute_OnPlayerDisconnected_ServerMode)
{
  net_mgr.current_mode = NET_MODE_SERVER;

  // The server simply logs the disconnection to the console
  EXPECT_NO_THROW (events.OnPlayerDisconnected ("Timeout"));
}

TEST_F (GuiNetworkEventsTest, Execute_OnPlayerJoined_ServerMode)
{
  net_mgr.current_mode = NET_MODE_SERVER;

  // The server logs the connection to the console
  EXPECT_NO_THROW (events.OnPlayerJoined ("Alice"));
}

TEST_F (GuiNetworkEventsTest, CreateWithNull_ReturnsEmpty)
{
  // Sending NULL acts as a reset command and returns an empty structure
  NetworkEvents empty_events = create_gui_network_events (nullptr);
  EXPECT_EQ (empty_events.OnGameStarted, nullptr);
}