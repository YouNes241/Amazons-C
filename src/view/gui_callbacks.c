#include "gui_callbacks.h"
#include "ai_factory.h"
#include "game_engine.h"
#include "gui_controller.h"
#include "network_common.h"
#include <assert.h>
#include <save.h>
#include <stdio.h>
#include <stdlib.h>

void
navigationClick (GtkButton *button, gpointer user_data)
{
  assert (button != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);
  assert (ctx->mainStack != NULL);

  const char *targetPage
      = (const char *)g_object_get_data (G_OBJECT (button), "target_page");
  assert (targetPage != NULL);

  gtk_stack_set_visible_child_name (ctx->mainStack, targetPage);
}

void
quitButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);

  gtk_window_close (GTK_WINDOW (ctx->mainWindow));
}

void
infoPopupButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);

  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/popups/info_popup.ui");
  assert (builder != NULL);

  GtkWindow *infoWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "info_window"));
  assert (infoWindow != NULL);

  gtk_window_set_transient_for (infoWindow, GTK_WINDOW (ctx->mainWindow));
  gtk_window_present (infoWindow);

  g_object_unref (builder);
}

void
configPopupButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);
  assert (ctx->gameConfig != NULL);

  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/popups/config_popup.ui");
  assert (builder != NULL);

  GtkWindow *configWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "config_window"));
  assert (configWindow != NULL);

  gtk_window_set_transient_for (configWindow, GTK_WINDOW (ctx->mainWindow));

  GtkWidget *verboseSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "verbose_switch"));
  assert (verboseSwitch != NULL);

  GtkWidget *blitzSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "blitz_switch"));
  assert (blitzSwitch != NULL);

  GtkWidget *timeoutField
      = GTK_WIDGET (gtk_builder_get_object (builder, "timeout_field"));
  assert (timeoutField != NULL);

  GtkWidget *boardSizeField
      = GTK_WIDGET (gtk_builder_get_object (builder, "board_size_field"));
  assert (boardSizeField != NULL);

  GtkWidget *aiWhiteSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "ai_white_switch"));
  assert (aiWhiteSwitch != NULL);

  GtkWidget *aiBlackSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "ai_black_switch"));
  assert (aiBlackSwitch != NULL);

  GtkWidget *langDropdownContainer = GTK_WIDGET (
      gtk_builder_get_object (builder, "lang_dropdown_container"));

  GtkWidget *bindNewGameBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_new_game_btn"));
  assert (bindNewGameBtn != NULL);

  GtkWidget *bindLoadBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_load_game_btn"));
  assert (bindLoadBtn != NULL);

  GtkWidget *bindSaveBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_save_game_btn"));
  assert (bindSaveBtn != NULL);

  GtkWidget *bindUndoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_undo_btn"));
  assert (bindUndoBtn != NULL);

  GtkWidget *bindRedoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_redo_btn"));
  assert (bindRedoBtn != NULL);

  GtkWidget *bindPauseBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_pause_btn"));
  assert (bindPauseBtn != NULL);

  GtkWidget *bindHintBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_hint_btn"));
  assert (bindHintBtn != NULL);

  GtkWidget *bindConfigBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_config_btn"));
  assert (bindConfigBtn != NULL);

  GtkWidget *bindInfoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_info_btn"));
  assert (bindInfoBtn != NULL);

  GtkWidget *bindQuitBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "bind_quit_btn"));
  assert (bindQuitBtn != NULL);

  gtk_switch_set_active (GTK_SWITCH (verboseSwitch), ctx->gameConfig->verbose);
  gtk_switch_set_active (GTK_SWITCH (blitzSwitch),
                         ctx->gameConfig->blitz_mode);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (timeoutField),
                             ctx->gameConfig->time_per_player);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (boardSizeField),
                             ctx->gameConfig->board_size);
  gtk_switch_set_active (GTK_SWITCH (aiWhiteSwitch),
                         ctx->gameConfig->ai_white_cfg.enabled);
  gtk_switch_set_active (GTK_SWITCH (aiBlackSwitch),
                         ctx->gameConfig->ai_black_cfg.enabled);

  g_signal_connect (verboseSwitch, "notify::active",
                    G_CALLBACK (updateConfigVerbose), ctx);
  g_signal_connect (blitzSwitch, "notify::active",
                    G_CALLBACK (updateConfigBlitz), ctx);
  g_signal_connect (timeoutField, "value-changed",
                    G_CALLBACK (updateConfigTimeout), ctx);
  g_signal_connect (boardSizeField, "value-changed",
                    G_CALLBACK (updateConfigBoardSize), ctx);
  g_signal_connect (aiWhiteSwitch, "notify::active",
                    G_CALLBACK (updateConfigAiWhite), ctx);
  g_signal_connect (aiBlackSwitch, "notify::active",
                    G_CALLBACK (updateConfigAiBlack), ctx);

  GtkWidget *langDropdown;
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
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);

  if (ctx->is_ai_thinking)
    {
      guiShowPopup (ctx, "Action disabled. AI is thinking...");
      return;
    }

  if (ctx->currentPhase == PHASE_SHOOT_ARROW)
    {
      onRightClick (NULL, 0, 0, 0, ctx);
      return;
    }

  if (ctx->isNetworkMatch)
    {
      guiShowPopup (ctx, "Action disabled during a network match.");
      return;
    }

  if (undo_move (ctx->gameState))
    {
      bool has_ai = ctx->gameConfig->ai_white_cfg.enabled
                    || ctx->gameConfig->ai_black_cfg.enabled;
      if (has_ai && ctx->gameState->undo_stack.top >= 0)
        {
          undo_move (ctx->gameState);
        }

      clearHintHighlights (ctx);
      ctx->isHintActive = false;
      ctx->currentPhase = PHASE_MOVE_QUEEN;
      clearHighlights (ctx);
      drawBoard (ctx);
      updateTurnDisplay (ctx);
      guiCheckAndTriggerAI (ctx);
    }
}

