/**
 * @file gui_utils.c
 * @brief Implementation of GTK GUI utility functions.
 *
 * @details This file implements the core visual manipulation and state
 * management functions for the GTK interface. It handles dynamic board
 * generation, piece rendering via CSS classes, drag-and-drop highlight
 * calculations, and custom keyboard shortcut recording mechanisms.
 */

#include "gui_utils.h"
#include "config.h"
#include "game_engine.h"
#include "game_types.h"
#include "gui_callbacks.h"
#include <assert.h>
#include <gtk/gtk.h>

/* Forward declarations for static keyboard shortcut handlers */
static void bindingButtonAnimate (GtkToggleButton *button, gpointer user_data);
static gboolean bindingKeyPressed (GtkEventControllerKey *controller,
                                   guint keyval, guint keycode,
                                   GdkModifierType state, gpointer user_data);

/**
 * @brief Allocates and initializes a new GUI context.
 *
 * @details Creates the context structure, attaches the provided game
 * configuration, initializes pointers to NULL, and sets up default keyboard
 * shortcuts.
 *
 * @param config Pointer to the global game configuration.
 * @return A dynamically allocated pointer to the initialized `GuiContext`.
 */
GuiContext *
initGuiContext (game_config_t *config)
{
  assert (config != NULL);

  GuiContext *ctx = malloc (sizeof (GuiContext));
  assert (ctx != NULL);

  ctx->gameConfig = config;
  ctx->gameState = NULL;
  ctx->mainWindow = NULL;
  ctx->mainStack = NULL;
  ctx->gameBoard = NULL;
  ctx->networkManager = NULL;
  ctx->currentPhase = PHASE_MOVE_QUEEN;
  ctx->isNetworkMatch = false;
  ctx->isHintActive = false;
  ctx->is_ai_white_initialized = false;
  ctx->is_ai_black_initialized = false;
  ctx->is_ai_thinking = false;

  memset (&ctx->keyBindings, 0, sizeof (ctx->keyBindings));
  strncpy (ctx->keyBindings.key_new_game, "<Control>n", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_load_game, "<Control>l", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_save_game, "<Control>s", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_config, "<Control>comma", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_info, "<Control>i", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_quit, "<Control>q", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_undo, "<Control>u", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_redo, "<Control>r", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_pause, "<Control>p", SHORTCUTS_SIZE - 1);
  strncpy (ctx->keyBindings.key_hint, "<Control>h", SHORTCUTS_SIZE - 1);

  return ctx;
}

/**
 * @brief Frees the memory allocated for the GUI context and its resources.
 * @param ctx Pointer to the `GuiContext` to destroy.
 */
void
freeGuiContext (GuiContext *ctx)
{
  assert (ctx != NULL);

  if (ctx->gameBoard)
    free (ctx->gameBoard);
  if (ctx->gameState)
    {
      game_cleanup (ctx->gameState);
      free (ctx->gameState);
    }
  free (ctx);
}

/**
 * @brief Updates the visual representation of a single cell on the board.
 *
 * @details Removes previous content CSS classes from the target cell and
 * applies the new class and unicode character representing the specified
 * entity.
 *
 * @param ctx Pointer to the `GuiContext`.
 * @param x The X coordinate (column) of the cell.
 * @param y The Y coordinate (row) of the cell.
 * @param content The entity to draw (Empty, White Queen, Black Queen, or
 * Arrow).
 */
void
drawPiece (GuiContext *ctx, int x, int y, CellContent content)
{
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameBoard != NULL);

  int size = ctx->gameState->size;
  assert (x >= 0 && x < size);
  assert (y >= 0 && y < size);

  int index = y * size + x;

  GtkWidget *cellBox = ctx->gameBoard[index];
  assert (cellBox != NULL);

  GtkWidget *label
      = GTK_WIDGET (g_object_get_data (G_OBJECT (cellBox), "cell_label"));
  assert (label != NULL);

  gtk_widget_remove_css_class (cellBox, "piece-white");
  gtk_widget_remove_css_class (cellBox, "piece-black");
  gtk_widget_remove_css_class (cellBox, "piece-arrow");
  gtk_label_set_text (GTK_LABEL (label), "");

  switch (content)
    {
    case CONTENT_WHITE_QUEEN:
      gtk_label_set_text (GTK_LABEL (label), "♛");
      gtk_widget_add_css_class (cellBox, "piece-white");
      break;

    case CONTENT_BLACK_QUEEN:
      gtk_label_set_text (GTK_LABEL (label), "♛");
      gtk_widget_add_css_class (cellBox, "piece-black");
      break;

    case CONTENT_ARROW:
      gtk_label_set_text (GTK_LABEL (label), "🔥");
      gtk_widget_add_css_class (cellBox, "piece-arrow");
      break;

    case CONTENT_EMPTY:
      break;
    }
}

