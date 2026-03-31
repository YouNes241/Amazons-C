#include "gui_callbacks.h"
#include "game_engine.h"
#include "gui_controller.h"
#include <save.h>
#include <stdio.h>
#include <stdlib.h>

void
navigationClick (GtkButton *button, gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  const char *targetPage
      = (const char *)g_object_get_data (G_OBJECT (button), "target_page");

  if (ctx->mainStack != NULL && targetPage != NULL)
    {
      gtk_stack_set_visible_child_name (ctx->mainStack, targetPage);
    }
}

void
quitButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  if (ctx && ctx->mainWindow)
    {
      gtk_window_close (GTK_WINDOW (ctx->mainWindow));
    }
}

void
infoPopupButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/popups/info_popup.ui");
  GtkWindow *infoWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "info_window"));

  gtk_window_set_transient_for (infoWindow, GTK_WINDOW (ctx->mainWindow));
  gtk_window_present (infoWindow);

  g_object_unref (builder);
}

void
configPopupButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/popups/config_popup.ui");
  GtkWindow *configWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "config_window"));

  gtk_window_set_transient_for (configWindow, GTK_WINDOW (ctx->mainWindow));

  GtkWidget *verboseSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "verbose_switch"));
  GtkWidget *blitzSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "blitz_switch"));
  GtkWidget *timeoutField
      = GTK_WIDGET (gtk_builder_get_object (builder, "timeout_field"));
  GtkWidget *boardSizeField
      = GTK_WIDGET (gtk_builder_get_object (builder, "board_size_field"));
  GtkWidget *aiWhiteSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "ai_white_switch"));
  GtkWidget *aiBlackSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "ai_black_switch"));

  GtkWidget *langDropdownContainer = GTK_WIDGET (
      gtk_builder_get_object (builder, "lang_dropdown_container"));
  GtkWidget *langDropdown;

  GtkWidget *bindNewGameBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_new_game_btn"));
  GtkWidget *bindLoadBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_load_game_btn"));
  GtkWidget *bindSaveBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_save_game_btn"));
  GtkWidget *bindUndoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_undo_btn"));
  GtkWidget *bindRedoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_redo_btn"));
  GtkWidget *bindPauseBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_pause_btn"));
  GtkWidget *bindHintBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_hint_btn"));
  GtkWidget *bindConfigBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_config_btn"));
  GtkWidget *bindInfoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_info_btn"));
  GtkWidget *bindQuitBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_quit_btn"));

  if (verboseSwitch)
    gtk_switch_set_active (GTK_SWITCH (verboseSwitch),
                           ctx->gameConfig->verbose);
  if (blitzSwitch)
    gtk_switch_set_active (GTK_SWITCH (blitzSwitch),
                           ctx->gameConfig->blitz_mode);
  if (timeoutField)
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (timeoutField),
                               ctx->gameConfig->time_per_player);
  if (boardSizeField)
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (boardSizeField),
                               ctx->gameConfig->board_size);
  if (aiWhiteSwitch)
    gtk_switch_set_active (GTK_SWITCH (aiWhiteSwitch),
                           ctx->gameConfig->ai_white);
  if (aiBlackSwitch)
    gtk_switch_set_active (GTK_SWITCH (aiBlackSwitch),
                           ctx->gameConfig->ai_black);

  if (verboseSwitch)
    g_signal_connect (verboseSwitch, "notify::active",
                      G_CALLBACK (updateConfigVerbose), ctx);
  if (blitzSwitch)
    g_signal_connect (blitzSwitch, "notify::active",
                      G_CALLBACK (updateConfigBlitz), ctx);
  if (timeoutField)
    g_signal_connect (timeoutField, "value-changed",
                      G_CALLBACK (updateConfigTimeout), ctx);
  if (boardSizeField)
    g_signal_connect (boardSizeField, "value-changed",
                      G_CALLBACK (updateConfigBoardSize), ctx);
  if (aiWhiteSwitch)
    g_signal_connect (aiWhiteSwitch, "notify::active",
                      G_CALLBACK (updateConfigAiWhite), ctx);
  if (aiBlackSwitch)
    g_signal_connect (aiBlackSwitch, "notify::active",
                      G_CALLBACK (updateConfigAiBlack), ctx);