void
redoButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);

  if (ctx->is_ai_thinking)
    {
      guiShowPopup (ctx, "Action disabled. AI is thinking...");
      return;
    }

  if (ctx->currentPhase == PHASE_SHOOT_ARROW)
    {
      onRightClick (NULL, 0, 0, 0, ctx);
      return;
    }

  if (ctx->isNetworkMatch)
    {
      guiShowPopup (ctx, "Action disabled during a network match.");
      return;
    }

  if (redo_move (ctx->gameState))
    {
      bool has_ai = ctx->gameConfig->ai_white_cfg.enabled
                    || ctx->gameConfig->ai_black_cfg.enabled;
      if (has_ai && ctx->gameState->redo_stack.top >= 0)
        {
          redo_move (ctx->gameState);
        }

      clearHintHighlights (ctx);
      ctx->isHintActive = false;
      ctx->currentPhase = PHASE_MOVE_QUEEN;
      clearHighlights (ctx);
      drawBoard (ctx);
      updateTurnDisplay (ctx);
      guiCheckAndTriggerAI (ctx);
    }
}

void
hintButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);

  if (ctx->is_ai_thinking)
    {
      return;
    }

  if (ctx->currentPhase != PHASE_MOVE_QUEEN)
    {
      guiShowPopup (ctx, "Finish your current move first.");
      return;
    }

  if (ctx->isHintActive)
    {
      clearHintHighlights (ctx);
      ctx->isHintActive = false;
      return;
    }

  ctx->is_ai_thinking = true;

  ConfigMap empty_params = { 0, NULL };

  AIWorkerData *data = malloc (sizeof (AIWorkerData));
  assert (data != NULL);

  data->ai = malloc (sizeof (AI_Interface));
  assert (data->ai != NULL);

  data->ctx = ctx;
  data->state_copy = *(ctx->gameState);

  *(data->ai) = create_ai ("minimax", empty_params);
  assert (data->ai->vtable != NULL);

  g_thread_new ("hint_thread", guiHintWorkerThread, data);
}

void
quitGameButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainStack != NULL);

  if (ctx->isNetworkMatch)
    {
      NetworkManager_SendGameOver (ctx->networkManager, false);

      ctx->isNetworkMatch = false;
      gtk_stack_set_visible_child_name (ctx->mainStack, "client_panel_page");
    }
  else
    {
      gtk_stack_set_visible_child_name (ctx->mainStack, "main_menu_page");
    }
}

