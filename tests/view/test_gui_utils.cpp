/**
 * @file test_gui_utils.cpp
 * @brief Unit tests for GUI utility functions.
 *
 * @details This test suite covers the utility functions defined in
 * `gui_utils.c`. It tests GTK object manipulations, CSS class toggling for
 * drawing pieces and highlights, dynamic board generation, and UI state
 * updates in a safely mocked headless GTK environment.
 */

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>

extern "C"
{
#include "../../src/view/gui_callbacks.h"
#include "config.h"
#include "game_engine.h"
#include "gui_utils.h"
#include "network_common.h"
}

/**
 * @class GuiUtilsTest
 * @brief Test fixture for GUI utilities.
 *
 * @details Initializes a headless GTK environment and sets up a mock
 * application state with essential widgets (Stack, TextView, Grid, etc.)
 * to safely test rendering logic and UI updates without a physical display.
 */
class GuiUtilsTest : public ::testing::Test
{
protected:
  GuiContext *ctx;        /**< Pointer to the main GUI context. */
  game_config_t config;   /**< Game configuration structure. */
  NetworkManager net_mgr; /**< Network manager instance. */
  static bool
      gtk_initialized; /**< Flag to ensure GTK initializes only once. */

  /**
   * @brief Sets up the test suite before any tests run.
   * Initializes GTK in headless mode to prevent CI environment crashes.
   */
  static void
  SetUpTestSuite ()
  {
    if (!gtk_initialized)
      {
        g_setenv ("GTK_A11Y", "none", TRUE); // Anti-crash for headless mode
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
    init_default_config (&config);
    config.board_size = 10;

    ctx = initGuiContext (&config);
    ASSERT_NE (ctx, nullptr);

    ctx->networkManager = &net_mgr;

    ctx->gameState = (GameState *)calloc (1, sizeof (GameState));
    ctx->gameState->size = 10;
    game_init (ctx->gameState);

    // Minimal GTK simulation setup
    ctx->mainWindow = GTK_WINDOW (gtk_window_new ());
    ctx->mainStack = GTK_STACK (gtk_stack_new ());

    GtkWidget *turnLabel = gtk_label_new ("Turn");
    g_object_set_data (G_OBJECT (ctx->mainStack), "turn_label", turnLabel);

    GtkWidget *consoleView = gtk_text_view_new ();
    g_object_set_data (G_OBJECT (ctx->mainStack), "server_console_view",
                       consoleView);

    GtkWidget *gameBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    g_object_set_data (G_OBJECT (ctx->mainStack), "game_box", gameBox);

    // Manually allocate a mock board for simple unit tests
    int boardSize = config.board_size * config.board_size;
    ctx->gameBoard = (GtkWidget **)calloc (boardSize, sizeof (GtkWidget *));
    for (int i = 0; i < boardSize; i++)
      {
        ctx->gameBoard[i] = gtk_button_new ();
        GtkWidget *lbl = gtk_label_new ("");
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "cell_label", lbl);
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "cell_index",
                           GINT_TO_POINTER (i));
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "is_legal_move",
                           GINT_TO_POINTER (0));
      }
  }

  /**
   * @brief Cleans up the environment after each test.
   * Frees the allocated GUI context and associated resources.
   */
  void
  TearDown () override
  {
    if (ctx)
      {
        freeGuiContext (ctx);
        ctx = nullptr;
      }
  }
};

bool GuiUtilsTest::gtk_initialized = false;

/* ==========================================================================
 * INITIALIZATION & CLEANUP TESTS
 * ==========================================================================
 */

/**
 * @brief Tests the default initialization of the GUI Context.
 */
TEST_F (GuiUtilsTest, InitGuiContext_SetsDefaults)
{
  EXPECT_EQ (ctx->gameConfig, &config);
  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);
  EXPECT_FALSE (ctx->isNetworkMatch);
  EXPECT_FALSE (ctx->isHintActive);
  EXPECT_FALSE (ctx->is_ai_thinking);

  EXPECT_STREQ (ctx->keyBindings.key_new_game, "<Control>n");
}

/* ==========================================================================
 * DRAWING TESTS
 * ==========================================================================
 */

/**
 * @brief Verifies rendering of a White Queen piece.
 */