#if GTK_CHECK_VERSION(4, 10, 0)
  const char *langs[] = { "English", "Français", NULL };
  GtkStringList *stringList = gtk_string_list_new (langs);
  langDropdown = gtk_drop_down_new (G_LIST_MODEL (stringList), NULL);
  gtk_drop_down_set_selected (GTK_DROP_DOWN (langDropdown),
                              ctx->gameConfig->language);

  g_signal_connect (langDropdown, "notify::selected",
                    G_CALLBACK (updateConfigLanguage), ctx);
#else
  langDropdown = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (langDropdown),
                                  "English");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (langDropdown),
                                  "Français");
  gtk_combo_box_set_active (GTK_COMBO_BOX (langDropdown),
                            ctx->gameConfig->language);

  g_signal_connect (langDropdown, "changed", G_CALLBACK (updateConfigLanguage),
                    ctx);
#endif

  if (langDropdownContainer)
    {
      gtk_box_append (GTK_BOX (langDropdownContainer), langDropdown);
    }

  setupBindingButton (bindNewGameBtn, ctx->keyBindings.key_new_game, ctx);
  setupBindingButton (bindLoadBtn, ctx->keyBindings.key_load_game, ctx);
  setupBindingButton (bindSaveBtn, ctx->keyBindings.key_save_game, ctx);
  setupBindingButton (bindUndoBtn, ctx->keyBindings.key_undo, ctx);
  setupBindingButton (bindRedoBtn, ctx->keyBindings.key_redo, ctx);
  setupBindingButton (bindPauseBtn, ctx->keyBindings.key_pause, ctx);
  setupBindingButton (bindHintBtn, ctx->keyBindings.key_hint, ctx);
  setupBindingButton (bindConfigBtn, ctx->keyBindings.key_config, ctx);
  setupBindingButton (bindInfoBtn, ctx->keyBindings.key_info, ctx);
  setupBindingButton (bindQuitBtn, ctx->keyBindings.key_quit, ctx);

  gtk_window_present (configWindow);

  g_object_unref (builder);
}

void
undoButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  if (!ctx || !ctx->gameState)
    return;

  bool has_ai = ctx->gameConfig->ai_white || ctx->gameConfig->ai_black;
  if (!has_ai)
    return;

  if (undo_move (ctx->gameState))
    {
      undo_move (ctx->gameState);

      ctx->currentPhase = PHASE_MOVE_QUEEN;
      clearHighlights (ctx);
      drawBoard (ctx);
      updateTurnDisplay (ctx);
    }
}

void
redoButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  if (!ctx || !ctx->gameState)
    return;

  bool has_ai = ctx->gameConfig->ai_white || ctx->gameConfig->ai_black;
  if (!has_ai)
    return;

  if (redo_move (ctx->gameState))
    {
      redo_move (ctx->gameState);

      ctx->currentPhase = PHASE_MOVE_QUEEN;
      clearHighlights (ctx);
      drawBoard (ctx);
      updateTurnDisplay (ctx);
    }
}

void
connectToServerButtonClick (GtkButton *button, gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;

  GtkEditable *ip_entry
      = GTK_EDITABLE (g_object_get_data (G_OBJECT (button), "ip_entry"));
  GtkEditable *port_entry
      = GTK_EDITABLE (g_object_get_data (G_OBJECT (button), "port_entry"));
  GtkEditable *username_entry
      = GTK_EDITABLE (g_object_get_data (G_OBJECT (button), "username_entry"));

  const char *ip_str = gtk_editable_get_text (ip_entry);
  const char *port_str = gtk_editable_get_text (port_entry);
  const char *username_str = gtk_editable_get_text (username_entry);

  int port = atoi (port_str);

  if (NetworkManager_ClientJoin (ctx->networkManager, ip_str, port) == 0)
    {
      NetworkManager_SendJoin (ctx->networkManager, username_str);

      if (ctx->mainStack != NULL)
        {
          GtkWidget *titleLabel = GTK_WIDGET (g_object_get_data (
              G_OBJECT (ctx->mainStack), "lobby_title_label"));
          if (titleLabel)
            {
              char *titleText = g_strdup_printf ("%s", username_str);
              gtk_label_set_text (GTK_LABEL (titleLabel), titleText);
            }
          gtk_stack_set_visible_child_name (ctx->mainStack,
                                            "client_panel_page");
        }
    }
}