void
joinServerButtonClick (GtkButton *button, gpointer user_data)
{
  assert (button != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->networkManager != NULL);

  const char *ip_str
      = (const char *)g_object_get_data (G_OBJECT (button), "server_ip");
  assert (ip_str != NULL);

  int port
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "server_port"));

  GtkEditable *username_entry
      = GTK_EDITABLE (g_object_get_data (G_OBJECT (button), "username_entry"));
  assert (username_entry != NULL);

  const char *username_str = gtk_editable_get_text (username_entry);

  if (!username_str || strlen (username_str) == 0)
    {
      g_print ("[GUI ERROR] Username cannot be empty.\n");
      return;
    }

  g_print ("[NETWORK] Attempting to connect to %s:%d as %s\n", ip_str, port,
           username_str);

  if (NetworkManager_ClientJoin (ctx->networkManager, ip_str, port) == 0)
    {
      NetworkManager_SendJoin (ctx->networkManager, username_str);

      assert (ctx->mainStack != NULL);

      GtkWidget *titleLabel = GTK_WIDGET (
          g_object_get_data (G_OBJECT (ctx->mainStack), "lobby_title_label"));
      assert (titleLabel != NULL);

      char *titleText = g_strdup_printf ("%s", username_str);
      gtk_label_set_text (GTK_LABEL (titleLabel), titleText);
      g_free (titleText);

      gtk_stack_set_visible_child_name (ctx->mainStack, "client_panel_page");
    }
  else
    {
      g_print ("[NETWORK ERROR] Connection failed.\n");
    }
}

void
refreshServerListButtonClick (GtkButton *button, gpointer user_data)
{
  assert (button != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->networkManager != NULL);

  GtkListBox *listbox
      = GTK_LIST_BOX (g_object_get_data (G_OBJECT (button), "server_listbox"));
  assert (listbox != NULL);

  GtkWidget *username_entry
      = GTK_WIDGET (g_object_get_data (G_OBJECT (button), "username_entry"));
  assert (username_entry != NULL);

  GtkWidget *child;
  while ((child = gtk_widget_get_first_child (GTK_WIDGET (listbox))) != NULL)
    {
      gtk_list_box_remove (listbox, child);
    }

  char list_buffer[2048] = { 0 };

  if (NetworkManager_GetServerList (ctx->networkManager, list_buffer,
                                    sizeof (list_buffer))
      > 0)
    {
      char *line = strtok (list_buffer, "\n");
      while (line != NULL)
        {
          char name[64] = { 0 };
          char ip[64] = { 0 };
          int port = 0;

          if (sscanf (line, "[%*d] %63[^(] (IP: %63[^,], Port: %d)", name, ip,
                      &port)
              == 3)
            {
              g_strchomp (name);

              GtkWidget *row_box
                  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
              gtk_widget_set_margin_start (row_box, 10);
              gtk_widget_set_margin_end (row_box, 10);
              gtk_widget_set_margin_top (row_box, 10);
              gtk_widget_set_margin_bottom (row_box, 10);

              char display_text[256];
              snprintf (display_text, sizeof (display_text), "%s\t\t%s:%d",
                        name, ip, port);

              GtkWidget *name_label = gtk_label_new (display_text);
              gtk_widget_set_halign (name_label, GTK_ALIGN_START);
              gtk_widget_set_hexpand (name_label, TRUE);

              GtkWidget *join_btn = gtk_button_new_with_label ("Join");

              g_object_set_data_full (G_OBJECT (join_btn), "server_ip",
                                      g_strdup (ip), g_free);
              g_object_set_data (G_OBJECT (join_btn), "server_port",
                                 GINT_TO_POINTER (port));
              g_object_set_data (G_OBJECT (join_btn), "username_entry",
                                 username_entry);

              g_signal_connect (join_btn, "clicked",
                                G_CALLBACK (joinServerButtonClick), ctx);

              gtk_box_append (GTK_BOX (row_box), name_label);
              gtk_box_append (GTK_BOX (row_box), join_btn);

              gtk_list_box_append (listbox, row_box);
            }
          line = strtok (NULL, "\n");
        }
    }
  else
    {
      GtkWidget *empty_label
          = gtk_label_new ("No local servers found. Please refresh.");
      gtk_widget_set_margin_top (empty_label, 15);
      gtk_widget_set_margin_bottom (empty_label, 15);
      gtk_list_box_append (listbox, empty_label);
    }
}

