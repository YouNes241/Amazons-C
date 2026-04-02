#include "gui_pages.h"
#include "gui_callbacks.h"
#include <gtk/gtk.h>

GtkWidget *
createMainMenuPage (GuiContext *ctx)
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *singleplayerBtn;
  GtkWidget *multiplayerBtn;
  GtkWidget *configBtn;
  GtkWidget *infoBtn;
  GtkWidget *quitBtn;

  builder = gtk_builder_new_from_resource ("/amazons/ui/main_menu.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "main_menu_box"));

  singleplayerBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "singleplayer_btn"));
  multiplayerBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "multiplayer_btn"));
  configBtn = GTK_WIDGET (gtk_builder_get_object (builder, "config_btn"));
  infoBtn = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  quitBtn = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));

  g_object_set_data (G_OBJECT (singleplayerBtn), "target_page",
                     "singleplayer_page");
  g_object_set_data (G_OBJECT (multiplayerBtn), "target_page",
                     "multiplayer_page");
  g_object_set_data (G_OBJECT (configBtn), "target_page", "config_page");
  g_object_set_data (G_OBJECT (infoBtn), "target_page", "info_page");

  g_signal_connect (singleplayerBtn, "clicked", G_CALLBACK (navigationClick),
                    ctx);
  g_signal_connect (multiplayerBtn, "clicked", G_CALLBACK (navigationClick),
                    ctx);
  g_signal_connect (configBtn, "clicked", G_CALLBACK (navigationClick), ctx);
  g_signal_connect (infoBtn, "clicked", G_CALLBACK (navigationClick), ctx);
  g_signal_connect (quitBtn, "clicked", G_CALLBACK (quitButtonClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createSingleplayerMenuPage (GuiContext *ctx)
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *newGameBtn;
  GtkWidget *returnBtn;

  builder = gtk_builder_new_from_resource ("/amazons/ui/singleplayer.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "singleplayer_box"));

  newGameBtn = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_btn"));
  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_object_set_data (G_OBJECT (returnBtn), "target_page", "main_menu_page");

  g_signal_connect (newGameBtn, "clicked", G_CALLBACK (newGameButtonClick),
                    ctx);
  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createMultiplayerMenuPage (GuiContext *ctx)
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *returnBtn;
  GtkWidget *hostServerBtn;
  GtkWidget *joinServerBtn;

  builder = gtk_builder_new_from_resource ("/amazons/ui/multiplayer.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "multiplayer_box"));

  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));
  hostServerBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "host_server_btn"));
  joinServerBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "join_server_btn"));

  g_object_set_data (G_OBJECT (returnBtn), "target_page", "main_menu_page");

  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick), ctx);

  if (hostServerBtn)
    {
      g_signal_connect (hostServerBtn, "clicked",
                        G_CALLBACK (hostServerPopupButtonClick), ctx);
    }

  if (joinServerBtn)
    {
      g_object_set_data (G_OBJECT (joinServerBtn), "target_page",
                         "join_server_page");
      g_signal_connect (joinServerBtn, "clicked", G_CALLBACK (navigationClick),
                        ctx);
    }

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createConfigPage (GuiContext *ctx)
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *returnBtn;
  GtkWidget *verboseSwitch;
  GtkWidget *blitzSwitch;
  GtkWidget *timeoutField;
  GtkWidget *boardSizeField;
  GtkWidget *aiWhiteSwitch;
  GtkWidget *aiBlackSwitch;
  GtkWidget *langDropdown;

  builder = gtk_builder_new_from_resource ("/amazons/ui/config.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "config_box"));
  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  verboseSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "verbose_switch"));
  blitzSwitch = GTK_WIDGET (gtk_builder_get_object (builder, "blitz_switch"));
  timeoutField
      = GTK_WIDGET (gtk_builder_get_object (builder, "timeout_field"));
  boardSizeField
      = GTK_WIDGET (gtk_builder_get_object (builder, "board_size_field"));
  aiWhiteSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "ai_white_switch"));
  aiBlackSwitch
      = GTK_WIDGET (gtk_builder_get_object (builder, "ai_black_switch"));

  GtkWidget *langDropdownContainer = GTK_WIDGET (
      gtk_builder_get_object (builder, "lang_dropdown_container"));

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
                           ctx->gameConfig->ai_white_cfg.enabled);
  if (aiBlackSwitch)
    gtk_switch_set_active (GTK_SWITCH (aiBlackSwitch),
                           ctx->gameConfig->ai_black_cfg.enabled);

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
  const char *langs[] = { "English", "French", NULL };
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
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (langDropdown), "French");
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

  g_object_set_data (G_OBJECT (returnBtn), "target_page", "main_menu_page");
  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createInfoPage (GuiContext *ctx)
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *returnBtn;

  builder = gtk_builder_new_from_resource ("/amazons/ui/info.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "info_box"));

  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_object_set_data (G_OBJECT (returnBtn), "target_page", "main_menu_page");
  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createGamePage (GuiContext *ctx)
{
  GtkBuilder *builder = gtk_builder_new_from_resource ("/amazons/ui/game.ui");
  GtkWidget *box = GTK_WIDGET (gtk_builder_get_object (builder, "game_box"));

  g_object_set_data (G_OBJECT (ctx->mainStack), "game_box", box);

  GtkWidget *newGameBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_btn"));
  GtkWidget *loadBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "load_game_btn"));
  GtkWidget *saveBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "save_game_btn"));
  GtkWidget *infoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  GtkWidget *configBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "config_btn"));
  GtkWidget *quitBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));
  GtkWidget *undoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "undo_btn"));
  GtkWidget *redoBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "redo_btn"));
  GtkWidget *hintBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "hint_btn"));
  GtkWidget *turnLabel
      = GTK_WIDGET (gtk_builder_get_object (builder, "turn_label"));
  GtkWidget *boardGrid
      = GTK_WIDGET (gtk_builder_get_object (builder, "board_grid"));

  g_object_set_data (G_OBJECT (ctx->mainStack), "turn_label", turnLabel);
  g_object_set_data (G_OBJECT (ctx->mainStack), "board_grid", boardGrid);

  ctx->gameState = NULL;
  ctx->gameBoard = NULL;

  setupBoard (ctx, boardGrid);

  GtkGesture *rightClick = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (rightClick),
                                 GDK_BUTTON_SECONDARY);
  g_signal_connect (rightClick, "pressed", G_CALLBACK (onRightClick), ctx);
  gtk_widget_add_controller (boardGrid, GTK_EVENT_CONTROLLER (rightClick));

  g_signal_connect (quitBtn, "clicked", G_CALLBACK (quitGameButtonClick), ctx);
  g_signal_connect (newGameBtn, "clicked", G_CALLBACK (newGameButtonClick),
                    ctx);
  g_signal_connect (loadBtn, "clicked", G_CALLBACK (loadButtonClick), ctx);
  g_signal_connect (saveBtn, "clicked", G_CALLBACK (saveButtonClick), ctx);
  g_signal_connect (infoBtn, "clicked", G_CALLBACK (infoPopupButtonClick),
                    ctx);
  g_signal_connect (configBtn, "clicked", G_CALLBACK (configPopupButtonClick),
                    ctx);
  g_signal_connect (undoBtn, "clicked", G_CALLBACK (undoButtonClick), ctx);
  g_signal_connect (redoBtn, "clicked", G_CALLBACK (redoButtonClick), ctx);
  g_signal_connect (hintBtn, "clicked", G_CALLBACK (hintButtonClick), ctx);

  updateGameShortcuts (ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createJoinServerPage (GuiContext *ctx)
{
  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/join_server.ui");
  GtkWidget *box
      = GTK_WIDGET (gtk_builder_get_object (builder, "join_server_box"));

  GtkWidget *returnBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));
  GtkWidget *refreshBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "refresh_btn"));
  GtkWidget *usernameEntry
      = GTK_WIDGET (gtk_builder_get_object (builder, "username_entry"));
  GtkWidget *serverListbox
      = GTK_WIDGET (gtk_builder_get_object (builder, "server_listbox"));

  g_object_set_data (G_OBJECT (returnBtn), "target_page", "multiplayer_page");
  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick), ctx);

  g_object_set_data (G_OBJECT (refreshBtn), "server_listbox", serverListbox);
  g_object_set_data (G_OBJECT (refreshBtn), "username_entry", usernameEntry);

  g_signal_connect (refreshBtn, "clicked",
                    G_CALLBACK (refreshServerListButtonClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createHostPanelPage (GuiContext *ctx)
{
  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/host_panel.ui");
  GtkWidget *box
      = GTK_WIDGET (gtk_builder_get_object (builder, "server_panel_box"));

  GtkWidget *stopServerBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "stop_server_btn"));

  GtkWidget *consoleView
      = GTK_WIDGET (gtk_builder_get_object (builder, "server_console_view"));
  if (ctx->mainStack)
    {
      g_object_set_data (G_OBJECT (ctx->mainStack), "server_console_view",
                         consoleView);
    }

  g_signal_connect (stopServerBtn, "clicked",
                    G_CALLBACK (stopServerButtonClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createClientPanelPage (GuiContext *ctx)
{
  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/client_panel.ui");
  GtkWidget *box
      = GTK_WIDGET (gtk_builder_get_object (builder, "client_panel_box"));
  GtkWidget *titleLabel
      = GTK_WIDGET (gtk_builder_get_object (builder, "lobby_title_label"));
  if (titleLabel && ctx->mainStack)
    {
      g_object_set_data (G_OBJECT (ctx->mainStack), "lobby_title_label",
                         titleLabel);
    }

  GtkWidget *disconnectBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "disconnect_btn"));
  GtkWidget *createGameBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "create_game_btn"));

  g_signal_connect (disconnectBtn, "clicked",
                    G_CALLBACK (disconnectFromServerButtonClick), ctx);
  g_signal_connect (createGameBtn, "clicked",
                    G_CALLBACK (createGameButtonClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}