#include "gui_pages.h"
#include <gtk/gtk.h>

static void quitButtonClick (GtkButton *button, gpointer user_data);

GtkWidget *
createMainMenuPage ()
{
  GtkBuilder *builder;
  GtkWidget *box;
  GtkWidget *btn_singleplayer;
  GtkWidget *btn_multiplayer;
  GtkWidget *btn_config;
  GtkWidget *btn_info;
  GtkWidget *btn_quit;

  builder = gtk_builder_new_from_file ("ui/main_menu.ui");

  box = GTK_WIDGET (gtk_builder_get_object (builder, "main_menu_box"));
  btn_singleplayer
      = GTK_WIDGET (gtk_builder_get_object (builder, "singleplayer_btn"));
  btn_multiplayer
      = GTK_WIDGET (gtk_builder_get_object (builder, "multiplayer_btn"));
  btn_config = GTK_WIDGET (gtk_builder_get_object (builder, "config_btn"));
  btn_info = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  btn_quit = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));

  g_signal_connect (btn_quit, "clicked", G_CALLBACK (quitButtonClick), NULL);

  g_object_ref (box);
  g_object_unref (builder);

  return box;
}

// GtkWidget *createSingleplayerMenuPage ();
// GtkWidget *createInfoPage ();

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