void
disconnectFromServerButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainStack != NULL);

  guiDisconnectAndRestartNetwork (ctx);

  gtk_stack_set_visible_child_name (ctx->mainStack, "join_server_page");
}

void
createGameButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);

  GtkBuilder *builder = gtk_builder_new_from_resource (
      "/amazons/ui/popups/create_game_popup.ui");
  assert (builder != NULL);

  GtkWindow *createGameWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "create_game_window"));
  assert (createGameWindow != NULL);

  gtk_window_set_transient_for (createGameWindow,
                                GTK_WINDOW (ctx->mainWindow));

  GtkWidget *targetIdEntry
      = GTK_WIDGET (gtk_builder_get_object (builder, "target_id_entry"));
  assert (targetIdEntry != NULL);

  GtkWidget *boardSizeSpin
      = GTK_WIDGET (gtk_builder_get_object (builder, "board_size_spin"));
  assert (boardSizeSpin != NULL);

  GtkWidget *cancelBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "cancel_btn"));
  assert (cancelBtn != NULL);

  GtkWidget *inviteBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "invite_btn"));
  assert (inviteBtn != NULL);

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
  assert (button != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);

  GtkEditable *target_id_entry = GTK_EDITABLE (
      g_object_get_data (G_OBJECT (button), "target_id_entry"));
  assert (target_id_entry != NULL);

  GtkSpinButton *board_size_spin = GTK_SPIN_BUTTON (
      g_object_get_data (G_OBJECT (button), "board_size_spin"));
  assert (board_size_spin != NULL);

  GtkWindow *popup_window
      = GTK_WINDOW (g_object_get_data (G_OBJECT (button), "popup_window"));
  assert (popup_window != NULL);

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
  assert (ctx != NULL);
  assert (ctx->mainStack != NULL);

  if (ctx->isNetworkMatch)
    {
      guiShowPopup (ctx, "Action disabled during a network match.");
      return;
    }

  GtkWidget *boardGrid = GTK_WIDGET (
      g_object_get_data (G_OBJECT (ctx->mainStack), "board_grid"));
  assert (boardGrid != NULL);

  setupBoard (ctx, boardGrid);

  guiStartNewGame (ctx);
  gtk_stack_set_visible_child_name (ctx->mainStack, "game_page");
}

void
hostServerPopupButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);

  GtkBuilder *builder = gtk_builder_new_from_resource (
      "/amazons/ui/popups/create_server_popup.ui");
  assert (builder != NULL);

  GtkWindow *createServerWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "create_server_window"));
  assert (createServerWindow != NULL);

  gtk_window_set_transient_for (createServerWindow,
                                GTK_WINDOW (ctx->mainWindow));

  GtkWidget *portSpin
      = GTK_WIDGET (gtk_builder_get_object (builder, "port_spin"));
  assert (portSpin != NULL);

  GtkWidget *cancelBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "cancel_btn"));
  assert (cancelBtn != NULL);

  GtkWidget *startBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "start_btn"));
  assert (startBtn != NULL);

  g_object_set_data (G_OBJECT (startBtn), "port_spin", portSpin);
  g_object_set_data (G_OBJECT (startBtn), "popup_window", createServerWindow);

  g_signal_connect_swapped (cancelBtn, "clicked",
                            G_CALLBACK (gtk_window_destroy),
                            createServerWindow);

  g_signal_connect (startBtn, "clicked", G_CALLBACK (startServerButtonClick),
                    ctx);

  gtk_window_present (createServerWindow);
  g_object_unref (builder);
}

void
startServerButtonClick (GtkButton *button, gpointer user_data)
{
  assert (button != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);

  GtkSpinButton *port_spin
      = GTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (button), "port_spin"));
  assert (port_spin != NULL);

  GtkWindow *popup_window
      = GTK_WINDOW (g_object_get_data (G_OBJECT (button), "popup_window"));
  assert (popup_window != NULL);

  int port = gtk_spin_button_get_value_as_int (port_spin);

  if (NetworkManager_ServerStart (ctx->networkManager, port) == 0)
    {

      assert (ctx->mainStack != NULL);

      gtk_stack_set_visible_child_name (ctx->mainStack, "host_panel_page");

      gui_log_to_server_console (ctx, "[*] Server initialized on port %d",
                                 port);
      gui_log_to_server_console (ctx, "[*] Waiting for players to connect...");
    }
  else
    {
      g_print ("[ERROR] Failed to start server. Port %d might be in use.\n",
               port);
    }

  gtk_window_destroy (popup_window);
}

