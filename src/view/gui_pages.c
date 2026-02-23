#include "gui_pages.h"
#include <gtk/gtk.h>

static void navigationClick (GtkButton *button, gpointer user_data);
static void quitButtonClick (GtkButton *button, gpointer user_data);

GtkWidget *
createMainMenuPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *btnSingleplayer;
  GtkWidget *btnMultiplayer;
  GtkWidget *btnConfig;
  GtkWidget *btnInfo;
  GtkWidget *btnQuit;

  builder = gtk_builder_new_from_file ("ui/main_menu.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "main_menu_box"));

  btnSingleplayer
      = GTK_WIDGET (gtk_builder_get_object (builder, "singleplayer_btn"));
  btnMultiplayer
      = GTK_WIDGET (gtk_builder_get_object (builder, "multiplayer_btn"));
  btnConfig = GTK_WIDGET (gtk_builder_get_object (builder, "config_btn"));
  btnInfo = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  btnQuit = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));

  g_signal_connect (btnSingleplayer, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "page_singleplayer");
  g_signal_connect (btnQuit, "clicked", G_CALLBACK (quitButtonClick), NULL);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

GtkWidget *
createSingleplayerMenuPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *btnNewGame;
  GtkWidget *btnLoadGame;
  GtkWidget *btnReturn;

  builder = gtk_builder_new_from_file ("ui/singleplayer.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "singleplayer_box"));

  btnNewGame = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_btn"));
  btnLoadGame = GTK_WIDGET (gtk_builder_get_object (builder, "load_game_btn"));
  btnReturn = GTK_WIDGET (gtk_builder_get_object (builder, "return_btn"));

  g_signal_connect (btnReturn, "clicked", G_CALLBACK (navigationClick),
                    (gpointer) "page_main_menu");

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

// GtkWidget *createInfoPage ();

static void
navigationClick (GtkButton *button, gpointer user_data)
{
  const char *target_page = (const char *)user_data;

  GtkWidget *stack
      = gtk_widget_get_ancestor (GTK_WIDGET (button), GTK_TYPE_STACK);

  if (stack != NULL && target_page != NULL)
    {
      gtk_stack_set_visible_child_name (GTK_STACK (stack), target_page);
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
