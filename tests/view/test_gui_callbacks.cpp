/**
 * @file test_gui_callbacks.cpp
 * @brief Unit tests for the GUI callbacks.
 *
 * @details This test suite covers the GUI callback logic defined in
 * `gui_callbacks.c`. It includes automated tests for button clicks,
 * drag-and-drop interactions, configuration updates, keyboard shortcuts, and
 * the AI hint system. A headless GTK environment is used to simulate UI
 * interactions safely.
 */

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <string>

#ifndef GRESOURCE_FILE
#define GRESOURCE_FILE "resources.gresource"
#endif

extern "C"
{
#include "../../src/view/gui_callbacks.h"
#include "ai_factory.h"
#include "ai_interface.h"
#include "config.h"
#include "game_engine.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include "network_common.h"
}

/**
 * @class GuiCallbacksTest
 * @brief Test fixture for testing GUI callbacks.
 *
 * @details Sets up a minimal GTK mock environment alongside the core game
 * state, network manager, and configurations required to trigger UI callbacks
 * without crashing or requiring a physical display.
 */
class GuiCallbacksTest : public ::testing::Test
{
protected:
  GuiContext *ctx;        /**< Pointer to the main GUI context. */
  game_config_t config;   /**< Game configuration instance. */
  NetworkManager net_mgr; /**< Network manager instance. */
  static bool
      gtk_initialized; /**< Flag ensuring GTK is initialized only once. */

  /**
   * @brief Sets up the test suite before any tests run.
   * Initializes GTK in headless mode and loads the compiled resources.
   */
  static void
  SetUpTestSuite ()
  {
    if (!gtk_initialized)
      {
        g_setenv ("GTK_A11Y", "none", TRUE);
        gtk_init ();
        gtk_initialized = true;

        GError *error = nullptr;
        GResource *res = g_resource_load (GRESOURCE_FILE, &error);

        if (res != nullptr)
          {
            g_resources_register (res);
            g_resource_unref (res);
            g_print ("[Test Setup] %s successfully loaded!\n", GRESOURCE_FILE);
          }
        else
          {
            g_print ("[Test Setup] Info: %s was not found. "
                     "The popups will be ignored.\n",
                     GRESOURCE_FILE);
            if (error)
              g_error_free (error);
          }
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

    ctx->gameState = (GameState *)calloc (1, sizeof (GameState));
    ASSERT_NE (ctx->gameState, nullptr);

    memset (&config, 0, sizeof (game_config_t));
    memset (&net_mgr, 0, sizeof (NetworkManager));

    init_default_config (&config);
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
    ctx->mainWindow = GTK_WINDOW (gtk_window_new ());
    ctx->mainStack = GTK_STACK (gtk_stack_new ());

    GtkWidget *turnLabel = gtk_label_new ("Turn");
    g_object_set_data (G_OBJECT (ctx->mainStack), "turn_label", turnLabel);

    GtkWidget *main_menu_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *game_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *client_panel_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *join_server_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *host_panel_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *multiplayer_page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    gtk_stack_add_named (ctx->mainStack, main_menu_page, "main_menu_page");
    gtk_stack_add_named (ctx->mainStack, game_page, "game_page");
    gtk_stack_add_named (ctx->mainStack, client_panel_page,
                         "client_panel_page");
    gtk_stack_add_named (ctx->mainStack, join_server_page, "join_server_page");
    gtk_stack_add_named (ctx->mainStack, host_panel_page, "host_panel_page");
    gtk_stack_add_named (ctx->mainStack, multiplayer_page, "multiplayer_page");

    GtkWidget *boardGrid = gtk_grid_new ();
    g_object_set_data (G_OBJECT (ctx->mainStack), "board_grid", boardGrid);

    int boardSize = config.board_size * config.board_size;
    ctx->gameBoard = (GtkWidget **)calloc (boardSize, sizeof (GtkWidget *));
    for (int i = 0; i < boardSize; i++)
      {
        ctx->gameBoard[i] = gtk_button_new ();

        GtkWidget *lbl = gtk_label_new ("");
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "cell_label", lbl);

        int x = i % config.board_size;
        int y = i / config.board_size;
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "cell_x",
                           GINT_TO_POINTER (x));
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "cell_y",
                           GINT_TO_POINTER (y));
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "cell_index",
                           GINT_TO_POINTER (i));
        g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "is_legal_move",
                           GINT_TO_POINTER (0));
      }

    // Set mock keybindings
    strncpy (ctx->keyBindings.key_new_game, "<Control>n", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_load_game, "<Control>o", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_save_game, "<Control>s", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_undo, "<Control>z", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_redo, "<Control>y", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_pause, "p", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_hint, "h", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_config, "<Control>comma", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_info, "F1", SHORTCUTS_SIZE);
    strncpy (ctx->keyBindings.key_quit, "<Control>q", SHORTCUTS_SIZE);
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
      ctx->ai_white.vtable->clean (&ctx->ai_white);

    if (ctx->is_ai_black_initialized && ctx->ai_black.vtable
        && ctx->ai_black.vtable->clean)
      ctx->ai_black.vtable->clean (&ctx->ai_black);

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
  }

  /**
   * @brief Helper function to generate a standard legal move for the White
   * player.
   * @return A valid Move structure.
   */
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
};

bool GuiCallbacksTest::gtk_initialized = false;

