#include "gui_pages.h"
#include "game_engine.h"
#include "gui_controller.h"
#include "gui_utils.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

static void navigationClick (GtkButton *button, gpointer user_data);
static void quitButtonClick (GtkButton *button, gpointer user_data);
static void infoPopupButtonClick (GtkButton *button, gpointer user_data);
static void configPopupButtonClick (GtkButton *button, gpointer user_data);
static void undoButtonClick (GtkButton *button, gpointer user_data);
static void redoButtonClick (GtkButton *button, gpointer user_data);
static void setupBoard (AppContext *ctx, GtkWidget *boardGrid);
static void newGameButtonClick (GtkButton *button, gpointer user_data);
static GdkContentProvider *onDrag (GtkDragSource *source, double x, double y,
                                   gpointer user_data);
static gboolean onDrop (GtkDropTarget *target, const GValue *value, double x,
                        double y, gpointer user_data);
static void onDragEnd (GtkDragSource *source, GdkDrag *drag,
                       gboolean delete_data, gpointer user_data);
static void fireArrow (GtkWidget *button, gpointer user_data);
static void onRightClick (GtkGestureClick *gesture, int n_press, double x,
                          double y, gpointer user_data);
static void onCellClicked (GtkGestureClick *gesture, int n_press, double x,
                           double y, gpointer user_data);
static void updateConfigVerbose (GtkSwitch *widget, GParamSpec *pspec,
                                 gpointer user_data);
static void updateConfigBlitz (GtkSwitch *widget, GParamSpec *pspec,
                               gpointer user_data);
static void updateConfigTimeout (GtkSpinButton *spin_button,
                                 gpointer user_data);
static void updateConfigBoardSize (GtkSpinButton *spin_button,
                                   gpointer user_data);
static void updateConfigAiWhite (GtkSwitch *widget, GParamSpec *pspec,
                                 gpointer user_data);
static void updateConfigAiBlack (GtkSwitch *widget, GParamSpec *pspec,
                                 gpointer user_data);
#if GTK_CHECK_VERSION(4, 10, 0)
static void updateConfigLanguage (GObject *object, GParamSpec *pspec,
                                  gpointer user_data);
#else
static void updateConfigLanguage (GtkComboBoxText *combo_box,
                                  gpointer user_data);
#endif
static gboolean shortcutNewGame (GtkWidget *widget, GVariant *args,
                                 gpointer user_data);
static gboolean shortcutInfo (GtkWidget *widget, GVariant *args,
                              gpointer user_data);
static gboolean shortcutConfig (GtkWidget *widget, GVariant *args,
                                gpointer user_data);
static gboolean shortcutQuit (GtkWidget *widget, GVariant *args,
                              gpointer user_data);
static gboolean shortcutUndo (GtkWidget *widget, GVariant *args,
                              gpointer user_data);
// REDO NEEDS ENGINE FIX //
static gboolean shortcutRedo (GtkWidget *widget, GVariant *args,
                              gpointer user_data);
static gboolean shortcutDummy (GtkWidget *widget, GVariant *args,
                               gpointer user_data);
static void bindingButtonAnimate (GtkToggleButton *button, gpointer user_data);
static gboolean bindingKeyPressed (GtkEventControllerKey *controller,
                                   guint keyval, guint keycode,
                                   GdkModifierType state, gpointer user_data);
static void setupBindingButton (GtkWidget *btn, char *target_str,
                                AppContext *ctx);
static void updateGameShortcuts (AppContext *ctx);