/**
 * @brief Completely redraws the visual board based on the internal
 * `GameState`.
 * @param ctx Pointer to the `GuiContext`.
 */
void
drawBoard (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameBoard != NULL);

  int size = ctx->gameState->size;

  for (int y = 0; y < size; y++)
    {
      for (int x = 0; x < size; x++)
        {
          drawPiece (ctx, x, y, ctx->gameState->board[y][x]);
        }
    }
}

/**
 * @brief Clears all valid-move highlights from the board UI.
 * @details Removes the "highlight-valid" CSS class from all cells and resets
 * the internal "is_legal_move" UI flag to 0.
 * @param ctx Pointer to the `GuiContext`.
 */
void
clearHighlights (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameBoard != NULL);

  int size = ctx->gameState->size;
  for (int i = 0; i < size * size; i++)
    {
      gtk_widget_remove_css_class (ctx->gameBoard[i], "highlight-valid");
      g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "is_legal_move",
                         GINT_TO_POINTER (0));
    }
}

/**
 * @brief Clears AI hint highlights from the board UI.
 * @param ctx Pointer to the `GuiContext`.
 */
void
clearHintHighlights (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameBoard != NULL);

  int size = ctx->gameState->size;
  for (int i = 0; i < size * size; i++)
    {
      gtk_widget_remove_css_class (ctx->gameBoard[i], "hint-queen");
      gtk_widget_remove_css_class (ctx->gameBoard[i], "hint-arrow");
    }
}

/**
 * @brief Calculates and highlights all legal destination squares for a
 * specific piece.
 *
 * @details Converts the board state into Bitboards to quickly calculate all
 * reachable squares in 8 directions using the game engine logic. Valid squares
 * are flagged in the GTK widgets to authorize drop/click actions.
 *
 * @param ctx Pointer to the `GuiContext`.
 * @param srcX The X coordinate (column) of the selected piece.
 * @param srcY The Y coordinate (row) of the selected piece.
 */
void
highlightReachableSquares (GuiContext *ctx, int srcX, int srcY)
{
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameBoard != NULL);

  int size = ctx->gameState->size;
  assert (srcX >= 0 && srcX < size);
  assert (srcY >= 0 && srcY < size);

  clearHighlights (ctx);

  uint8_t start_index = coords_to_index (srcX, srcY, size);

  BitboardState bb = sync_from_logic (ctx->gameState);

  Bitboard obstacles = bitboard_or (bb.white_queens,
                                    bitboard_or (bb.black_queens, bb.arrows));
  // The moving piece itself is not an obstacle
  bitboard_clear_bit (&obstacles, start_index);

  Bitboard reachable = get_reachable_squares (obstacles, start_index, size);

  for (int i = 0; i < size * size; i++)
    {
      if (bitboard_get_bit (reachable, i))
        {
          gtk_widget_add_css_class (ctx->gameBoard[i], "highlight-valid");
          g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "is_legal_move",
                             GINT_TO_POINTER (1));
        }
    }

  gtk_widget_add_css_class (ctx->gameBoard[start_index], "active-queen");
}

/**
 * @brief Dynamically builds the physical GTK widgets for the game board grid.
 *
 * @details Clears any existing UI grid, instantiates a new internal GameState,
 * allocates individual cell boxes (GtkBox) with labels, and attaches
 * drag-and-drop and click gesture controllers to each cell. Handles dynamic
 * board inversion for network matches (Black plays at the bottom).
 *
 * @param ctx Pointer to the `GuiContext`.
 * @param boardGrid Pointer to the `GtkGrid` container that will hold the
 * cells.
 */
