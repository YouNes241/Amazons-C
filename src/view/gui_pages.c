#include "gui_pages.h"
#include "game_engine.h"
#include "gui_utils.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

static void navigationClick (GtkButton *button, gpointer user_data);
static void quitButtonClick (GtkButton *button, gpointer user_data);
static void infoButtonClick (GtkButton *button, gpointer user_data);
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
  g_signal_connect (quitBtn, "clicked", G_CALLBACK (quitButtonClick), NULL);

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

  g_object_set_data (G_OBJECT (newGameBtn), "target_page", "game_page");
  g_object_set_data (G_OBJECT (returnBtn), "target_page", "main_menu_page");

  g_signal_connect (newGameBtn, "clicked", G_CALLBACK (navigationClick), ctx);
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

  builder = gtk_builder_new_from_resource ("/amazons/ui/config.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "config_box"));

  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

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
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *infoBtn;
  GtkWidget *quitBtn;
  GtkWidget *boardGrid;

  builder = gtk_builder_new_from_resource ("/amazons/ui/game.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "game_box"));

  infoBtn = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  quitBtn = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));
  boardGrid = GTK_WIDGET (gtk_builder_get_object (builder, "board_grid"));

  int size = ctx->gameConfig->board_size;
  ctx->gameState = malloc (sizeof (GameState));
  ctx->gameState->size = size;
  game_init (ctx->gameState);

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

  GtkGesture *rightClick = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (rightClick),
                                 GDK_BUTTON_SECONDARY);
  g_signal_connect (rightClick, "pressed", G_CALLBACK (onRightClick), ctx);
  gtk_widget_add_controller (boardGrid, GTK_EVENT_CONTROLLER (rightClick));

  g_signal_connect (quitBtn, "clicked", G_CALLBACK (quitButtonClick), NULL);
  g_signal_connect (infoBtn, "clicked", G_CALLBACK (infoButtonClick), NULL);

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
  (void)user_data;

  GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (button));

  if (GTK_IS_WINDOW (root))
    {
      gtk_window_close (GTK_WINDOW (root));
    }
}

static void
infoButtonClick (GtkButton *button, gpointer user_data)
{
  (void)user_data;

  GtkWindow *mainWindow
      = GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (button)));

  GtkBuilder *builder
      = gtk_builder_new_from_resource ("/amazons/ui/game_pages/info_popup.ui");
  GtkWindow *infoWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "info_window"));

  gtk_window_set_transient_for (infoWindow, mainWindow);

  gtk_window_present (infoWindow);

  g_object_unref (builder);
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

  int cases_trouvees = 0;
  for (int i = 0;
       i < ctx->gameConfig->board_size * ctx->gameConfig->board_size; i++)
    {
      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (ctx->gameBoard[i]),
                                              "is_legal_move"))
          == 1)
        {
          cases_trouvees++;
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
  int size = ctx->gameConfig->board_size;

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

  if (is_move_legal (ctx->gameState, current_move))
    {
      apply_move (ctx->gameState, current_move);

      clearHighlights (ctx);
      drawBoard (ctx);

      ctx->currentPhase = PHASE_MOVE_QUEEN;
      g_print ("Arrow fired on (%d, %d).\n", targetX, targetY);

      if (is_game_over (ctx->gameState, ctx->gameState->current_player))
        {
          PlayerColor winner = get_winner (ctx->gameState);
          g_print ("GAME OVER! Winner is: %d\n", winner);
        }
    }
  else
    {
      clearHighlights (ctx);
      drawBoard (ctx);
      ctx->currentPhase = PHASE_MOVE_QUEEN;
      g_print ("Error: Engine rejected the move.\n");
    }
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