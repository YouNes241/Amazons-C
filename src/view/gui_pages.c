#include "gui_pages.h"
#include <gtk/gtk.h>

static void navigationClick (GtkButton *button, gpointer user_data);
static void quitButtonClick (GtkButton *button, gpointer user_data);
static void infoButtonClick (GtkButton *button, gpointer user_data);

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

  builder = gtk_builder_new_from_file ("ui/game.ui");
  box = GTK_WIDGET (gtk_builder_get_object (builder, "game_box"));

  infoBtn = GTK_WIDGET (gtk_builder_get_object (builder, "info_btn"));
  quitBtn = GTK_WIDGET (gtk_builder_get_object (builder, "quit_btn"));

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