GtkWidget *
createMainMenuPage (AppContext *ctx)
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
createSingleplayerMenuPage (AppContext *ctx)
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
createMultiplayerMenuPage (AppContext *ctx)
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *returnBtn;

  builder = gtk_builder_new_from_resource ("/amazons/ui/multiplayer.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "multiplayer_box"));

  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_object_set_data (G_OBJECT (returnBtn), "target_page", "main_menu_page");

  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createConfigPage (AppContext *ctx)
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

  g_object_set_data (G_OBJECT (returnBtn), "target_page", "main_menu_page");
  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick), ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createInfoPage (AppContext *ctx)
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
createGamePage (AppContext *ctx)
{
  GtkBuilder *builder = gtk_builder_new_from_resource ("/amazons/ui/game.ui");
  GtkWidget *box = GTK_WIDGET (gtk_builder_get_object (builder, "game_box"));

  g_object_set_data (G_OBJECT (ctx->mainStack), "game_box", box);

  GtkWidget *newGameBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_btn"));
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
  GtkWidget *boardGrid
      = GTK_WIDGET (gtk_builder_get_object (builder, "board_grid"));

  g_object_set_data (G_OBJECT (ctx->mainStack), "board_grid", boardGrid);

  ctx->gameState = NULL;
  ctx->gameBoard = NULL;

  setupBoard (ctx, boardGrid);

  GtkGesture *rightClick = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (rightClick),
                                 GDK_BUTTON_SECONDARY);
  g_signal_connect (rightClick, "pressed", G_CALLBACK (onRightClick), ctx);
  gtk_widget_add_controller (boardGrid, GTK_EVENT_CONTROLLER (rightClick));

  g_object_set_data (G_OBJECT (quitBtn), "target_page", "main_menu_page");

  g_signal_connect (quitBtn, "clicked", G_CALLBACK (navigationClick), ctx);
  g_signal_connect (newGameBtn, "clicked", G_CALLBACK (newGameButtonClick),
                    ctx);
  g_signal_connect (infoBtn, "clicked", G_CALLBACK (infoPopupButtonClick),
                    NULL);
  g_signal_connect (configBtn, "clicked", G_CALLBACK (configPopupButtonClick),
                    ctx);
  g_signal_connect (undoBtn, "clicked", G_CALLBACK (undoButtonClick), ctx);
  g_signal_connect (redoBtn, "clicked", G_CALLBACK (redoButtonClick), ctx);

  updateGameShortcuts (ctx);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

static void
navigationClick (GtkButton *button, gpointer user_data)
{
  AppContext *ctx = (AppContext *)user_data;
  const char *targetPage
      = (const char *)g_object_get_data (G_OBJECT (button), "target_page");

  if (ctx->mainStack != NULL && targetPage != NULL)
    {
      gtk_stack_set_visible_child_name (ctx->mainStack, targetPage);
    }
}

static void
quitButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  AppContext *ctx = (AppContext *)user_data;

  if (ctx && ctx->mainWindow)
    {
      gtk_window_close (GTK_WINDOW (ctx->mainWindow));
    }
}

static void
infoPopupButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  AppContext *ctx = (AppContext *)user_data;

  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/game_pages/info_popup.ui");
  GtkWindow *infoWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "info_window"));

  gtk_window_set_transient_for (infoWindow, GTK_WINDOW (ctx->mainWindow));
  gtk_window_present (infoWindow);

  g_object_unref (builder);
}

static void
configPopupButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  AppContext *ctx = (AppContext *)user_data;

  GtkBuilder *builder = gtk_builder_new_from_resource (
      "/amazons/ui/game_pages/config_popup.ui");
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

static void
undoButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  AppContext *ctx = (AppContext *)user_data;

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
    }
}

static void
redoButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  AppContext *ctx = (AppContext *)user_data;

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
    }
}

