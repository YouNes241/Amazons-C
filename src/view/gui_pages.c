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
static void fireArrow (GtkButton *button, gpointer user_data);

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

      GtkWidget *cellBtn = gtk_button_new ();
      gtk_widget_set_size_request (cellBtn, 60, 60);

      if ((x + y) % 2 == 0)
        {
          gtk_widget_add_css_class (cellBtn, "cell-light");
        }
      else
        {
          gtk_widget_add_css_class (cellBtn, "cell-dark");
        }

      ctx->gameBoard[i] = cellBtn;

      g_object_set_data (G_OBJECT (cellBtn), "cell_x", GINT_TO_POINTER (x));
      g_object_set_data (G_OBJECT (cellBtn), "cell_y", GINT_TO_POINTER (y));
      g_object_set_data (G_OBJECT (cellBtn), "cell_index",
                         GINT_TO_POINTER (i));

      g_signal_connect (cellBtn, "clicked", G_CALLBACK (fireArrow), ctx);

      GtkDragSource *dragSource = gtk_drag_source_new ();
      g_signal_connect (dragSource, "prepare", G_CALLBACK (onDrag), ctx);
      gtk_widget_add_controller (cellBtn, GTK_EVENT_CONTROLLER (dragSource));

      GtkDropTarget *dropTarget
          = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
      g_signal_connect (dropTarget, "drop", G_CALLBACK (onDrop), ctx);
      gtk_widget_add_controller (cellBtn, GTK_EVENT_CONTROLLER (dropTarget));

      gtk_grid_attach (GTK_GRID (boardGrid), cellBtn, x, y, 1, 1);
    }

  drawBoard (ctx);

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

  GtkWidget *btn
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (source));

  int srcX = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_x"));
  int srcY = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_y"));
  int srcIndex
      = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_index"));

  CellContent piece = ctx->gameState->board[srcX][srcY];

  if (piece == CONTENT_EMPTY || piece == CONTENT_ARROW)
    return NULL;

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
  GtkWidget *btn
      = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (target));

  int destX = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_x"));
  int destY = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (btn), "cell_y"));

  const char *srcString = g_value_get_string (value);
  int srcIndex = atoi (srcString);
  int size = ctx->gameConfig->board_size;

  int srcX = srcIndex % size;
  int srcY = srcIndex / size;

  if (ctx->gameState->board[destX][destY] != CONTENT_EMPTY)
    return FALSE;

  CellContent currentPiece = ctx->gameState->board[srcX][srcY];
  ctx->gameState->board[srcX][srcY] = CONTENT_EMPTY;
  ctx->gameState->board[destX][destY] = currentPiece;

  drawPiece (ctx, srcX, srcY, CONTENT_EMPTY);
  drawPiece (ctx, destX, destY, currentPiece);

  g_print ("Moved from (%d, %d) to (%d, %d)\n", srcX, srcY, destX, destY);

  return TRUE;
}

static void
fireArrow (GtkButton *button, gpointer user_data)
{
  AppContext *ctx = (AppContext *)user_data;
  int x = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "cell_x"));
  int y = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "cell_y"));

  if (ctx->gameState->board[x][y] == CONTENT_EMPTY)
    {
      ctx->gameState->board[x][y] = CONTENT_ARROW;

      drawPiece (ctx, x, y, CONTENT_ARROW);

      g_print ("Fired arrow on (%d, %d)\n", x, y);
    }
  else
    {
      g_print ("Cannot fire an arrow on (%d, %d).\n", x, y);
    }
}