void
setupBoard (GuiContext *ctx, GtkWidget *boardGrid)
{
  assert (ctx != NULL);
  assert (ctx->gameConfig != NULL);
  assert (boardGrid != NULL);

  int size = ctx->gameConfig->board_size;
  assert (size > 0);

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

  // Clear existing grid children
  GtkWidget *child = gtk_widget_get_first_child (boardGrid);
  while (child != NULL)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);
      gtk_grid_remove (GTK_GRID (boardGrid), child);
      child = next;
    }

  ctx->gameState = malloc (sizeof (GameState));
  assert (ctx->gameState != NULL);
  ctx->gameState->size = size;
  game_init (ctx->gameState);

  ctx->gameBoard = malloc (size * size * sizeof (GtkWidget *));
  assert (ctx->gameBoard != NULL);
  ctx->currentPhase = PHASE_MOVE_QUEEN;

  bool flipBoard = false;

  // Board inversion logic: The player's pieces should be at the bottom
  if (ctx->isNetworkMatch)
    {
      flipBoard = (ctx->selfPlayerColor == PLAYER_BLACK);
    }
  else
    {
      flipBoard = (ctx->gameConfig->ai_white_cfg.enabled
                   && !ctx->gameConfig->ai_black_cfg.enabled);
    }

  for (unsigned int i = 0; i < size * size; i++)
    {
      unsigned int logicalX = i % size;
      unsigned int logicalY = i / size;

      GtkWidget *cellBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_widget_set_size_request (cellBox, 80, 80);

      GtkWidget *label = gtk_label_new ("");
      gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
      gtk_widget_set_hexpand (label, TRUE);
      gtk_widget_set_vexpand (label, TRUE);
      gtk_box_append (GTK_BOX (cellBox), label);

      g_object_set_data (G_OBJECT (cellBox), "cell_label", label);

      // Checkerboard pattern styling
      if ((logicalX + logicalY) % 2 == 0)
        gtk_widget_add_css_class (cellBox, "cell-light");
      else
        gtk_widget_add_css_class (cellBox, "cell-dark");

      ctx->gameBoard[i] = cellBox;

      g_object_set_data (G_OBJECT (cellBox), "cell_x",
                         GINT_TO_POINTER (logicalX));
      g_object_set_data (G_OBJECT (cellBox), "cell_y",
                         GINT_TO_POINTER (logicalY));
      g_object_set_data (G_OBJECT (cellBox), "cell_index",
                         GINT_TO_POINTER (i));

      // Click gestures for Arrow Shooting
      GtkGesture *leftClick = gtk_gesture_click_new ();
      gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (leftClick),
                                     GDK_BUTTON_PRIMARY);
      g_signal_connect (leftClick, "pressed", G_CALLBACK (onCellClicked), ctx);
      gtk_widget_add_controller (cellBox, GTK_EVENT_CONTROLLER (leftClick));

      // Drag Source for Queen Moving
      GtkDragSource *dragSource = gtk_drag_source_new ();
      g_signal_connect (dragSource, "prepare", G_CALLBACK (onDrag), ctx);
      g_signal_connect (dragSource, "drag-end", G_CALLBACK (onDragEnd), ctx);
      gtk_widget_add_controller (cellBox, GTK_EVENT_CONTROLLER (dragSource));

      // Drop Target for Queen Landing
      GtkDropTarget *dropTarget
          = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
      g_signal_connect (dropTarget, "drop", G_CALLBACK (onDrop), ctx);
      gtk_widget_add_controller (cellBox, GTK_EVENT_CONTROLLER (dropTarget));

      // Map logical coordinates to physical grid positions
      int visualX = logicalX;
      int visualY = size - 1 - logicalY;

      if (flipBoard)
        {
          visualX = size - 1 - logicalX;
          visualY = logicalY;
        }

      gtk_grid_attach (GTK_GRID (boardGrid), cellBox, visualX, visualY, 1, 1);
    }

  drawBoard (ctx);
  updateTurnDisplay (ctx);
}

/**
 * @brief Binds a key-capture listener to a toggle button for shortcut
 * configuration.
 *
 * @param btn The `GtkToggleButton` to convert into a shortcut recorder.
 * @param target_str The string buffer where the captured shortcut will be
 * saved.
 * @param ctx Pointer to the `GuiContext`.
 */