void
disconnectFromServerButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  guiDisconnectAndRestartNetwork (ctx);

  if (ctx->mainStack != NULL)
    {
      gtk_stack_set_visible_child_name (ctx->mainStack, "multiplayer_page");
    }
}

void
createGameButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  GtkBuilder *builder = gtk_builder_new_from_resource (
      "/amazons/ui/popups/create_game_popup.ui");
  GtkWindow *createGameWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "create_game_window"));

  gtk_window_set_transient_for (createGameWindow,
                                GTK_WINDOW (ctx->mainWindow));

  GtkWidget *targetIdEntry
      = GTK_WIDGET (gtk_builder_get_object (builder, "target_id_entry"));
  GtkWidget *boardSizeSpin
      = GTK_WIDGET (gtk_builder_get_object (builder, "board_size_spin"));
  GtkWidget *cancelBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "cancel_btn"));
  GtkWidget *inviteBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "invite_btn"));

  g_object_set_data (G_OBJECT (inviteBtn), "target_id_entry", targetIdEntry);
  g_object_set_data (G_OBJECT (inviteBtn), "board_size_spin", boardSizeSpin);
  g_object_set_data (G_OBJECT (inviteBtn), "popup_window", createGameWindow);

  g_signal_connect_swapped (cancelBtn, "clicked",
                            G_CALLBACK (gtk_window_destroy), createGameWindow);
  g_signal_connect (inviteBtn, "clicked", G_CALLBACK (sendInviteButtonClick),
                    ctx);

  gtk_window_present (createGameWindow);
  g_object_unref (builder);
}

void
sendInviteButtonClick (GtkButton *button, gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;

  GtkEditable *target_id_entry = GTK_EDITABLE (
      g_object_get_data (G_OBJECT (button), "target_id_entry"));
  GtkSpinButton *board_size_spin = GTK_SPIN_BUTTON (
      g_object_get_data (G_OBJECT (button), "board_size_spin"));
  GtkWindow *popup_window
      = GTK_WINDOW (g_object_get_data (G_OBJECT (button), "popup_window"));

  const char *id_str = gtk_editable_get_text (target_id_entry);
  int target_id = atoi (id_str);
  int board_size = gtk_spin_button_get_value_as_int (board_size_spin);

  if (target_id <= 0)
    {
      return;
    }

  if (ctx->networkManager)
    {
      NetworkManager_SendInvite (ctx->networkManager, target_id, board_size);
    }

  gtk_window_destroy (popup_window);
}

void
newGameButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  GtkWidget *boardGrid = GTK_WIDGET (
      g_object_get_data (G_OBJECT (ctx->mainStack), "board_grid"));

  if (boardGrid)
    {
      setupBoard (ctx, boardGrid);
    }

  guiStartNewGame (ctx);
  gtk_stack_set_visible_child_name (ctx->mainStack, "game_page");
}

GdkContentProvider *
onDrag (GtkDragSource *source, double x, double y, gpointer user_data)
{
  (void)x;
  (void)y;

  GuiContext *ctx = (GuiContext *)user_data;

  if (ctx->currentPhase != PHASE_MOVE_QUEEN
      || (ctx->isNetworkMatch
          && ctx->gameState->current_player != ctx->selfPlayerColor))
    {
      return NULL;
    }

  GtkWidget *btn
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (source));

  int srcX = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_x"));
  int srcY = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_y"));
  int srcIndex
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_index"));

  CellContent piece = ctx->gameState->board[srcY][srcX];

  if (piece == CONTENT_EMPTY || piece == CONTENT_ARROW)
    {
      return NULL;
    }

  PlayerColor currentPlayer = ctx->gameState->current_player;
  if ((currentPlayer == PLAYER_WHITE && piece != CONTENT_WHITE_QUEEN)
      || (currentPlayer == PLAYER_BLACK && piece != CONTENT_BLACK_QUEEN))
    {
      return NULL;
    }

  highlightReachableSquares (ctx, srcX, srcY);

  int foundCells = 0;
  for (int i = 0; i < ctx->gameState->size * ctx->gameState->size; i++)
    {
      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (ctx->gameBoard[i]),
                                              "is_legal_move"))
          == 1)
        {
          foundCells++;
        }
    }

  char strIndex[8];
  snprintf (strIndex, sizeof (strIndex), "%d", srcIndex);

  GValue value = G_VALUE_INIT;
  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, strIndex);

  GdkContentProvider *provider = gdk_content_provider_new_for_value (&value);
  g_value_unset (&value);

  return provider;
}