TEST_F (GuiUtilsTest, DrawPiece_WhiteQueen)
{
  GtkWidget *cell = ctx->gameBoard[0];
  drawPiece (ctx, 0, 0, CONTENT_WHITE_QUEEN);

  EXPECT_TRUE (gtk_widget_has_css_class (cell, "piece-white"));
  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-black"));

  GtkLabel *lbl
      = GTK_LABEL (g_object_get_data (G_OBJECT (cell), "cell_label"));
  EXPECT_STREQ (gtk_label_get_text (lbl), "♛");
}

/**
 * @brief Verifies rendering of a Black Queen piece.
 */
TEST_F (GuiUtilsTest, DrawPiece_BlackQueen)
{
  GtkWidget *cell = ctx->gameBoard[1];
  drawPiece (ctx, 1, 0, CONTENT_BLACK_QUEEN);

  EXPECT_TRUE (gtk_widget_has_css_class (cell, "piece-black"));
  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-arrow"));

  GtkLabel *lbl
      = GTK_LABEL (g_object_get_data (G_OBJECT (cell), "cell_label"));
  EXPECT_STREQ (gtk_label_get_text (lbl), "♛");
}

/**
 * @brief Verifies rendering of an Arrow piece.
 */
TEST_F (GuiUtilsTest, DrawPiece_Arrow)
{
  GtkWidget *cell = ctx->gameBoard[2];
  drawPiece (ctx, 2, 0, CONTENT_ARROW);

  EXPECT_TRUE (gtk_widget_has_css_class (cell, "piece-arrow"));

  GtkLabel *lbl
      = GTK_LABEL (g_object_get_data (G_OBJECT (cell), "cell_label"));
  EXPECT_STREQ (gtk_label_get_text (lbl), "🔥");
}

/**
 * @brief Verifies that rendering an empty cell correctly clears previous
 * contents.
 */
TEST_F (GuiUtilsTest, DrawPiece_Empty)
{
  GtkWidget *cell = ctx->gameBoard[3];

  // First place a queen to ensure drawPiece clears it properly
  drawPiece (ctx, 3, 0, CONTENT_WHITE_QUEEN);
  drawPiece (ctx, 3, 0, CONTENT_EMPTY);

  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-white"));
  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-black"));
  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-arrow"));

  GtkLabel *lbl
      = GTK_LABEL (g_object_get_data (G_OBJECT (cell), "cell_label"));
  EXPECT_STREQ (gtk_label_get_text (lbl), "");
}

/**
 * @brief Verifies that the entire board is synchronized with the GameState.
 */
TEST_F (GuiUtilsTest, DrawBoard_UpdatesAllCells)
{
  // Manually place elements in the GameState
  ctx->gameState->board[0][0] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[9][9] = CONTENT_BLACK_QUEEN;
  ctx->gameState->board[5][5] = CONTENT_ARROW;

  drawBoard (ctx);

  EXPECT_TRUE (gtk_widget_has_css_class (ctx->gameBoard[0], "piece-white"));
  EXPECT_TRUE (gtk_widget_has_css_class (ctx->gameBoard[99], "piece-black"));
  EXPECT_TRUE (gtk_widget_has_css_class (ctx->gameBoard[55], "piece-arrow"));
}

/* ==========================================================================
 * HIGHLIGHTS TESTS
 * ==========================================================================
 */

/**
 * @brief Verifies that highlight clearing removes CSS classes and internal
 * markers.
 */
TEST_F (GuiUtilsTest, ClearHighlights_RemovesCSSAndData)
{
  gtk_widget_add_css_class (ctx->gameBoard[0], "highlight-valid");
  g_object_set_data (G_OBJECT (ctx->gameBoard[0]), "is_legal_move",
                     GINT_TO_POINTER (1));

  clearHighlights (ctx);

  EXPECT_FALSE (
      gtk_widget_has_css_class (ctx->gameBoard[0], "highlight-valid"));
  EXPECT_EQ (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (ctx->gameBoard[0]),
                                                 "is_legal_move")),
             0);
}

/**
 * @brief Verifies that valid reachable squares are properly highlighted using
 * the engine logic.
 */