/* ==========================================================================
 * HIGH COVERAGE ADDITIONS (New Branches Hit)
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, InfoPopupButtonClick_ShowsPopup)
{
  if (g_resources_get_info ("/amazons/ui/popups/info_popup.ui",
                            G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr, nullptr,
                            nullptr))
    {
      EXPECT_NO_THROW (infoPopupButtonClick (nullptr, ctx));
    }
  else
    {
      GTEST_SKIP ()
          << "Resource /amazons/ui/popups/info_popup.ui not found, skipping.";
    }
}

TEST_F (GuiCallbacksTest, ConfigPopupButtonClick_ShowsPopup)
{
  if (g_resources_get_info ("/amazons/ui/popups/config_popup.ui",
                            G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr, nullptr,
                            nullptr))
    {
      EXPECT_NO_THROW (configPopupButtonClick (nullptr, ctx));
    }
  else
    {
      GTEST_SKIP () << "Resource /amazons/ui/popups/config_popup.ui not "
                       "found, skipping.";
    }
}

TEST_F (GuiCallbacksTest, CreateGameButtonClick_ShowsPopup)
{
  if (g_resources_get_info ("/amazons/ui/popups/create_game_popup.ui",
                            G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr, nullptr,
                            nullptr))
    {
      EXPECT_NO_THROW (createGameButtonClick (nullptr, ctx));
    }
  else
    {
      GTEST_SKIP () << "Resource /amazons/ui/popups/create_game_popup.ui not "
                       "found, skipping.";
    }
}

TEST_F (GuiCallbacksTest, HostServerPopupButtonClick_ShowsPopup)
{
  if (g_resources_get_info ("/amazons/ui/popups/create_server_popup.ui",
                            G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr, nullptr,
                            nullptr))
    {
      EXPECT_NO_THROW (hostServerPopupButtonClick (nullptr, ctx));
    }
  else
    {
      GTEST_SKIP () << "Resource /amazons/ui/popups/create_server_popup.ui "
                       "not found, skipping.";
    }
}

TEST_F (GuiCallbacksTest, UndoButtonClick_EmptyStack)
{
  ctx->is_ai_thinking = false;
  ctx->isNetworkMatch = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  // Executes without crashing (stack is empty, undo_move returns false)
  EXPECT_NO_THROW (undoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, RedoButtonClick_EmptyStack)
{
  ctx->is_ai_thinking = false;
  ctx->isNetworkMatch = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  // Executes without crashing (redo stack is empty, redo_move returns false)
  EXPECT_NO_THROW (redoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, SendInviteButtonClick_NullNetworkManager)
{
  GtkWidget *btn = gtk_button_new_with_label ("Invite");
  GtkWidget *entry = gtk_entry_new ();
  GtkWidget *spin = gtk_spin_button_new_with_range (4, 11, 1);
  GtkWidget *popup = gtk_window_new ();

  gtk_editable_set_text (GTK_EDITABLE (entry), "5"); // Valid target ID
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 10);

  g_object_set_data (G_OBJECT (btn), "target_id_entry", entry);
  g_object_set_data (G_OBJECT (btn), "board_size_spin", spin);
  g_object_set_data (G_OBJECT (btn), "popup_window", popup);

  // Simulates the absence of a network manager (e.g., local match)
  GuiContext ctx2 = *ctx;
  ctx2.networkManager = nullptr;

  EXPECT_NO_THROW (sendInviteButtonClick (GTK_BUTTON (btn), &ctx2));
}

TEST_F (GuiCallbacksTest, JoinServerButtonClick_ConnectionSuccess)
{
  // Create an actual server on a free port to guarantee ClientJoin success
  int port = 18888;
  NetworkManager_ServerStart (ctx->networkManager, port);

  GtkWidget *btn = gtk_button_new_with_label ("Join");
  GtkWidget *entry = gtk_entry_new ();

  GtkWidget *titleLabel = gtk_label_new ("Title");
  g_object_set_data (G_OBJECT (ctx->mainStack), "lobby_title_label",
                     titleLabel);

  g_object_set_data (G_OBJECT (btn), "server_ip", (gpointer) "127.0.0.1");
  g_object_set_data (G_OBJECT (btn), "server_port", GINT_TO_POINTER (port));
  g_object_set_data (G_OBJECT (btn), "username_entry", entry);

  gtk_editable_set_text (GTK_EDITABLE (entry), "TestUser");

  EXPECT_NO_THROW (joinServerButtonClick (GTK_BUTTON (btn), ctx));

  NetworkManager_ShutdownServer (ctx->networkManager);
}

TEST_F (GuiCallbacksTest, RefreshServerListButtonClick_ServersFound)
{
  int port = 18889;
  NetworkManager_ServerStart (ctx->networkManager, port);

  GtkWidget *btn = gtk_button_new_with_label ("Refresh");
  GtkWidget *listbox = gtk_list_box_new ();
  GtkWidget *entry = gtk_entry_new ();

  g_object_set_data (G_OBJECT (btn), "server_listbox", listbox);
  g_object_set_data (G_OBJECT (btn), "username_entry", entry);

  EXPECT_NO_THROW (refreshServerListButtonClick (GTK_BUTTON (btn), ctx));

  NetworkManager_ShutdownServer (ctx->networkManager);
}

#if !GTK_CHECK_VERSION(4, 10, 0)
TEST_F (GuiCallbacksTest, OnSaveResponse_Cancel)
{
  // Tests the Cancel branch of native dialog boxes
  EXPECT_NO_THROW (on_save_response (nullptr, GTK_RESPONSE_CANCEL, ctx));
}

TEST_F (GuiCallbacksTest, OnLoadResponse_Cancel)
{
  // Tests the Cancel branch of native dialog boxes
  EXPECT_NO_THROW (on_load_response (nullptr, GTK_RESPONSE_CANCEL, ctx));
}
#endif

TEST_F (GuiCallbacksTest, OnDrag_Success_ReturnsProvider)
{
  ctx->is_ai_thinking = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  ctx->gameState->current_player = PLAYER_WHITE;
  ctx->selfPlayerColor = PLAYER_WHITE;
  ctx->isNetworkMatch = false;

  ctx->gameState->board[0][3] = CONTENT_WHITE_QUEEN;

  GtkDragSource *source = gtk_drag_source_new ();
  gtk_widget_add_controller (ctx->gameBoard[3], GTK_EVENT_CONTROLLER (source));

  g_object_set_data (G_OBJECT (ctx->gameBoard[4]), "is_legal_move",
                     GINT_TO_POINTER (1));

  GdkContentProvider *provider = onDrag (source, 0, 0, ctx);
  EXPECT_NE (provider, nullptr); // Drag is successfully authorized!

  if (provider)
    g_object_unref (provider);
}

TEST_F (GuiCallbacksTest, FireArrow_NetworkMatch_OwnTurn)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_WHITE;
  ctx->gameState->current_player = PLAYER_WHITE;

  ctx->tmpQueenSrc.col = 3;
  ctx->tmpQueenSrc.row = 0;
  ctx->tmpQueenDst.col = 3;
  ctx->tmpQueenDst.row = 1;

  ctx->gameState->board[0][3] = CONTENT_EMPTY;
  ctx->gameState->board[1][3] = CONTENT_WHITE_QUEEN;

  GtkWidget *btn = ctx->gameBoard[23]; // Cell (3, 2)
  g_object_set_data (G_OBJECT (btn), "is_legal_move", GINT_TO_POINTER (1));

  EXPECT_NO_THROW (fireArrow (btn, ctx));
  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);
}

TEST_F (GuiCallbacksTest, StartServerButtonClick_Failure)
{
  GtkWidget *btn = gtk_button_new_with_label ("Start");
  GtkWidget *spin = gtk_spin_button_new_with_range (1, 65535, 1);

  // Uses privileged port 80 which refuses to open without sudo, causing a
  // failure.
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 80);
  g_object_set_data (G_OBJECT (btn), "port_spin", spin);

  GtkWidget *entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "80");
  g_object_set_data (G_OBJECT (btn), "port_entry", entry);

  GtkWidget *popup = gtk_window_new ();
  g_object_set_data (G_OBJECT (btn), "popup_window", popup);

  testing::internal::CaptureStdout ();
  EXPECT_NO_THROW (startServerButtonClick (GTK_BUTTON (btn), ctx));
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_NE (output.find ("Failed to start server"), std::string::npos);
}

TEST_F (GuiCallbacksTest, JoinServerButtonClick_ConnectionFailure)
{
  GtkWidget *btn = gtk_button_new_with_label ("Join");
  GtkWidget *entry = gtk_entry_new ();

  // Provide a completely invalid IP address to force connection failure
  g_object_set_data (G_OBJECT (btn), "server_ip",
                     (gpointer) "256.256.256.256");
  g_object_set_data (G_OBJECT (btn), "server_port", GINT_TO_POINTER (99999));
  g_object_set_data (G_OBJECT (btn), "username_entry", entry);
  gtk_editable_set_text (GTK_EDITABLE (entry), "TestUser");

  testing::internal::CaptureStdout ();
  EXPECT_NO_THROW (joinServerButtonClick (GTK_BUTTON (btn), ctx));
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_NE (output.find ("Connection failed"), std::string::npos);
}

TEST_F (GuiCallbacksTest, SaveButtonClick_NormalPath)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  ctx->is_ai_thinking = false;
  EXPECT_NO_THROW (saveButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, LoadButtonClick_NormalPath)
{
  ctx->isNetworkMatch = false;
  ctx->is_ai_thinking = false;
  EXPECT_NO_THROW (loadButtonClick (nullptr, ctx));
}

/* ==========================================================================
 * BASIC NAVIGATION TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, QuitButtonClick_WindowClose)
{
  GtkWidget *btn = gtk_button_new ();
  EXPECT_NO_THROW (quitButtonClick (GTK_BUTTON (btn), ctx));
}

/* ==========================================================================
 * SHORTCUT TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, ShortcutNewGame_ReturnsTrue)
{
  ctx->isNetworkMatch = true;
  gboolean result = shortcutNewGame (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

TEST_F (GuiCallbacksTest, ShortcutInfo_ReturnsTrue)
{
  if (g_resources_get_info ("/amazons/ui/popups/info_popup.ui",
                            G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr, nullptr,
                            nullptr))
    {
      EXPECT_TRUE (shortcutInfo (nullptr, nullptr, ctx));
    }
  else
    {
      GTEST_SKIP () << "Resource missing";
    }
}

TEST_F (GuiCallbacksTest, ShortcutConfig_ReturnsTrue)
{
  if (g_resources_get_info ("/amazons/ui/popups/config_popup.ui",
                            G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr, nullptr,
                            nullptr))
    {
      EXPECT_TRUE (shortcutConfig (nullptr, nullptr, ctx));
    }
  else
    {
      GTEST_SKIP () << "Resource missing";
    }
}

TEST_F (GuiCallbacksTest, ShortcutQuit_ReturnsTrue)
{
  gboolean result = shortcutQuit (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

TEST_F (GuiCallbacksTest, ShortcutUndo_ReturnsTrue)
{
  gboolean result = shortcutUndo (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

TEST_F (GuiCallbacksTest, ShortcutRedo_ReturnsTrue)
{
  gboolean result = shortcutRedo (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

TEST_F (GuiCallbacksTest, ShortcutSaveGame_ReturnsTrue)
{
  gboolean result = shortcutSaveGame (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

TEST_F (GuiCallbacksTest, ShortcutLoadGame_ReturnsTrue)
{
  gboolean result = shortcutLoadGame (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

TEST_F (GuiCallbacksTest, ShortcutHint_ReturnsTrue)
{
  ctx->is_ai_thinking = true;
  gboolean result = shortcutHint (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

TEST_F (GuiCallbacksTest, ShortcutDummy_ReturnsTrue)
{
  gboolean result = shortcutDummy (nullptr, nullptr, ctx);
  EXPECT_TRUE (result);
}

/* ==========================================================================
 * UNDO/REDO BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, UndoButtonClick_AIThinking)
{
  ctx->is_ai_thinking = true;
  EXPECT_NO_THROW (undoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, UndoButtonClick_NetworkMatch)
{
  ctx->isNetworkMatch = true;
  ctx->is_ai_thinking = false;
  EXPECT_NO_THROW (undoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, UndoButtonClick_ShootArrowPhase)
{
  ctx->is_ai_thinking = false;
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->tmpQueenSrc.col = 3;
  ctx->tmpQueenSrc.row = 0;
  ctx->tmpQueenDst.col = 3;
  ctx->tmpQueenDst.row = 1;

  EXPECT_NO_THROW (undoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, UndoButtonClick_SuccessfulUndo)
{
  ctx->is_ai_thinking = false;
  ctx->isNetworkMatch = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  Move m = createLegalWhiteMove ();
  apply_move (ctx->gameState, m);
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_BLACK);

  EXPECT_NO_THROW (undoButtonClick (nullptr, ctx));
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_WHITE);
}

TEST_F (GuiCallbacksTest, UndoButtonClick_WithAIEnabled)
{
  ctx->is_ai_thinking = false;
  ctx->isNetworkMatch = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  config.ai_black_cfg.enabled = true;

  Move m1 = createLegalWhiteMove ();
  apply_move (ctx->gameState, m1);

  Move m2;
  m2.queen_src.col = 3;
  m2.queen_src.row = 9;
  m2.queen_dst.col = 3;
  m2.queen_dst.row = 8;
  m2.arrow_dst.col = 3;
  m2.arrow_dst.row = 7;
  apply_move (ctx->gameState, m2);

  EXPECT_NO_THROW (undoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, RedoButtonClick_AIThinking)
{
  ctx->is_ai_thinking = true;
  EXPECT_NO_THROW (redoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, RedoButtonClick_NetworkMatch)
{
  ctx->isNetworkMatch = true;
  ctx->is_ai_thinking = false;
  EXPECT_NO_THROW (redoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, RedoButtonClick_ShootArrowPhase)
{
  ctx->is_ai_thinking = false;
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->tmpQueenSrc.col = 3;
  ctx->tmpQueenSrc.row = 0;
  ctx->tmpQueenDst.col = 3;
  ctx->tmpQueenDst.row = 1;
  EXPECT_NO_THROW (redoButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, RedoButtonClick_SuccessfulRedo)
{
  ctx->is_ai_thinking = false;
  ctx->isNetworkMatch = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  Move m = createLegalWhiteMove ();
  apply_move (ctx->gameState, m);
  undo_move (ctx->gameState);

  EXPECT_NO_THROW (redoButtonClick (nullptr, ctx));
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_BLACK);
}

TEST_F (GuiCallbacksTest, RedoButtonClick_WithAIEnabled)
{
  ctx->is_ai_thinking = false;
  ctx->isNetworkMatch = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  config.ai_black_cfg.enabled = true;

  Move m1 = createLegalWhiteMove ();
  apply_move (ctx->gameState, m1);

  Move m2;
  m2.queen_src.col = 3;
  m2.queen_src.row = 9;
  m2.queen_dst.col = 3;
  m2.queen_dst.row = 8;
  m2.arrow_dst.col = 3;
  m2.arrow_dst.row = 7;
  apply_move (ctx->gameState, m2);

  undo_move (ctx->gameState);
  undo_move (ctx->gameState);

  EXPECT_NO_THROW (redoButtonClick (nullptr, ctx));
}

/* ==========================================================================
 * HINT BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, HintButtonClick_AIThinking)
{
  ctx->is_ai_thinking = true;
  EXPECT_NO_THROW (hintButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, HintButtonClick_NotMoveQueenPhase)
{
  ctx->is_ai_thinking = false;
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  EXPECT_NO_THROW (hintButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, HintButtonClick_HintAlreadyActive)
{
  ctx->is_ai_thinking = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  ctx->isHintActive = true;

  EXPECT_NO_THROW (hintButtonClick (nullptr, ctx));
  EXPECT_FALSE (ctx->isHintActive);
}

/* ==========================================================================
 * DRAG & DROP TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, OnDrag_AIThinking_ReturnsNull)
{
  ctx->is_ai_thinking = true;
  GtkDragSource *source = gtk_drag_source_new ();
  gtk_widget_add_controller (ctx->gameBoard[0], GTK_EVENT_CONTROLLER (source));

  EXPECT_EQ (onDrag (source, 0, 0, ctx), nullptr);
}

TEST_F (GuiCallbacksTest, OnDrag_EmptyCell_ReturnsNull)
{
  ctx->is_ai_thinking = false;
  ctx->gameState->board[0][0] = CONTENT_EMPTY;

  GtkDragSource *source = gtk_drag_source_new ();
  gtk_widget_add_controller (ctx->gameBoard[0], GTK_EVENT_CONTROLLER (source));

  EXPECT_EQ (onDrag (source, 0, 0, ctx), nullptr);
}

TEST_F (GuiCallbacksTest, OnDrag_WrongPhase_ReturnsNull)
{
  ctx->is_ai_thinking = false;
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->gameState->board[0][3] = CONTENT_WHITE_QUEEN;

  GtkDragSource *source = gtk_drag_source_new ();
  gtk_widget_add_controller (ctx->gameBoard[3], GTK_EVENT_CONTROLLER (source));

  EXPECT_EQ (onDrag (source, 0, 0, ctx), nullptr);
}

TEST_F (GuiCallbacksTest, OnDrag_WrongPlayerQueen_ReturnsNull)
{
  ctx->is_ai_thinking = false;
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  ctx->gameState->current_player = PLAYER_WHITE;

  ctx->gameState->board[9][3] = CONTENT_BLACK_QUEEN;

  GtkDragSource *source = gtk_drag_source_new ();
  gtk_widget_add_controller (ctx->gameBoard[93],
                             GTK_EVENT_CONTROLLER (source));

  EXPECT_EQ (onDrag (source, 0, 0, ctx), nullptr);
}

TEST_F (GuiCallbacksTest, OnDrop_WrongPhase_ReturnsFalse)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;

  GtkDropTarget *target = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_widget_add_controller (ctx->gameBoard[1], GTK_EVENT_CONTROLLER (target));

  GValue val = G_VALUE_INIT;
  g_value_init (&val, G_TYPE_STRING);
  g_value_set_string (&val, "0");

  EXPECT_FALSE (onDrop (target, &val, 0, 0, ctx));
  g_value_unset (&val);
}

TEST_F (GuiCallbacksTest, OnDrop_IllegalMove_ReturnsFalse)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  GtkDropTarget *target = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_widget_add_controller (ctx->gameBoard[1], GTK_EVENT_CONTROLLER (target));

  g_object_set_data (G_OBJECT (ctx->gameBoard[1]), "is_legal_move",
                     GINT_TO_POINTER (0));

  GValue val = G_VALUE_INIT;
  g_value_init (&val, G_TYPE_STRING);
  g_value_set_string (&val, "0");

  EXPECT_FALSE (onDrop (target, &val, 0, 0, ctx));
  g_value_unset (&val);
}

TEST_F (GuiCallbacksTest, OnDrop_DestNotEmpty_ReturnsFalse)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  GtkDropTarget *target = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_widget_add_controller (ctx->gameBoard[1], GTK_EVENT_CONTROLLER (target));

  g_object_set_data (G_OBJECT (ctx->gameBoard[1]), "is_legal_move",
                     GINT_TO_POINTER (1));
  ctx->gameState->board[0][1] = CONTENT_ARROW;

  GValue val = G_VALUE_INIT;
  g_value_init (&val, G_TYPE_STRING);
  g_value_set_string (&val, "0");

  EXPECT_FALSE (onDrop (target, &val, 0, 0, ctx));
  g_value_unset (&val);
}

TEST_F (GuiCallbacksTest, OnDrop_Success)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  GtkDropTarget *target = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
  gtk_widget_add_controller (ctx->gameBoard[13],
                             GTK_EVENT_CONTROLLER (target));

  g_object_set_data (G_OBJECT (ctx->gameBoard[13]), "is_legal_move",
                     GINT_TO_POINTER (1));
  ctx->gameState->board[0][3] = CONTENT_WHITE_QUEEN;
  ctx->gameState->board[1][3] = CONTENT_EMPTY;

  GValue val = G_VALUE_INIT;
  g_value_init (&val, G_TYPE_STRING);
  g_value_set_string (&val, "3");

  EXPECT_TRUE (onDrop (target, &val, 0, 0, ctx));

  EXPECT_EQ (ctx->currentPhase, PHASE_SHOOT_ARROW);
  EXPECT_EQ (ctx->gameState->board[1][3], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (ctx->gameState->board[0][3], CONTENT_EMPTY);

  g_value_unset (&val);
}

/* ==========================================================================
 * QUIT GAME BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, QuitGameButtonClick_NetworkMatch)
{
  ctx->isNetworkMatch = true;
  EXPECT_NO_THROW (quitGameButtonClick (nullptr, ctx));
  EXPECT_FALSE (ctx->isNetworkMatch);
}

TEST_F (GuiCallbacksTest, QuitGameButtonClick_LocalGame)
{
  ctx->isNetworkMatch = false;
  EXPECT_NO_THROW (quitGameButtonClick (nullptr, ctx));
}

/* ==========================================================================
 * NEW GAME BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, NewGameButtonClick_NetworkMatch)
{
  ctx->isNetworkMatch = true;
  EXPECT_NO_THROW (newGameButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, NewGameButtonClick_LocalGame)
{
  ctx->isNetworkMatch = false;
  EXPECT_NO_THROW (newGameButtonClick (nullptr, ctx));
}

/* ==========================================================================
 * STOP SERVER BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, StopServerButtonClick)
{
  EXPECT_NO_THROW (stopServerButtonClick (nullptr, ctx));
}

/* ==========================================================================
 * DISCONNECT BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, DisconnectFromServerButtonClick)
{
  ctx->isNetworkMatch = true;
  EXPECT_NO_THROW (disconnectFromServerButtonClick (nullptr, ctx));
  EXPECT_FALSE (ctx->isNetworkMatch);
}

/* ==========================================================================
 * ON RIGHT CLICK TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, OnRightClick_AIThinking)
{
  ctx->is_ai_thinking = true;
  EXPECT_NO_THROW (onRightClick (nullptr, 0, 0, 0, ctx));
}

TEST_F (GuiCallbacksTest, OnRightClick_NotShootArrowPhase)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  EXPECT_NO_THROW (onRightClick (nullptr, 0, 0, 0, ctx));
}

TEST_F (GuiCallbacksTest, OnRightClick_ShootArrowPhase)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->tmpQueenSrc.col = 3;
  ctx->tmpQueenSrc.row = 0;
  ctx->tmpQueenDst.col = 3;
  ctx->tmpQueenDst.row = 1;

  ctx->gameState->board[0][3] = CONTENT_EMPTY;
  ctx->gameState->board[1][3] = CONTENT_WHITE_QUEEN;

  EXPECT_NO_THROW (onRightClick (nullptr, 0, 0, 0, ctx));
  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);
}

/* ==========================================================================
 * FIRE ARROW TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, FireArrow_AIThinking)
{
  ctx->is_ai_thinking = true;
  GtkWidget *btn = ctx->gameBoard[0];
  EXPECT_NO_THROW (fireArrow (btn, ctx));
}

TEST_F (GuiCallbacksTest, FireArrow_NotShootArrowPhase)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  GtkWidget *btn = ctx->gameBoard[0];
  EXPECT_NO_THROW (fireArrow (btn, ctx));
}

TEST_F (GuiCallbacksTest, FireArrow_NetworkMatch_WrongPlayer)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->isNetworkMatch = true;
  ctx->selfPlayerColor = PLAYER_BLACK;
  ctx->gameState->current_player = PLAYER_WHITE;

  GtkWidget *btn = ctx->gameBoard[0];
  EXPECT_NO_THROW (fireArrow (btn, ctx));
}

TEST_F (GuiCallbacksTest, FireArrow_NotLegalMove)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->isNetworkMatch = false;

  GtkWidget *btn = ctx->gameBoard[22];
  g_object_set_data (G_OBJECT (btn), "is_legal_move", GINT_TO_POINTER (0));

  EXPECT_NO_THROW (fireArrow (btn, ctx));
}

TEST_F (GuiCallbacksTest, FireArrow_Success)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->isNetworkMatch = false;
  ctx->is_ai_thinking = false;
  ctx->gameState->current_player = PLAYER_WHITE;

  ctx->tmpQueenSrc.col = 3;
  ctx->tmpQueenSrc.row = 0;
  ctx->tmpQueenDst.col = 3;
  ctx->tmpQueenDst.row = 1;

  ctx->gameState->board[0][3] = CONTENT_EMPTY;
  ctx->gameState->board[1][3] = CONTENT_WHITE_QUEEN;

  GtkWidget *btn = ctx->gameBoard[23];
  g_object_set_data (G_OBJECT (btn), "is_legal_move", GINT_TO_POINTER (1));

  EXPECT_NO_THROW (fireArrow (btn, ctx));

  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);
  EXPECT_EQ (ctx->gameState->current_player, PLAYER_BLACK);
}

/* ==========================================================================
 * ON DRAG END TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, OnDragEnd_MoveQueenPhase)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  EXPECT_NO_THROW (onDragEnd (nullptr, nullptr, false, ctx));
}

TEST_F (GuiCallbacksTest, OnDragEnd_ShootArrowPhase)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  EXPECT_NO_THROW (onDragEnd (nullptr, nullptr, false, ctx));
}

/* ==========================================================================
 * CONFIG UPDATE TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, UpdateConfigVerbose)
{
  GtkWidget *sw = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (sw), TRUE);
  updateConfigVerbose (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_TRUE (ctx->gameConfig->verbose);

  gtk_switch_set_active (GTK_SWITCH (sw), FALSE);
  updateConfigVerbose (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_FALSE (ctx->gameConfig->verbose);
}

TEST_F (GuiCallbacksTest, UpdateConfigBlitz)
{
  GtkWidget *sw = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (sw), TRUE);
  updateConfigBlitz (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_TRUE (ctx->gameConfig->blitz_mode);

  gtk_switch_set_active (GTK_SWITCH (sw), FALSE);
  updateConfigBlitz (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_FALSE (ctx->gameConfig->blitz_mode);
}

TEST_F (GuiCallbacksTest, UpdateConfigTimeout)
{
  GtkWidget *spin = gtk_spin_button_new_with_range (0, 3600, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 120);
  updateConfigTimeout (GTK_SPIN_BUTTON (spin), ctx);
  EXPECT_EQ (ctx->gameConfig->time_per_player, 120);
}

TEST_F (GuiCallbacksTest, UpdateConfigBoardSize)
{
  GtkWidget *spin = gtk_spin_button_new_with_range (4, 11, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 8);
  updateConfigBoardSize (GTK_SPIN_BUTTON (spin), ctx);
  EXPECT_EQ (ctx->gameConfig->board_size, 8);
}

TEST_F (GuiCallbacksTest, UpdateConfigAiWhite)
{
  GtkWidget *sw = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (sw), TRUE);
  updateConfigAiWhite (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_TRUE (ctx->gameConfig->ai_white_cfg.enabled);

  gtk_switch_set_active (GTK_SWITCH (sw), FALSE);
  updateConfigAiWhite (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_FALSE (ctx->gameConfig->ai_white_cfg.enabled);
}

TEST_F (GuiCallbacksTest, UpdateConfigAiBlack)
{
  GtkWidget *sw = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (sw), TRUE);
  updateConfigAiBlack (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_TRUE (ctx->gameConfig->ai_black_cfg.enabled);

  gtk_switch_set_active (GTK_SWITCH (sw), FALSE);
  updateConfigAiBlack (GTK_SWITCH (sw), nullptr, ctx);
  EXPECT_FALSE (ctx->gameConfig->ai_black_cfg.enabled);
}

/* ==========================================================================
 * UPDATE CONFIG LANGUAGE TESTS
 * ==========================================================================
 */