gboolean
onDrop (GtkDropTarget *target, const GValue *value, double x, double y,
        gpointer user_data)
{
  (void)x;
  (void)y;

  GuiContext *ctx = (GuiContext *)user_data;

  if (ctx->currentPhase != PHASE_MOVE_QUEEN)
    {
      return FALSE;
    }

  GtkWidget *btn
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));

  int destX = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_x"));
  int destY = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_y"));

  int isLegal
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "is_legal_move"));
  if (!isLegal)
    {
      return FALSE;
    }

  const char *srcString = g_value_get_string (value);
  int srcIndex = atoi (srcString);
  int size = ctx->gameState->size;

  int srcX = srcIndex % size;
  int srcY = srcIndex / size;

  if (ctx->gameState->board[destY][destX] != CONTENT_EMPTY)
    {
      return FALSE;
    }

  ctx->tmpQueenSrc.col = srcX;
  ctx->tmpQueenSrc.row = srcY;
  ctx->tmpQueenDst.col = destX;
  ctx->tmpQueenDst.row = destY;

  CellContent currentPiece = ctx->gameState->board[srcY][srcX];
  ctx->gameState->board[srcY][srcX] = CONTENT_EMPTY;
  ctx->gameState->board[destY][destX] = currentPiece;

  drawPiece (ctx, srcX, srcY, CONTENT_EMPTY);
  drawPiece (ctx, destX, destY, currentPiece);

  ctx->currentPhase = PHASE_SHOOT_ARROW;
  highlightReachableSquares (ctx, destX, destY);

  g_print ("Moved from (%d, %d) to (%d, %d)\n", srcX, srcY, destX, destY);

  return TRUE;
}

void
onDragEnd (GtkDragSource *source, GdkDrag *drag, gboolean delete_data,
           gpointer user_data)
{
  (void)source;
  (void)drag;
  (void)delete_data;
  GuiContext *ctx = (GuiContext *)user_data;

  if (ctx->currentPhase == PHASE_MOVE_QUEEN)
    {
      clearHighlights (ctx);
      drawBoard (ctx);
    }
}

void
fireArrow (GtkWidget *button, gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;

  if (ctx->currentPhase != PHASE_SHOOT_ARROW
      || (ctx->isNetworkMatch
          && ctx->gameState->current_player != ctx->selfPlayerColor))
    return;

  int targetX
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "cell_x"));
  int targetY
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "cell_y"));

  int isLegal = GPOINTER_TO_INT (
      g_object_get_data (G_OBJECT (button), "is_legal_move"));
  if (!isLegal)
    {
      g_print ("Invalid shot at (%d, %d).\n", targetX, targetY);
      return;
    }

  CellContent piece
      = ctx->gameState->board[ctx->tmpQueenDst.row][ctx->tmpQueenDst.col];
  ctx->gameState->board[ctx->tmpQueenDst.row][ctx->tmpQueenDst.col]
      = CONTENT_EMPTY;
  ctx->gameState->board[ctx->tmpQueenSrc.row][ctx->tmpQueenSrc.col] = piece;

  Move current_move;
  current_move.queen_src = ctx->tmpQueenSrc;
  current_move.queen_dst = ctx->tmpQueenDst;
  current_move.arrow_dst.col = targetX;
  current_move.arrow_dst.row = targetY;

  int size = ctx->gameState->size;
  gtk_widget_remove_css_class (
      ctx->gameBoard[ctx->tmpQueenDst.row * size + ctx->tmpQueenDst.col],
      "active-queen");

  guiHandlePlayerMove (ctx, current_move);
}