static void
setupBoard (AppContext *ctx, GtkWidget *boardGrid)
{
  int size = ctx->gameConfig->board_size;

  if (ctx->gameState)
    {
      game_cleanup (ctx->gameState);
      free (ctx->gameState);
      ctx->gameState = NULL;
    }
  if (ctx->gameBoard)
    {
      free (ctx->gameBoard);
      ctx->gameBoard = NULL;
    }

  GtkWidget *child = gtk_widget_get_first_child (boardGrid);
  while (child != NULL)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);
      gtk_grid_remove (GTK_GRID (boardGrid), child);
      child = next;
    }

  ctx->gameState = malloc (sizeof (GameState));
  ctx->gameState->size = size;
  game_init (ctx->gameState);

  ctx->gameBoard = malloc (size * size * sizeof (GtkWidget *));
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  for (unsigned int i = 0; i < size * size; i++)
    {
      unsigned int x = i % size;
      unsigned int y = i / size;

      GtkWidget *cellBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_widget_set_size_request (cellBox, 80, 80);

      GtkWidget *label = gtk_label_new ("");
      gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
      gtk_widget_set_hexpand (label, TRUE);
      gtk_widget_set_vexpand (label, TRUE);
      gtk_box_append (GTK_BOX (cellBox), label);

      g_object_set_data (G_OBJECT (cellBox), "cell_label", label);

      if ((x + y) % 2 == 0)
        gtk_widget_add_css_class (cellBox, "cell-light");
      else
        gtk_widget_add_css_class (cellBox, "cell-dark");

      ctx->gameBoard[i] = cellBox;

      g_object_set_data (G_OBJECT (cellBox), "cell_x", GINT_TO_POINTER (x));
      g_object_set_data (G_OBJECT (cellBox), "cell_y", GINT_TO_POINTER (y));
      g_object_set_data (G_OBJECT (cellBox), "cell_index",
                         GINT_TO_POINTER (i));

      GtkGesture *leftClick = gtk_gesture_click_new ();
      gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (leftClick),
                                     GDK_BUTTON_PRIMARY);
      g_signal_connect (leftClick, "pressed", G_CALLBACK (onCellClicked), ctx);
      gtk_widget_add_controller (cellBox, GTK_EVENT_CONTROLLER (leftClick));

      GtkDragSource *dragSource = gtk_drag_source_new ();
      g_signal_connect (dragSource, "prepare", G_CALLBACK (onDrag), ctx);
      g_signal_connect (dragSource, "drag-end", G_CALLBACK (onDragEnd), ctx);
      gtk_widget_add_controller (cellBox, GTK_EVENT_CONTROLLER (dragSource));

      GtkDropTarget *dropTarget
          = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
      g_signal_connect (dropTarget, "drop", G_CALLBACK (onDrop), ctx);
      gtk_widget_add_controller (cellBox, GTK_EVENT_CONTROLLER (dropTarget));

      gtk_grid_attach (GTK_GRID (boardGrid), cellBox, x, size - 1 - y, 1, 1);
    }

  drawBoard (ctx);
}

static void
newGameButtonClick (GtkButton *button, gpointer user_data)
{
  (void)button;
  AppContext *ctx = (AppContext *)user_data;

  GtkWidget *boardGrid = GTK_WIDGET (
      g_object_get_data (G_OBJECT (ctx->mainStack), "board_grid"));

  if (boardGrid)
    {
      setupBoard (ctx, boardGrid);
    }

  guiStartNewGame (ctx);
  gtk_stack_set_visible_child_name (ctx->mainStack, "game_page");
}