void
setupBindingButton (GtkWidget *btn, char *target_str, GuiContext *ctx)
{
  assert (btn != NULL);
  assert (target_str != NULL);
  assert (ctx != NULL);

  gtk_button_set_label (GTK_BUTTON (btn), target_str);

  g_object_set_data (G_OBJECT (btn), "target_str", target_str);

  g_signal_connect (btn, "toggled", G_CALLBACK (bindingButtonAnimate), NULL);

  GtkEventController *key_ctrl = gtk_event_controller_key_new ();
  g_signal_connect (key_ctrl, "key-pressed", G_CALLBACK (bindingKeyPressed),
                    ctx);
  gtk_widget_add_controller (btn, key_ctrl);
}

/**
 * @brief Callback to visually update a shortcut binding button when toggled.
 * @param button The toggle button being interacted with.
 * @param user_data Unused context parameter.
 */
static void
bindingButtonAnimate (GtkToggleButton *button, gpointer user_data)
{
  (void)user_data;
  assert (button != NULL);

  if (gtk_toggle_button_get_active (button))
    {
      gtk_button_set_label (GTK_BUTTON (button), "Press...");
    }
  else
    {
      char *target_str
          = (char *)g_object_get_data (G_OBJECT (button), "target_str");
      assert (target_str != NULL);
      gtk_button_set_label (GTK_BUTTON (button), target_str);
    }
}

/**
 * @brief Callback intercepting a key press while a shortcut button is active.
 *
 * @details Captures the keyboard stroke, converts it into a GTK accelerator
 * string (e.g., "<Control>n"), saves it to the config, and deactivates the
 * button.
 *
 * @param controller The key event controller.
 * @param keyval The pressed key value.
 * @param keycode The raw hardware keycode.
 * @param state Modifiers (Ctrl, Shift, Alt).
 * @param user_data Pointer to the GuiContext.
 * @return TRUE if the key was intercepted, FALSE otherwise.
 */
static gboolean
bindingKeyPressed (GtkEventControllerKey *controller, guint keyval,
                   guint keycode, GdkModifierType state, gpointer user_data)
{
  (void)keycode;

  assert (controller != NULL);

  GtkWidget *button
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller));
  assert (button != NULL);

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return FALSE;

  // Ignore bare modifier presses, we want the actual key trigger
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
      assert (target_str != NULL);

      snprintf (target_str, SHORTCUTS_SIZE, "%s", accel_name);
      gtk_button_set_label (GTK_BUTTON (button), accel_name);
    }

  if (accel_name)
    {
      g_free (accel_name);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  GuiContext *ctx = (GuiContext *)user_data;
  assert (ctx != NULL);

  updateGameShortcuts (ctx);

  return TRUE;
}

/**
 * @brief Refreshes the application's global shortcut controllers based on
 * current bindings.
 * @param ctx Pointer to the `GuiContext`.
 */
void
updateGameShortcuts (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->mainStack != NULL);

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
                    { ctx->keyBindings.key_load_game, shortcutLoadGame },
                    { ctx->keyBindings.key_save_game, shortcutSaveGame },
                    { ctx->keyBindings.key_undo, shortcutUndo },
                    { ctx->keyBindings.key_redo, shortcutRedo },
                    { ctx->keyBindings.key_pause, shortcutDummy },
                    { ctx->keyBindings.key_hint, shortcutHint } };

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

/**
 * @brief Updates the UI label indicating whose turn it is.
 * @param ctx Pointer to the `GuiContext`.
 */
void
updateTurnDisplay (GuiContext *ctx)
{
  assert (ctx != NULL);
  assert (ctx->mainStack != NULL);
  assert (ctx->gameState != NULL);

  GtkWidget *turnLabel = GTK_WIDGET (
      g_object_get_data (G_OBJECT (ctx->mainStack), "turn_label"));
  assert (turnLabel != NULL);

  PlayerColor current = ctx->gameState->current_player;

  if (ctx->isNetworkMatch)
    {
      if (current == ctx->selfPlayerColor)
        gtk_label_set_text (GTK_LABEL (turnLabel), "It is your turn !");
      else
        {
          char *opponentTurnLabel = g_strdup_printf (
              "%s is playing...",
              (current == PLAYER_WHITE) ? "White" : "Black");
          gtk_label_set_text (GTK_LABEL (turnLabel), opponentTurnLabel);
          g_free (opponentTurnLabel);
        }
    }
  else
    {
      if (current == PLAYER_WHITE)
        gtk_label_set_text (GTK_LABEL (turnLabel), "Turn : White");
      else
        gtk_label_set_text (GTK_LABEL (turnLabel), "Turn : Black");
    }
}