void
stopServerButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->mainStack != NULL);

  g_print ("[SERVER] Shutting down server...\n");

  NetworkManager_ShutdownServer (ctx->networkManager);

  gtk_stack_set_visible_child_name (ctx->mainStack, "multiplayer_page");
}

GdkContentProvider *
onDrag (GtkDragSource *source, double x, double y, gpointer user_data)
{
  (void)x;
  (void)y;

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameBoard != NULL);

  GtkWidget *btn
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (source));
  assert (btn != NULL);

  int srcX = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_x"));
  int srcY = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_y"));
  int srcIndex
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_index"));

  int size = ctx->gameState->size;
  assert (srcX >= 0 && srcX < size);
  assert (srcY >= 0 && srcY < size);

  CellContent piece = ctx->gameState->board[srcY][srcX];

  if (piece == CONTENT_EMPTY || piece == CONTENT_ARROW)
    {
      return NULL;
    }

  PlayerColor currentPlayer = ctx->gameState->current_player;

  if ((ctx->isNetworkMatch && currentPlayer != ctx->selfPlayerColor)
      || ctx->currentPhase != PHASE_MOVE_QUEEN)
    {
      return NULL;
    }

  bool is_ai_turn = (currentPlayer == PLAYER_WHITE
                     && ctx->gameConfig->ai_white_cfg.enabled)
                    || (currentPlayer == PLAYER_BLACK
                        && ctx->gameConfig->ai_black_cfg.enabled);

  if (is_ai_turn && !ctx->isNetworkMatch)
    {
      return NULL;
    }

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

  if (foundCells == 0)
    {
      clearHighlights (ctx);
      return NULL;
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
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);

  if (ctx->currentPhase != PHASE_MOVE_QUEEN)
    {
      return FALSE;
    }

  GtkWidget *btn
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));
  assert (btn != NULL);

  int destX = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_x"));
  int destY = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_y"));

  int size = ctx->gameState->size;
  assert (destX >= 0 && destX < size);
  assert (destY >= 0 && destY < size);

  int isLegal
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "is_legal_move"));
  if (!isLegal)
    {
      return FALSE;
    }

  assert (value != NULL);
  const char *srcString = g_value_get_string (value);
  int srcIndex = atoi (srcString);

  int srcX = srcIndex % size;
  int srcY = srcIndex / size;
  assert (srcX >= 0 && srcX < size);
  assert (srcY >= 0 && srcY < size);

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
  assert (ctx != NULL);

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
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (button != NULL);

  if (ctx->currentPhase != PHASE_SHOOT_ARROW
      || (ctx->isNetworkMatch
          && ctx->gameState->current_player != ctx->selfPlayerColor))
    return;

  int targetX
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "cell_x"));
  int targetY
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "cell_y"));

  int size = ctx->gameState->size;
  assert (targetX >= 0 && targetX < size);
  assert (targetY >= 0 && targetY < size);

  int isLegal = GPOINTER_TO_INT (
      g_object_get_data (G_OBJECT (button), "is_legal_move"));
  if (!isLegal)
    return;

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
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);

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
  assert (cellBox != NULL);

  fireArrow (cellBox, user_data);
}