TEST_F (GuiUtilsTest, HighlightReachableSquares_AppliesClasses)
{
  // Setup: Clear the board and place a single White Queen at (3,3)
  for (int i = 0; i < 100; i++)
    ctx->gameState->board[i / 10][i % 10] = CONTENT_EMPTY;

  ctx->gameState->board[3][3] = CONTENT_WHITE_QUEEN;

  highlightReachableSquares (ctx, 3, 3);

  int start_idx = 3 * 10 + 3;
  int adjacent_idx = 3 * 10 + 4;

  // The source cell must be marked as active
  EXPECT_TRUE (
      gtk_widget_has_css_class (ctx->gameBoard[start_idx], "active-queen"));

  // An adjacent empty cell must be valid and highlighted
  EXPECT_TRUE (gtk_widget_has_css_class (ctx->gameBoard[adjacent_idx],
                                         "highlight-valid"));
  EXPECT_EQ (GPOINTER_TO_INT (g_object_get_data (
                 G_OBJECT (ctx->gameBoard[adjacent_idx]), "is_legal_move")),
             1);
}

/**
 * @brief Verifies that AI hints correctly apply target highlighting CSS
 * classes.
 */
TEST_F (GuiUtilsTest, ShowHintOnBoard_AppliesHintClasses)
{
  Position src;
  src.col = 3;
  src.row = 0;

  Position dst;
  dst.col = 3;
  dst.row = 1;

  Position arr;
  arr.col = 3;
  arr.row = 2;

  showHintOnBoard (ctx, src, dst, arr);

  int src_idx = 0 * 10 + 3; // Row 0, Col 3 = index 3
  int dst_idx = 1 * 10 + 3; // Row 1, Col 3 = index 13
  int arr_idx = 2 * 10 + 3; // Row 2, Col 3 = index 23

  EXPECT_TRUE (
      gtk_widget_has_css_class (ctx->gameBoard[src_idx], "hint-queen"));
  EXPECT_TRUE (
      gtk_widget_has_css_class (ctx->gameBoard[dst_idx], "hint-queen"));
  EXPECT_TRUE (
      gtk_widget_has_css_class (ctx->gameBoard[arr_idx], "hint-arrow"));
}

/**
 * @brief Verifies that clearing hint highlights removes the appropriate CSS
 * classes.
 */
TEST_F (GuiUtilsTest, ClearHintHighlights_RemovesHintClasses)
{
  gtk_widget_add_css_class (ctx->gameBoard[0], "hint-queen");
  gtk_widget_add_css_class (ctx->gameBoard[1], "hint-arrow");

  clearHintHighlights (ctx);

  EXPECT_FALSE (gtk_widget_has_css_class (ctx->gameBoard[0], "hint-queen"));
  EXPECT_FALSE (gtk_widget_has_css_class (ctx->gameBoard[1], "hint-arrow"));
}

/* ==========================================================================
 * TURN DISPLAY TESTS
 * ==========================================================================
 */

/**
 * @brief Verifies the turn display update for a local White turn.
 */
TEST_F (GuiUtilsTest, UpdateTurnDisplay_LocalWhite)
{
  ctx->isNetworkMatch = false;
  ctx->gameState->current_player = PLAYER_WHITE;

  updateTurnDisplay (ctx);

  GtkLabel *turnLabel = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->mainStack), "turn_label"));
  EXPECT_STREQ (gtk_label_get_text (turnLabel), "Turn : White");
}

/**
 * @brief Verifies the turn display update for a local Black turn.
 */
TEST_F (GuiUtilsTest, UpdateTurnDisplay_LocalBlack)
{
  ctx->isNetworkMatch = false;
  ctx->gameState->current_player = PLAYER_BLACK;

  updateTurnDisplay (ctx);

  GtkLabel *turnLabel = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->mainStack), "turn_label"));
  EXPECT_STREQ (gtk_label_get_text (turnLabel), "Turn : Black");
}

/**
 * @brief Verifies the turn display update during a network match when it's the
 * player's turn.
 */
TEST_F (GuiUtilsTest, UpdateTurnDisplay_NetworkMyTurn)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_WHITE;
  ctx->gameState->current_player = PLAYER_WHITE;

  updateTurnDisplay (ctx);

  GtkLabel *turnLabel = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->mainStack), "turn_label"));
  EXPECT_STREQ (gtk_label_get_text (turnLabel), "It is your turn !");
}

/**
 * @brief Verifies the turn display update during a network match when it's the
 * opponent's turn.
 */