static GdkContentProvider *
onDrag (GtkDragSource *source, double x, double y, gpointer user_data)
{
  (void)x;
  (void)y;

  AppContext *ctx = (AppContext *)user_data;

  if (ctx->currentPhase != PHASE_MOVE_QUEEN)
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

static gboolean
onDrop (GtkDropTarget *target, const GValue *value, double x, double y,
        gpointer user_data)
{
  (void)x;
  (void)y;

  AppContext *ctx = (AppContext *)user_data;

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

static void
onDragEnd (GtkDragSource *source, GdkDrag *drag, gboolean delete_data,
           gpointer user_data)
{
  (void)source;
  (void)drag;
  (void)delete_data;
  AppContext *ctx = (AppContext *)user_data;

  if (ctx->currentPhase == PHASE_MOVE_QUEEN)
    {
      clearHighlights (ctx);
      drawBoard (ctx);
    }
}

static void
fireArrow (GtkWidget *button, gpointer user_data)
{
  AppContext *ctx = (AppContext *)user_data;

  if (ctx->currentPhase != PHASE_SHOOT_ARROW)
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

static void
onRightClick (GtkGestureClick *gesture, int n_press, double x, double y,
              gpointer user_data)
{
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
  AppContext *ctx = (AppContext *)user_data;

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

static void
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

static void
updateConfigVerbose (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((AppContext *)user_data)->gameConfig->verbose
      = gtk_switch_get_active (widget);
}

static void
updateConfigBlitz (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((AppContext *)user_data)->gameConfig->blitz_mode
      = gtk_switch_get_active (widget);
}

static void
updateConfigTimeout (GtkSpinButton *spin_button, gpointer user_data)
{
  ((AppContext *)user_data)->gameConfig->time_per_player
      = gtk_spin_button_get_value_as_int (spin_button);
}

static void
updateConfigBoardSize (GtkSpinButton *spin_button, gpointer user_data)
{
  ((AppContext *)user_data)->gameConfig->board_size
      = gtk_spin_button_get_value_as_int (spin_button);
}

static void
updateConfigAiWhite (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((AppContext *)user_data)->gameConfig->ai_white
      = gtk_switch_get_active (widget);
}

static void
updateConfigAiBlack (GtkSwitch *widget, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  ((AppContext *)user_data)->gameConfig->ai_black
      = gtk_switch_get_active (widget);
}

#if GTK_CHECK_VERSION(4, 10, 0)
static void
updateConfigLanguage (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void)pspec;
  guint active = gtk_drop_down_get_selected (GTK_DROP_DOWN (object));
  if (active == 0 || active == 1)
    {
      ((AppContext *)user_data)->gameConfig->language = (lang_t)active;
    }
}
#else
static void
updateConfigLanguage (GtkComboBoxText *combo_box, gpointer user_data)
{
  int active = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));
  if (active == 0 || active == 1)
    {
      ((AppContext *)user_data)->gameConfig->language = (lang_t)active;
    }
}
#endif

static gboolean
shortcutNewGame (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  newGameButtonClick (NULL, user_data);
  return TRUE;
}

static gboolean
shortcutInfo (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  infoPopupButtonClick (NULL, user_data);
  return TRUE;
}

static gboolean
shortcutConfig (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  configPopupButtonClick (NULL, user_data);
  return TRUE;
}

static gboolean
shortcutQuit (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  quitButtonClick (NULL, user_data);
  return TRUE;
}

static gboolean
shortcutUndo (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  undoButtonClick (NULL, user_data);
  return TRUE;
}

G_GNUC_UNUSED
static gboolean
shortcutRedo (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  redoButtonClick (NULL, user_data);
  return TRUE;
}

static gboolean
shortcutDummy (GtkWidget *widget, GVariant *args, gpointer user_data)
{
  (void)widget;
  (void)args;
  (void)user_data;
  g_print ("Not implemented.\n");
  return TRUE;
}

static void
updateGameShortcuts (AppContext *ctx)
{
  if (!ctx || !ctx->mainStack)
    return;

  GtkWidget *game_box
      = GTK_WIDGET (g_object_get_data (G_OBJECT (ctx->mainStack), "game_box"));
  if (!game_box)
    return;

  GtkEventController *old_ctrl = GTK_EVENT_CONTROLLER (
      g_object_get_data (G_OBJECT (game_box), "shortcut_controller"));
  if (old_ctrl)
    {
      gtk_widget_remove_controller (game_box, old_ctrl);
    }

  GtkShortcutController *shortcutController
      = GTK_SHORTCUT_CONTROLLER (gtk_shortcut_controller_new ());
  gtk_shortcut_controller_set_scope (shortcutController,
                                     GTK_SHORTCUT_SCOPE_MANAGED);

  struct
  {
    const char *key;
    GtkShortcutFunc func;
  } shortcuts[] = { { ctx->keyBindings.key_new_game, shortcutNewGame },
                    { ctx->keyBindings.key_config, shortcutConfig },
                    { ctx->keyBindings.key_info, shortcutInfo },
                    { ctx->keyBindings.key_quit, shortcutQuit },
                    { ctx->keyBindings.key_load_game, shortcutDummy },
                    { ctx->keyBindings.key_save_game, shortcutDummy },
                    { ctx->keyBindings.key_undo, shortcutUndo },
                    { ctx->keyBindings.key_redo, shortcutDummy },
                    { ctx->keyBindings.key_pause, shortcutDummy },
                    { ctx->keyBindings.key_hint, shortcutDummy } };

  for (int i = 0; i < SHORTCUTS_COUNT; i++)
    {
      if (shortcuts[i].key != NULL && shortcuts[i].key[0] != '\0')
        {
          GtkShortcutTrigger *trigger
              = gtk_shortcut_trigger_parse_string (shortcuts[i].key);
          if (trigger)
            {
              GtkShortcutAction *action
                  = gtk_callback_action_new (shortcuts[i].func, ctx, NULL);
              GtkShortcut *shortcut = gtk_shortcut_new (trigger, action);
              gtk_shortcut_controller_add_shortcut (shortcutController,
                                                    shortcut);
            }
        }
    }

  g_object_set_data (G_OBJECT (game_box), "shortcut_controller",
                     shortcutController);
  gtk_widget_add_controller (game_box,
                             GTK_EVENT_CONTROLLER (shortcutController));
}

static void
bindingButtonAnimate (GtkToggleButton *button, gpointer user_data)
{
  (void)user_data;
  if (gtk_toggle_button_get_active (button))
    {
      gtk_button_set_label (GTK_BUTTON (button), "Press...");
    }
  else
    {
      char *target_str
          = (char *)g_object_get_data (G_OBJECT (button), "target_str");
      if (target_str)
        gtk_button_set_label (GTK_BUTTON (button), target_str);
    }
}

static gboolean
bindingKeyPressed (GtkEventControllerKey *controller, guint keyval,
                   guint keycode, GdkModifierType state, gpointer user_data)
{
  (void)keycode;
  GtkWidget *button
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller));

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return FALSE;

  if (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R
      || keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R
      || keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R)
    return FALSE;

  state &= gtk_accelerator_get_default_mod_mask ();

  char *accel_name = gtk_accelerator_name (keyval, state);

  if (accel_name && accel_name[0] != '\0')
    {
      char *target_str
          = (char *)g_object_get_data (G_OBJECT (button), "target_str");
      if (target_str)
        {
          strncpy (target_str, accel_name, SHORTCUTS_SIZE - 1);
          target_str[SHORTCUTS_SIZE - 1] = '\0';
        }

      gtk_button_set_label (GTK_BUTTON (button), accel_name);
      g_free (accel_name);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  AppContext *ctx = (AppContext *)user_data;
  if (ctx)
    {
      updateGameShortcuts (ctx);
    }

  return TRUE;
}

static void
setupBindingButton (GtkWidget *btn, char *target_str, AppContext *ctx)
{
  if (!btn)
    return;

  gtk_button_set_label (GTK_BUTTON (btn), target_str);

  g_object_set_data (G_OBJECT (btn), "target_str", target_str);

  g_signal_connect (btn, "toggled", G_CALLBACK (bindingButtonAnimate), NULL);

  GtkEventController *key_ctrl = gtk_event_controller_key_new ();
  g_signal_connect (key_ctrl, "key-pressed", G_CALLBACK (bindingKeyPressed),
                    ctx);
  gtk_widget_add_controller (btn, key_ctrl);
}