void
updateConfigVerbose (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;

  assert (widget != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  ctx->gameConfig->verbose = gtk_switch_get_active (widget);
}

void
updateConfigBlitz (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;

  assert (widget != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  ctx->gameConfig->blitz_mode = gtk_switch_get_active (widget);
}

void
updateConfigTimeout (GtkSpinButton *spin_button, gpointer user_data)
{
  assert (spin_button != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  ctx->gameConfig->time_per_player
      = gtk_spin_button_get_value_as_int (spin_button);
}

void
updateConfigBoardSize (GtkSpinButton *spin_button, gpointer user_data)
{
  assert (spin_button != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  ctx->gameConfig->board_size = gtk_spin_button_get_value_as_int (spin_button);
}

void
updateConfigAiWhite (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;

  assert (widget != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  ctx->gameConfig->ai_white_cfg.enabled = gtk_switch_get_active (widget);
}

void
updateConfigAiBlack (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;

  assert (widget != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  ctx->gameConfig->ai_black_cfg.enabled = gtk_switch_get_active (widget);
}

#if GTK_CHECK_VERSION(4, 10, 0)
void
updateConfigLanguage (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;

  assert (object != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  guint active = gtk_drop_down_get_selected (GTK_DROP_DOWN (object));
  if (active == 0 || active == 1)
    {
      ctx->gameConfig->language = (lang_t)active;
    }
}

void
on_save_file_selected (GObject *source_object, GAsyncResult *res,
                       gpointer user_data)
{
  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);
  assert (ctx->gameState != NULL);

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
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);
  assert (ctx->mainStack != NULL);

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
          assert (boardGrid != NULL);

          setupBoard (ctx, boardGrid);
          load_game (path, ctx->gameConfig, ctx->gameState);
          guiStartNewGame (ctx);

          ctx->currentPhase = PHASE_MOVE_QUEEN;
          clearHighlights (ctx);
          drawBoard (ctx);
          updateTurnDisplay (ctx);

          gtk_stack_set_visible_child_name (ctx->mainStack, "game_page");
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
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);
  assert (ctx->gameState != NULL);

  if (ctx->currentPhase == PHASE_SHOOT_ARROW)
    {
      guiShowPopup (
          ctx, "Please finish your move (shoot the arrow) before saving.");
      return;
    }

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
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);

  if (ctx->isNetworkMatch)
    {
      guiShowPopup (ctx, "Action disabled during a network match.");
      return;
    }

  GtkFileDialog *dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, "Load a game");
  gtk_file_dialog_open (dialog, ctx->mainWindow, NULL, on_load_file_selected,
                        ctx);
}
#else
void
updateConfigLanguage (GtkComboBoxText *combo_box, gpointer user_data)
{
  assert (combo_box != NULL);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);

  int active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
  if (active == 0 || active == 1)
    {
      ctx->gameConfig->language = (lang_t)active;
    }
}

void
on_save_response (GtkNativeDialog *native, int response, gpointer user_data)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GuiContext *ctx = (GuiContext *)user_data;
      assert (ctx != NULL);
      assert (ctx->gameConfig != NULL);
      assert (ctx->gameState != NULL);

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
      assert (ctx != NULL);
      assert (ctx->gameConfig != NULL);
      assert (ctx->mainStack != NULL);

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
              assert (boardGrid != NULL);

              setupBoard (ctx, boardGrid);
              load_game (path, ctx->gameConfig, ctx->gameState);
              guiStartNewGame (ctx);

              ctx->currentPhase = PHASE_MOVE_QUEEN;
              clearHighlights (ctx);
              drawBoard (ctx);
              updateTurnDisplay (ctx);

              gtk_stack_set_visible_child_name (ctx->mainStack, "game_page");
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
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->mainWindow != NULL);

  if (ctx->currentPhase == PHASE_SHOOT_ARROW)
    {
      guiShowPopup (
          ctx, "Please finish your move (shoot the arrow) before saving.");
      return;
    }

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
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->mainWindow != NULL);

  if (ctx->isNetworkMatch)
    {
      guiShowPopup (ctx, "Action disabled during a network match.");
      return;
    }

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
  quitGameButtonClick (NULL, user_data);
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
shortcutHint (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;

  hintButtonClick (NULL, user_data);
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

gboolean
guiHintApplyMove (gpointer user_data)
{
  AIWorkerData *data = (AIWorkerData *)user_data;
  GuiContext *ctx = data->ctx;

  if (data->ai->vtable->clean)
    {
      data->ai->vtable->clean (data->ai);
    }

  if (ctx->gameState
      && ctx->gameState->current_player == data->state_copy.current_player)
    {
      showHintOnBoard (ctx, data->best_move.queen_src,
                       data->best_move.queen_dst, data->best_move.arrow_dst);
      ctx->isHintActive = true;
    }

  ctx->is_ai_thinking = false;

  free (data->ai);
  free (data);
  return G_SOURCE_REMOVE;
}

gpointer
guiHintWorkerThread (gpointer user_data)
{
  AIWorkerData *data = (AIWorkerData *)user_data;

  data->best_move
      = data->ai->vtable->get_best_move (data->ai, data->state_copy);

  g_idle_add (guiHintApplyMove, data);

  return NULL;
}