/**
 * @brief Visually displays an AI-suggested move (Hint) on the board.
 * @param ctx Pointer to the `GuiContext`.
 * @param q_src The recommended origin position of the Queen.
 * @param q_dst The recommended destination position of the Queen.
 * @param a_dst The recommended target position for the Arrow.
 */
void
showHintOnBoard (GuiContext *ctx, Position q_src, Position q_dst,
                 Position a_dst)
{
  assert (ctx != NULL);
  assert (ctx->gameState != NULL);
  assert (ctx->gameBoard != NULL);

  int size = ctx->gameState->size;

  assert (q_src.col >= 0 && q_src.col < size);
  assert (q_src.row >= 0 && q_src.row < size);
  assert (q_dst.col >= 0 && q_dst.col < size);
  assert (q_dst.row >= 0 && q_dst.row < size);
  assert (a_dst.col >= 0 && a_dst.col < size);
  assert (a_dst.row >= 0 && a_dst.row < size);

  clearHighlights (ctx);

  int src_idx = q_src.row * size + q_src.col;
  int dst_idx = q_dst.row * size + q_dst.col;
  int arr_idx = a_dst.row * size + a_dst.col;

  gtk_widget_add_css_class (ctx->gameBoard[src_idx], "hint-queen");
  gtk_widget_add_css_class (ctx->gameBoard[dst_idx], "hint-queen");
  gtk_widget_add_css_class (ctx->gameBoard[arr_idx], "hint-arrow");
}

/**
 * @brief Logs a formatted message to the built-in server console view.
 * @param ctx Pointer to the `GuiContext`.
 * @param format Printf-style format string.
 * @param ... Additional arguments for the format string.
 */
void
gui_log_to_server_console (GuiContext *ctx, const char *format, ...)
{
  assert (ctx != NULL);
  assert (ctx->mainStack != NULL);
  assert (format != NULL);

  GtkWidget *consoleView = GTK_WIDGET (
      g_object_get_data (G_OBJECT (ctx->mainStack), "server_console_view"));
  if (!consoleView)
    return;

  char buffer[1024];
  va_list args;
  va_start (args, format);
  int len = vsnprintf (buffer, sizeof (buffer), format, args);
  va_end (args);

  if (len < 0 || len >= (int)sizeof (buffer))
    {
      buffer[sizeof (buffer) - 1] = '\0';
    }

  GtkTextBuffer *text_buf
      = gtk_text_view_get_buffer (GTK_TEXT_VIEW (consoleView));
  GtkTextIter end;
  gtk_text_buffer_get_end_iter (text_buf, &end);
  gtk_text_buffer_insert (text_buf, &end, buffer, -1);
  gtk_text_buffer_insert (text_buf, &end, "\n", -1);

  GtkTextMark *mark
      = gtk_text_buffer_create_mark (text_buf, NULL, &end, FALSE);
  gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (consoleView), mark);
  gtk_text_buffer_delete_mark (text_buf, mark);
}

/**
 * @brief Displays a non-blocking modal popup message to the user.
 * @param ctx Pointer to the `GuiContext`.
 * @param message The text to display in the popup.
 */
void
guiShowPopup (GuiContext *ctx, const char *message)
{
  assert (ctx != NULL);
  assert (ctx->mainWindow != NULL);
  assert (message != NULL);

#if GTK_CHECK_VERSION(4, 10, 0)
  GtkAlertDialog *dialog = gtk_alert_dialog_new ("%s", message);
  gtk_alert_dialog_show (dialog, GTK_WINDOW (ctx->mainWindow));
  g_object_unref (dialog);
#else
  GtkWidget *dialog = gtk_message_dialog_new (
      GTK_WINDOW (ctx->mainWindow),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO,
      GTK_BUTTONS_OK, "%s", message);
  g_signal_connect_swapped (dialog, "response",
                            G_CALLBACK (gtk_window_destroy), dialog);
  gtk_window_present (GTK_WINDOW (dialog));
#endif
}