TEST_F (GuiUtilsTest, UpdateTurnDisplay_NetworkOpponentTurn)
{
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_WHITE;
  ctx->gameState->current_player = PLAYER_BLACK;

  updateTurnDisplay (ctx);

  GtkLabel *turnLabel = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->mainStack), "turn_label"));
  EXPECT_STREQ (gtk_label_get_text (turnLabel), "Black is playing...");
}

/* ==========================================================================
 * CONSOLE & POPUP TESTS
 * ==========================================================================
 */

/**
 * @brief Verifies that logging to the server console appends text properly.
 */
TEST_F (GuiUtilsTest, GuiLogToServerConsole_AppendsText)
{
  gui_log_to_server_console (ctx, "Test Message %d", 42);

  GtkTextView *console = GTK_TEXT_VIEW (
      g_object_get_data (G_OBJECT (ctx->mainStack), "server_console_view"));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (console);

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  char *text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  // The text must contain "Test Message 42\n"
  EXPECT_NE (strstr (text, "Test Message 42\n"), nullptr);
  g_free (text);
}

/**
 * @brief Verifies that calling the popup display function does not crash the
 * headless environment.
 */
TEST_F (GuiUtilsTest, GuiShowPopup_DoesNotCrash)
{
  // In headless mode, showing a popup only allocates the window (no crash
  // expected)
  EXPECT_NO_THROW (guiShowPopup (ctx, "Test Popup"));
}

/* ==========================================================================
 * BINDING BUTTON TESTS
 * ==========================================================================
 */

/**
 * @brief Verifies that key binding buttons toggle correctly.
 */
TEST_F (GuiUtilsTest, SetupBindingButton_InitializesCorrectly)
{
  GtkWidget *btn = gtk_toggle_button_new ();

  // Use the existing buffer in ctx to simulate target_str
  setupBindingButton (btn, ctx->keyBindings.key_pause, ctx);

  EXPECT_STREQ (gtk_button_get_label (GTK_BUTTON (btn)),
                ctx->keyBindings.key_pause);

  // Simulate button press (Toggled -> 'Press...')
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn), TRUE);
  EXPECT_STREQ (gtk_button_get_label (GTK_BUTTON (btn)), "Press...");
}

/* ==========================================================================
 * SHORTCUT CONTROLLER TESTS
 * ==========================================================================
 */

/**
 * @brief Verifies that game shortcuts attach the GtkShortcutController to the
 * game view.
 */
TEST_F (GuiUtilsTest, UpdateGameShortcuts_AttachesController)
{
  GtkWidget *gameBox
      = GTK_WIDGET (g_object_get_data (G_OBJECT (ctx->mainStack), "game_box"));

  updateGameShortcuts (ctx);

  GtkEventController *ctrl = GTK_EVENT_CONTROLLER (
      g_object_get_data (G_OBJECT (gameBox), "shortcut_controller"));
  EXPECT_NE (ctrl, nullptr);
  EXPECT_TRUE (GTK_IS_SHORTCUT_CONTROLLER (ctrl));
}

/* ==========================================================================
 * FULL BOARD SETUP TEST
 * ==========================================================================
 */

/**
 * @brief Verifies that the dynamic board generation creates all cells
 * properly.
 */
TEST_F (GuiUtilsTest, SetupBoard_CreatesGridAndGameState)
{
  // Create an empty GTK grid
  GtkWidget *grid = gtk_grid_new ();

  // Ask setupBoard to populate the grid (10x10 = 100 cells)
  EXPECT_NO_THROW (setupBoard (ctx, grid));

  EXPECT_NE (ctx->gameState, nullptr);
  EXPECT_NE (ctx->gameBoard, nullptr);
  EXPECT_EQ (ctx->gameState->size, 10);
  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);

  // Verify that the board is properly initialized and filled with GtkBox
  // (cells)
  EXPECT_TRUE (GTK_IS_BOX (ctx->gameBoard[0]));

  // Verify that a label is properly injected
  GtkLabel *lbl = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->gameBoard[0]), "cell_label"));
  EXPECT_NE (lbl, nullptr);

  // Verify color assignment (CSS checkerboard pattern)
  EXPECT_TRUE (gtk_widget_has_css_class (ctx->gameBoard[0], "cell-light")
               || gtk_widget_has_css_class (ctx->gameBoard[0], "cell-dark"));
}