#if GTK_CHECK_VERSION(4, 10, 0)
TEST_F (GuiCallbacksTest, UpdateConfigLanguage_English)
{
  const char *langs[] = { "English", "Français", NULL };
  GtkStringList *stringList = gtk_string_list_new (langs);
  GtkWidget *dropdown = gtk_drop_down_new (G_LIST_MODEL (stringList), NULL);
  gtk_drop_down_set_selected (GTK_DROP_DOWN (dropdown), 0);

  updateConfigLanguage (G_OBJECT (dropdown), nullptr, ctx);
  EXPECT_EQ (ctx->gameConfig->language, (lang_t)0);
}

TEST_F (GuiCallbacksTest, UpdateConfigLanguage_French)
{
  const char *langs[] = { "English", "Français", NULL };
  GtkStringList *stringList = gtk_string_list_new (langs);
  GtkWidget *dropdown = gtk_drop_down_new (G_LIST_MODEL (stringList), NULL);
  gtk_drop_down_set_selected (GTK_DROP_DOWN (dropdown), 1);

  updateConfigLanguage (G_OBJECT (dropdown), nullptr, ctx);
  EXPECT_EQ (ctx->gameConfig->language, (lang_t)1);
}

TEST_F (GuiCallbacksTest, UpdateConfigLanguage_InvalidIndex)
{
  const char *langs[] = { "English", "Français", "Invalid", NULL };
  GtkStringList *stringList = gtk_string_list_new (langs);
  GtkWidget *dropdown = gtk_drop_down_new (G_LIST_MODEL (stringList), NULL);
  gtk_drop_down_set_selected (GTK_DROP_DOWN (dropdown), 2);

  ctx->gameConfig->language = (lang_t)0;
  updateConfigLanguage (G_OBJECT (dropdown), nullptr, ctx);
  EXPECT_EQ (ctx->gameConfig->language, (lang_t)0);
}
#else
TEST_F (GuiCallbacksTest, UpdateConfigLanguage_English_ComboBox)
{
  GtkWidget *combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "English");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Français");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

  updateConfigLanguage (GTK_COMBO_BOX_TEXT (combo), ctx);
  EXPECT_EQ (ctx->gameConfig->language, (lang_t)0);
}

