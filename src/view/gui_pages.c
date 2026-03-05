#include "gui_pages.h"
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
createMainMenuPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *singleplayerBtn;
  GtkWidget *multiplayerBtn;
  GtkWidget *configBtn;
  GtkWidget *infoBtn;
  GtkWidget *quitBtn;

  builder = gtk_builder_new_from_file ("ui/main_menu.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "main_menu_box"));

  singleplayerBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "singleplayer_btn"));
  multiplayerBtn
      = GTK_WIDGET (gtk_builder_get_object (builder, "multiplayer_btn"));
  configBtn = GTK_WIDGET (gtk_builder_get_object (builder, "config_btn"));
  infoBtn = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  quitBtn = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));

  g_signal_connect (singleplayerBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "singleplayer_page");
  g_signal_connect (multiplayerBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "multiplayer_page");
  g_signal_connect (configBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "config_page");
  g_signal_connect (infoBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "info_page");
  g_signal_connect (quitBtn, "clicked", G_CALLBACK (quitButtonClick), NULL);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createSingleplayerMenuPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *newGameBtn;
  GtkWidget *returnBtn;

  builder = gtk_builder_new_from_file ("ui/singleplayer.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "singleplayer_box"));

  newGameBtn = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_btn"));
  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_signal_connect (newGameBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "game_page");
  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "main_menu_page");

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createMultiplayerMenuPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *returnBtn;

  builder = gtk_builder_new_from_file ("ui/multiplayer.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "multiplayer_box"));

  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "main_menu_page");

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createConfigPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *returnBtn;

  builder = gtk_builder_new_from_file ("ui/config.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "config_box"));

  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "main_menu_page");

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createInfoPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *returnBtn;

  builder = gtk_builder_new_from_file ("ui/info.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "info_box"));

  returnBtn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_signal_connect (returnBtn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "main_menu_page");

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createGamePage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *infoBtn;
  GtkWidget *quitBtn;
  GtkWidget *boardGrid;

  builder = gtk_builder_new_from_file ("ui/game.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "game_box"));

  infoBtn = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  quitBtn = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));
  boardGrid = GTK_WIDGET (gtk_builder_get_object (builder, "board_grid"));

  for (unsigned int i = 0; i < GRID_HEIGHT * GRID_WIDTH; i++)
    {
      unsigned int x = i % GRID_WIDTH;
      unsigned int y = i / GRID_WIDTH;

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

      gameBoard[i] = cellBtn;

      g_signal_connect (cellBtn, "clicked", G_CALLBACK (fireArrow),
                        GINT_TO_POINTER (i));

      GtkDragSource *dragSource = gtk_drag_source_new ();
      g_signal_connect (dragSource, "prepare", G_CALLBACK (onDrag),
                        GINT_TO_POINTER (i));
      gtk_widget_add_controller (cellBtn, GTK_EVENT_CONTROLLER (dragSource));

      GtkDropTarget *dropTarget
          = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);
      g_signal_connect (dropTarget, "drop", G_CALLBACK (onDrop),
                        GINT_TO_POINTER (i));
      gtk_widget_add_controller (cellBtn, GTK_EVENT_CONTROLLER (dropTarget));

      gtk_grid_attach (GTK_GRID (boardGrid), cellBtn, x, y, 1, 1);
    }

  drawPiece (0, 6, PIECE_WHITE);
  drawPiece (3, 9, PIECE_WHITE);
  drawPiece (6, 9, PIECE_WHITE);
  drawPiece (9, 6, PIECE_WHITE);

  drawPiece (0, 3, PIECE_BLACK);
  drawPiece (3, 0, PIECE_BLACK);
  drawPiece (6, 0, PIECE_BLACK);
  drawPiece (9, 3, PIECE_BLACK);

  g_signal_connect (quitBtn, "clicked", G_CALLBACK (quitButtonClick), NULL);
  g_signal_connect (infoBtn, "clicked", G_CALLBACK (infoButtonClick), NULL);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

static void
navigationClick (GtkButton *button, gpointer user_data)
{
  const char *targetPage = (const char *)user_data;

  GtkWidget *stack
      = gtk_widget_get_ancestor (GTK_WIDGET (button), GTK_TYPE_STACK);

  if (stack != NULL && targetPage != NULL)
    {
      gtk_stack_set_visible_child_name (GTK_STACK (stack), targetPage);
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
      = gtk_builder_new_from_file ("ui/game_pages/info_popup.ui");
  GtkWindow *infoWindow
      = GTK_WINDOW (gtk_builder_get_object (builder, "info_window"));

  gtk_window_set_transient_for (infoWindow, mainWindow);

  gtk_window_present (infoWindow);

  g_object_unref (builder);
}

static GdkContentProvider *
onDrag (GtkDragSource *source, double x, double y, gpointer user_data)
{
  (void)source;
  (void)x;
  (void)y;

  int srcIndex = GPOINTER_TO_INT (user_data);
  int srcX = srcIndex % GRID_WIDTH;
  int srcY = srcIndex / GRID_WIDTH;

  if (getPiece (srcX, srcY) == PIECE_EMPTY)
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
  (void)target;
  (void)x;
  (void)y;

  int destIndex = GPOINTER_TO_INT (user_data);
  const char *srcString = g_value_get_string (value);
  int srcIndex = atoi (srcString);

  int srcX = srcIndex % GRID_WIDTH;
  int srcY = srcIndex / GRID_WIDTH;
  int destX = destIndex % GRID_WIDTH;
  int destY = destIndex / GRID_WIDTH;

  if (getPiece (destX, destY) != PIECE_EMPTY)
    return FALSE;

  PieceType currentPiece = getPiece (srcX, srcY);
  drawPiece (srcX, srcY, PIECE_EMPTY);
  drawPiece (destX, destY, currentPiece);

  g_print ("Move from (%d, %d) to (%d, %d)\n", srcX, srcY, destX, destY);

  return TRUE;
}

static void
fireArrow (GtkButton *button, gpointer user_data)
{
  (void)button;

  int index = GPOINTER_TO_INT (user_data);
  int x = index % GRID_WIDTH;
  int y = index / GRID_WIDTH;

  if (getPiece (x, y) == PIECE_EMPTY)
    {
      drawPiece (x, y, PIECE_ARROW);
      g_print ("Arrow fired on (%d, %d)\n", x, y);
    }
}