void
onRightClick (GtkGestureClick *gesture, int n_press, double x, double y,
              gpointer user_data)
{
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
  GuiContext *ctx = (GuiContext *)user_data;

  if (ctx->currentPhase == PHASE_SHOOT_ARROW)
    {
      int srcX = ctx->tmpQueenSrc.col;
      int srcY = ctx->tmpQueenSrc.row;
      int dstX = ctx->tmpQueenDst.col;
      int dstY = ctx->tmpQueenDst.row;

      CellContent piece = ctx->gameState->board[dstY][dstX];

      ctx->gameState->board[dstY][dstX] = CONTENT_EMPTY;
      ctx->gameState->board[srcY][srcX] = piece;

      drawPiece (ctx, dstX, dstY, CONTENT_EMPTY);
      drawPiece (ctx, srcX, srcY, piece);

      clearHighlights (ctx);
      ctx->currentPhase = PHASE_MOVE_QUEEN;

      g_print ("Move cancelled.\n");
    }
}

void
onCellClicked (GtkGestureClick *gesture, int n_press, double x, double y,
               gpointer user_data)
{
  (void)n_press;
  (void)x;
  (void)y;
  GtkWidget *cellBox
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
  fireArrow (cellBox, user_data);
}

void
updateConfigVerbose (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((GuiContext *)user_data)->gameConfig->verbose
      = gtk_switch_get_active (widget);
}

void
updateConfigBlitz (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((GuiContext *)user_data)->gameConfig->blitz_mode
      = gtk_switch_get_active (widget);
}

void
updateConfigTimeout (GtkSpinButton *spin_button, gpointer user_data)
{
  ((GuiContext *)user_data)->gameConfig->time_per_player
      = gtk_spin_button_get_value_as_int (spin_button);
}

void
updateConfigBoardSize (GtkSpinButton *spin_button, gpointer user_data)
{
  ((GuiContext *)user_data)->gameConfig->board_size
      = gtk_spin_button_get_value_as_int (spin_button);
}

void
updateConfigAiWhite (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((GuiContext *)user_data)->gameConfig->ai_white
      = gtk_switch_get_active (widget);
}

void
updateConfigAiBlack (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((GuiContext *)user_data)->gameConfig->ai_black
      = gtk_switch_get_active (widget);
}

#if GTK_CHECK_VERSION(4, 10, 0)
void
updateConfigLanguage (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  guint active = gtk_drop_down_get_selected (GTK_DROP_DOWN (object));
  if (active == 0 || active == 1)
    {
      ((GuiContext *)user_data)->gameConfig->language = (lang_t)active;
    }
}

void
on_save_file_selected (GObject *source_object, GAsyncResult *res,
                       gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
  GError *error = NULL;
  GFile *file = gtk_file_dialog_save_finish (dialog, res, &error);

  if (file != NULL)
    {
      char *path = g_file_get_path (file);
      if (save_game (path, ctx->gameConfig, ctx->gameState) == 0)
        g_print ("[GUI] Game saved to : %s\n", path);
      else
        g_print ("[GUI ERROR] Saving failed.\n");

      g_free (path);
      g_object_unref (file);
    }
}

void
on_load_file_selected (GObject *source_object, GAsyncResult *res,
                       gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
  GError *error = NULL;
  GFile *file = gtk_file_dialog_open_finish (dialog, res, &error);

  if (file != NULL)
    {
      char *path = g_file_get_path (file);

      GameState dummy_state;
      dummy_state.size = MAX_BOARD_SIZE;
      game_init (&dummy_state);

      int original_size = ctx->gameConfig->board_size;

      if (load_game (path, ctx->gameConfig, &dummy_state) == 0)
        {
          g_print ("[GUI] Game loaded from : %s\n", path);

          ctx->gameConfig->board_size = dummy_state.size;

          GtkWidget *boardGrid = GTK_WIDGET (
              g_object_get_data (G_OBJECT (ctx->mainStack), "board_grid"));

          if (boardGrid)
            {
              setupBoard (ctx, boardGrid);
              load_game (path, ctx->gameConfig, ctx->gameState);
              guiStartNewGame (ctx);
            }

          ctx->currentPhase = PHASE_MOVE_QUEEN;
          clearHighlights (ctx);
          drawBoard (ctx);
          updateTurnDisplay (ctx);
        }
      else
        {
          ctx->gameConfig->board_size = original_size;
          g_print ("[GUI ERROR] Loading failed.\n");
        }

      game_cleanup (&dummy_state);
      g_free (path);
      g_object_unref (file);
    }
}

