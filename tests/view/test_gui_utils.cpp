/**
 * @file test_gui_utils.cpp
 * @brief Unit tests for gui_utils.c functions
 *
 * Tests cover GUI utility functions, GTK object manipulations,
 * CSS class toggling, and dynamic board generation.
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

class GuiUtilsTest : public ::testing::Test
{
protected:
  GuiContext *ctx;
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
    init_default_config (&config);
    config.board_size = 10;

    ctx = initGuiContext (&config);
    ASSERT_NE (ctx, nullptr);

    ctx->networkManager = &net_mgr;

    ctx->gameState = (GameState *)calloc (1, sizeof (GameState));
    ctx->gameState->size = 10;
    game_init (ctx->gameState);

    ctx->mainWindow = GTK_WINDOW (gtk_window_new ());
    ctx->mainStack = GTK_STACK (gtk_stack_new ());

    GtkWidget *turnLabel = gtk_label_new ("Turn");
    g_object_set_data (G_OBJECT (ctx->mainStack), "turn_label", turnLabel);

    GtkWidget *consoleView = gtk_text_view_new ();
    g_object_set_data (G_OBJECT (ctx->mainStack), "server_console_view",
                       consoleView);

    GtkWidget *gameBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    g_object_set_data (G_OBJECT (ctx->mainStack), "game_box", gameBox);

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

TEST_F (GuiUtilsTest, DrawPiece_Arrow)
{
  GtkWidget *cell = ctx->gameBoard[2];
  drawPiece (ctx, 2, 0, CONTENT_ARROW);

  EXPECT_TRUE (gtk_widget_has_css_class (cell, "piece-arrow"));

  GtkLabel *lbl
      = GTK_LABEL (g_object_get_data (G_OBJECT (cell), "cell_label"));
  EXPECT_STREQ (gtk_label_get_text (lbl), "🔥");
}

TEST_F (GuiUtilsTest, DrawPiece_Empty)
{
  GtkWidget *cell = ctx->gameBoard[3];

  drawPiece (ctx, 3, 0, CONTENT_WHITE_QUEEN);
  drawPiece (ctx, 3, 0, CONTENT_EMPTY);

  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-white"));
  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-black"));
  EXPECT_FALSE (gtk_widget_has_css_class (cell, "piece-arrow"));

  GtkLabel *lbl
      = GTK_LABEL (g_object_get_data (G_OBJECT (cell), "cell_label"));
  EXPECT_STREQ (gtk_label_get_text (lbl), "");
}

TEST_F (GuiUtilsTest, DrawBoard_UpdatesAllCells)
{
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

TEST_F (GuiUtilsTest, HighlightReachableSquares_AppliesClasses)
{
  for (int i = 0; i < 100; i++)
    ctx->gameState->board[i / 10][i % 10] = CONTENT_EMPTY;

  ctx->gameState->board[3][3] = CONTENT_WHITE_QUEEN;

  highlightReachableSquares (ctx, 3, 3);

  int start_idx = 3 * 10 + 3;
  int adjacent_idx = 3 * 10 + 4;

  EXPECT_TRUE (
      gtk_widget_has_css_class (ctx->gameBoard[start_idx], "active-queen"));

  EXPECT_TRUE (gtk_widget_has_css_class (ctx->gameBoard[adjacent_idx],
                                         "highlight-valid"));
  EXPECT_EQ (GPOINTER_TO_INT (g_object_get_data (
                 G_OBJECT (ctx->gameBoard[adjacent_idx]), "is_legal_move")),
             1);
}

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

TEST_F (GuiUtilsTest, UpdateTurnDisplay_LocalWhite)
{
  ctx->isNetworkMatch = false;
  ctx->gameState->current_player = PLAYER_WHITE;

  updateTurnDisplay (ctx);

  GtkLabel *turnLabel = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->mainStack), "turn_label"));
  EXPECT_STREQ (gtk_label_get_text (turnLabel), "Turn : White");
}

TEST_F (GuiUtilsTest, UpdateTurnDisplay_LocalBlack)
{
  ctx->isNetworkMatch = false;
  ctx->gameState->current_player = PLAYER_BLACK;

  updateTurnDisplay (ctx);

  GtkLabel *turnLabel = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->mainStack), "turn_label"));
  EXPECT_STREQ (gtk_label_get_text (turnLabel), "Turn : Black");
}

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

TEST_F (GuiUtilsTest, GuiLogToServerConsole_AppendsText)
{
  gui_log_to_server_console (ctx, "Test Message %d", 42);

  GtkTextView *console = GTK_TEXT_VIEW (
      g_object_get_data (G_OBJECT (ctx->mainStack), "server_console_view"));
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (console);

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  char *text = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  // Le texte doit contenir "Test Message 42\n"
  EXPECT_NE (strstr (text, "Test Message 42\n"), nullptr);
  g_free (text);
}

TEST_F (GuiUtilsTest, GuiShowPopup_DoesNotCrash)
{
  // En mode headless, afficher un popup ne fait qu'allouer la fenêtre (pas de
  // crash attendu)
  EXPECT_NO_THROW (guiShowPopup (ctx, "Test Popup"));
}

/* ==========================================================================
 * BINDING BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiUtilsTest, SetupBindingButton_InitializesCorrectly)
{
  GtkWidget *btn = gtk_toggle_button_new ();

  // Utilise le buffer existant dans ctx pour simuler target_str
  setupBindingButton (btn, ctx->keyBindings.key_pause, ctx);

  EXPECT_STREQ (gtk_button_get_label (GTK_BUTTON (btn)),
                ctx->keyBindings.key_pause);

  // Simulation de l'appui sur le bouton (Toggled -> "Press...")
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn), TRUE);
  EXPECT_STREQ (gtk_button_get_label (GTK_BUTTON (btn)), "Press...");
}

/* ==========================================================================
 * SHORTCUT CONTROLLER TESTS
 * ==========================================================================
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

TEST_F (GuiUtilsTest, SetupBoard_CreatesGridAndGameState)
{
  // On crée une grille GTK vide
  GtkWidget *grid = gtk_grid_new ();

  // On demande à setupBoard de peupler la grille (10x10 = 100 cases)
  EXPECT_NO_THROW (setupBoard (ctx, grid));

  EXPECT_NE (ctx->gameState, nullptr);
  EXPECT_NE (ctx->gameBoard, nullptr);
  EXPECT_EQ (ctx->gameState->size, 10);
  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);

  // Vérifie que le plateau est bien initialisé et rempli de GtkBox (cases)
  EXPECT_TRUE (GTK_IS_BOX (ctx->gameBoard[0]));

  // Vérifie qu'un label est bien injecté
  GtkLabel *lbl = GTK_LABEL (
      g_object_get_data (G_OBJECT (ctx->gameBoard[0]), "cell_label"));
  EXPECT_NE (lbl, nullptr);

  // Vérifie l'assignation de couleurs (damier css)
  EXPECT_TRUE (gtk_widget_has_css_class (ctx->gameBoard[0], "cell-light")
               || gtk_widget_has_css_class (ctx->gameBoard[0], "cell-dark"));
}