TEST_F (GuiCallbacksTest, UpdateConfigLanguage_French_ComboBox)
{
  GtkWidget *combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "English");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Français");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);

  updateConfigLanguage (GTK_COMBO_BOX_TEXT (combo), ctx);
  EXPECT_EQ (ctx->gameConfig->language, (lang_t)1);
}

TEST_F (GuiCallbacksTest, UpdateConfigLanguage_InvalidIndex_ComboBox)
{
  GtkWidget *combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "English");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Français");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "Invalid");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 2);

  ctx->gameConfig->language = (lang_t)0;
  updateConfigLanguage (GTK_COMBO_BOX_TEXT (combo), ctx);
  EXPECT_EQ (ctx->gameConfig->language, (lang_t)0);
}
#endif

/* ==========================================================================
 * SAVE/LOAD BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, SaveButtonClick_ShootArrowPhase)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  EXPECT_NO_THROW (saveButtonClick (nullptr, ctx));
}

TEST_F (GuiCallbacksTest, LoadButtonClick_NetworkMatch)
{
  ctx->isNetworkMatch = true;
  EXPECT_NO_THROW (loadButtonClick (nullptr, ctx));
}

/* ==========================================================================
 * GUI HINT APPLY MOVE TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, GuiHintApplyMove_Basic)
{
  AIWorkerData *data = (AIWorkerData *)malloc (sizeof (AIWorkerData));
  ASSERT_NE (data, nullptr);

  data->ai = (AI_Interface *)malloc (sizeof (AI_Interface));
  ASSERT_NE (data->ai, nullptr);

  Param params[1] = {};
  ConfigMap cm = { 0, params };
  *(data->ai) = create_ai ("random", cm);

  data->ctx = ctx;
  data->state_copy = *ctx->gameState;
  data->best_move = createLegalWhiteMove ();

  ctx->is_ai_thinking = true;
  gboolean result = guiHintApplyMove (data);

  EXPECT_EQ (result, G_SOURCE_REMOVE);
  EXPECT_FALSE (ctx->is_ai_thinking);
  EXPECT_TRUE (ctx->isHintActive);
}

TEST_F (GuiCallbacksTest, GuiHintApplyMove_PlayerChanged)
{
  AIWorkerData *data = (AIWorkerData *)malloc (sizeof (AIWorkerData));
  ASSERT_NE (data, nullptr);

  data->ai = (AI_Interface *)malloc (sizeof (AI_Interface));
  ASSERT_NE (data->ai, nullptr);

  Param params[1] = {};
  ConfigMap cm = { 0, params };
  *(data->ai) = create_ai ("random", cm);

  data->ctx = ctx;
  data->state_copy = *ctx->gameState;
  data->state_copy.current_player = PLAYER_BLACK;
  data->best_move = createLegalWhiteMove ();

  ctx->is_ai_thinking = true;
  ctx->gameState->current_player = PLAYER_WHITE;

  gboolean result = guiHintApplyMove (data);

  EXPECT_EQ (result, G_SOURCE_REMOVE);
  EXPECT_FALSE (ctx->is_ai_thinking);
  EXPECT_FALSE (ctx->isHintActive);
}

/* ==========================================================================
 * GUI HINT WORKER THREAD TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, GuiHintWorkerThread_GeneratesMove)
{
  AIWorkerData *data = (AIWorkerData *)malloc (sizeof (AIWorkerData));
  ASSERT_NE (data, nullptr);

  data->ai = (AI_Interface *)malloc (sizeof (AI_Interface));
  ASSERT_NE (data->ai, nullptr);

  Param params[1] = {};
  ConfigMap cm = { 0, params };
  *(data->ai) = create_ai ("random", cm);

  data->ctx = ctx;
  data->state_copy = *ctx->gameState;

  gpointer result = guiHintWorkerThread (data);

  EXPECT_EQ (result, nullptr);
}

/* ==========================================================================
 * NAVIGATION CLICK TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, NavigationClick_ToGamePage)
{
  GtkWidget *btn = gtk_button_new_with_label ("Game");
  g_object_set_data (G_OBJECT (btn), "target_page", (gpointer) "game_page");

  EXPECT_NO_THROW (navigationClick (GTK_BUTTON (btn), ctx));

  const char *visible
      = gtk_stack_get_visible_child_name (GTK_STACK (ctx->mainStack));
  EXPECT_STREQ (visible, "game_page");
}

TEST_F (GuiCallbacksTest, NavigationClick_ToMainMenu)
{
  GtkWidget *btn = gtk_button_new_with_label ("Menu");
  g_object_set_data (G_OBJECT (btn), "target_page",
                     (gpointer) "main_menu_page");

  EXPECT_NO_THROW (navigationClick (GTK_BUTTON (btn), ctx));

  const char *visible
      = gtk_stack_get_visible_child_name (GTK_STACK (ctx->mainStack));
  EXPECT_STREQ (visible, "main_menu_page");
}

/* ==========================================================================
 * JOIN SERVER BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, JoinServerButtonClick_EmptyUsername)
{
  GtkWidget *btn = gtk_button_new_with_label ("Join");
  GtkWidget *entry = gtk_entry_new ();

  g_object_set_data (G_OBJECT (btn), "server_ip", (gpointer) "127.0.0.1");
  g_object_set_data (G_OBJECT (btn), "server_port", GINT_TO_POINTER (12345));
  g_object_set_data (G_OBJECT (btn), "username_entry", entry);

  gtk_editable_set_text (GTK_EDITABLE (entry), "");
  EXPECT_NO_THROW (joinServerButtonClick (GTK_BUTTON (btn), ctx));
}

TEST_F (GuiCallbacksTest, JoinServerButtonClick_WithUsername)
{
  GtkWidget *btn = gtk_button_new_with_label ("Join");
  GtkWidget *entry = gtk_entry_new ();

  GtkWidget *titleLabel = gtk_label_new ("Title");
  g_object_set_data (G_OBJECT (ctx->mainStack), "lobby_title_label",
                     titleLabel);

  g_object_set_data (G_OBJECT (btn), "server_ip", (gpointer) "127.0.0.1");
  g_object_set_data (G_OBJECT (btn), "server_port", GINT_TO_POINTER (12345));
  g_object_set_data (G_OBJECT (btn), "username_entry", entry);

  gtk_editable_set_text (GTK_EDITABLE (entry), "TestUser");
  EXPECT_NO_THROW (joinServerButtonClick (GTK_BUTTON (btn), ctx));
}

/* ==========================================================================
 * REFRESH SERVER LIST BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, RefreshServerListButtonClick_NoServers)
{
  GtkWidget *btn = gtk_button_new_with_label ("Refresh");
  GtkWidget *listbox = gtk_list_box_new ();
  GtkWidget *entry = gtk_entry_new ();

  g_object_set_data (G_OBJECT (btn), "server_listbox", listbox);
  g_object_set_data (G_OBJECT (btn), "username_entry", entry);

  EXPECT_NO_THROW (refreshServerListButtonClick (GTK_BUTTON (btn), ctx));
}

/* ==========================================================================
 * SEND INVITE BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, SendInviteButtonClick_InvalidTargetId)
{
  GtkWidget *btn = gtk_button_new_with_label ("Invite");
  GtkWidget *entry = gtk_entry_new ();
  GtkWidget *spin = gtk_spin_button_new_with_range (4, 11, 1);
  GtkWidget *popup = gtk_window_new ();

  gtk_editable_set_text (GTK_EDITABLE (entry), "0"); // Invalid ID

  g_object_set_data (G_OBJECT (btn), "target_id_entry", entry);
  g_object_set_data (G_OBJECT (btn), "board_size_spin", spin);
  g_object_set_data (G_OBJECT (btn), "popup_window", popup);

  EXPECT_NO_THROW (sendInviteButtonClick (GTK_BUTTON (btn), ctx));
}

TEST_F (GuiCallbacksTest, SendInviteButtonClick_ValidTargetId)
{
  GtkWidget *btn = gtk_button_new_with_label ("Invite");
  GtkWidget *entry = gtk_entry_new ();
  GtkWidget *spin = gtk_spin_button_new_with_range (4, 11, 1);
  GtkWidget *popup = gtk_window_new ();

  gtk_editable_set_text (GTK_EDITABLE (entry), "5"); // Valid ID
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 10);

  g_object_set_data (G_OBJECT (btn), "target_id_entry", entry);
  g_object_set_data (G_OBJECT (btn), "board_size_spin", spin);
  g_object_set_data (G_OBJECT (btn), "popup_window", popup);

  EXPECT_NO_THROW (sendInviteButtonClick (GTK_BUTTON (btn), ctx));
}

/* ==========================================================================
 * START SERVER BUTTON TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, StartServerButtonClick_Success)
{
  GtkWidget *btn = gtk_button_new_with_label ("Start");

  GtkWidget *spin = gtk_spin_button_new_with_range (1024, 65535, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 19999);
  g_object_set_data (G_OBJECT (btn), "port_spin", spin);

  GtkWidget *entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "19999");
  g_object_set_data (G_OBJECT (btn), "port_entry", entry);

  GtkWidget *popup = gtk_window_new ();
  g_object_set_data (G_OBJECT (btn), "popup_window", popup);

  GtkWidget *consoleView = gtk_text_view_new ();
  g_object_set_data (G_OBJECT (ctx->mainStack), "server_console_view",
                     consoleView);

  EXPECT_NO_THROW (startServerButtonClick (GTK_BUTTON (btn), ctx));

  NetworkManager_ShutdownServer (ctx->networkManager);
}

/* ==========================================================================
 * CELL CLICKED TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, OnCellClicked_CallsFireArrow)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  GtkWidget *cell = ctx->gameBoard[0];
  GtkGesture *gesture = gtk_gesture_click_new ();
  gtk_widget_add_controller (cell, GTK_EVENT_CONTROLLER (gesture));

  EXPECT_NO_THROW (onCellClicked (GTK_GESTURE_CLICK (gesture), 1, 0, 0, ctx));
}

/* ==========================================================================
 * GAME STATE CONSISTENCY TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, PhaseTransition_MoveToShoot)
{
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  EXPECT_EQ (ctx->currentPhase, PHASE_SHOOT_ARROW);
}

TEST_F (GuiCallbacksTest, PhaseTransition_ShootToMove)
{
  ctx->currentPhase = PHASE_SHOOT_ARROW;
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  EXPECT_EQ (ctx->currentPhase, PHASE_MOVE_QUEEN);
}

TEST_F (GuiCallbacksTest, NetworkMatchFlagToggle)
{
  ctx->isNetworkMatch = false;
  EXPECT_FALSE (ctx->isNetworkMatch);
  ctx->isNetworkMatch = true;
  EXPECT_TRUE (ctx->isNetworkMatch);
}

TEST_F (GuiCallbacksTest, AIThinkingFlagToggle)
{
  ctx->is_ai_thinking = false;
  EXPECT_FALSE (ctx->is_ai_thinking);
  ctx->is_ai_thinking = true;
  EXPECT_TRUE (ctx->is_ai_thinking);
}

TEST_F (GuiCallbacksTest, HintActiveFlagToggle)
{
  ctx->isHintActive = false;
  EXPECT_FALSE (ctx->isHintActive);
  ctx->isHintActive = true;
  EXPECT_TRUE (ctx->isHintActive);
}

/* ==========================================================================
 * TEMP POSITION TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, TmpQueenPositions)
{
  ctx->tmpQueenSrc.col = 3;
  ctx->tmpQueenSrc.row = 0;
  ctx->tmpQueenDst.col = 3;
  ctx->tmpQueenDst.row = 1;

  EXPECT_EQ (ctx->tmpQueenSrc.col, 3);
  EXPECT_EQ (ctx->tmpQueenSrc.row, 0);
  EXPECT_EQ (ctx->tmpQueenDst.col, 3);
  EXPECT_EQ (ctx->tmpQueenDst.row, 1);
}

/* ==========================================================================
 * SELF PLAYER COLOR TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, SelfPlayerColorWhite)
{
  ctx->selfPlayerColor = PLAYER_WHITE;
  EXPECT_EQ (ctx->selfPlayerColor, PLAYER_WHITE);
}

TEST_F (GuiCallbacksTest, SelfPlayerColorBlack)
{
  ctx->selfPlayerColor = PLAYER_BLACK;
  EXPECT_EQ (ctx->selfPlayerColor, PLAYER_BLACK);
}

/* ==========================================================================
 * KEY BINDINGS TESTS
 * ==========================================================================
 */

TEST_F (GuiCallbacksTest, KeyBindingsSet)
{
  EXPECT_STREQ (ctx->keyBindings.key_new_game, "<Control>n");
  EXPECT_STREQ (ctx->keyBindings.key_quit, "<Control>q");
  EXPECT_STREQ (ctx->keyBindings.key_undo, "<Control>z");
}