void
saveButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  if (!ctx || !ctx->gameState)
    return;
  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, "Save the current game");
  gtk_file_dialog_save (dialog, ctx->mainWindow, NULL, on_save_file_selected,
                        ctx);
}

void
loadButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  if (!ctx || !ctx->gameState)
    return;
  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, "Load a game");
  gtk_file_dialog_open (dialog, ctx->mainWindow, NULL, on_load_file_selected,
                        ctx);
}
#else
void
updateConfigLanguage (GtkComboBoxText *combo_box, gpointer user_data)
{
  int active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
  if (active == 0 || active == 1)
    {
      ((GuiContext *)user_data)->gameConfig->language = (lang_t)active;
    }
}

void
on_save_response (GtkNativeDialog *native, int response, gpointer user_data)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GuiContext *ctx = (GuiContext *)user_data;
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (native);
      GFile *file = gtk_file_chooser_get_file (chooser);
      if (file)
        {
          char *path = g_file_get_path (file);
          if (save_game (path, ctx->gameConfig, ctx->gameState) == 0)
            g_print ("[GUI] Game saved to : %s\n", path);
          else
            g_print ("[GUI ERROR] Saving failed.\n");

          g_free (path);
          g_object_unref (file);
        }
    }
  g_object_unref (native);
}

void
on_load_response (GtkNativeDialog *native, int response, gpointer user_data)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GuiContext *ctx = (GuiContext *)user_data;
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (native);
      GFile *file = gtk_file_chooser_get_file (chooser);
      if (file)
        {
          char *path = g_file_get_path (file);

          GameState dummy_state;
          dummy_state.size = MAX_BOARD_SIZE;
          game_init (&dummy_state);

          int original_size = ctx->gameConfig->board_size;

          if (load_game (path, ctx->gameConfig, &dummy_state) == 0)
            {
              g_print ("[GUI] Load game from : %s\n", path);

              ctx->gameConfig->board_size = dummy_state.size;

              GtkWidget *boardGrid = GTK_WIDGET (
                  g_object_get_data (G_OBJECT (ctx->mainStack), "board_grid"));

              if (boardGrid)
                {
                  setupBoard (ctx, boardGrid);
                  load_game (path, ctx->gameConfig, ctx->gameState);
                  guiStartNewGame (ctx);
                }

              ctx->currentPhase = PHASE_MOVE_QUEEN;
              clearHighlights (ctx);
              drawBoard (ctx);
              updateTurnDisplay (ctx);
            }
          else
            {
              ctx->gameConfig->board_size = original_size;
              g_print ("[GUI ERROR] Loading failed.\n");
            }

          game_cleanup (&dummy_state);
          g_free (path);
          g_object_unref (file);
        }
    }
  g_object_unref (native);
}

void
saveButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  if (!ctx || !ctx->gameState)
    return;
  GtkFileChooserNative *native = gtk_file_chooser_native_new (
      "Save the current game", ctx->mainWindow, GTK_FILE_CHOOSER_ACTION_SAVE,
      "Save", "Cancel");
  g_signal_connect (native, "response", G_CALLBACK (on_save_response), ctx);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
}

void
loadButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  GuiContext *ctx = (GuiContext *)user_data;

  if (!ctx || !ctx->gameState)
    return;
  GtkFileChooserNative *native = gtk_file_chooser_native_new (
      "Load a game", ctx->mainWindow, GTK_FILE_CHOOSER_ACTION_OPEN, "Open",
      "Cancel");
  g_signal_connect (native, "response", G_CALLBACK (on_load_response), ctx);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
}
#endif

gboolean
shortcutNewGame (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  newGameButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutInfo (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  infoPopupButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutConfig (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  configPopupButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutQuit (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  quitButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutUndo (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  undoButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutRedo (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  redoButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutSaveGame (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  saveButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutLoadGame (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  loadButtonClick (NULL, user_data);
  return TRUE;
}

gboolean
shortcutDummy (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  (void)user_data;
  g_print ("Not implemented.\n");
  return TRUE;
}
