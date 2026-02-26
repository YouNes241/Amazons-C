#include "gui_application.h"
#include "gui_pages.h"
#include <gtk/gtk.h>

static void initWindow (GtkApplication *app, gpointer userData);

GtkApplication *
createApplication (void)
{
  GtkApplication *app;

  app = gtk_application_new ("fr.bordeaux.amazons", G_APPLICATION_NON_UNIQUE);
  g_signal_connect (app, "activate", G_CALLBACK (initWindow), NULL);

  return app;
}

int
runApplication (GtkApplication *app, int argc, char **argv)
{
  int status;

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

static void
initWindow (GtkApplication *app, gpointer userData)
{
  (void)userData; // disable warning

  GtkCssProvider *cssProvider = gtk_css_provider_new ();
  gtk_css_provider_load_from_path (cssProvider, "ui/style.css");

  gtk_style_context_add_provider_for_display (
      gdk_display_get_default (), GTK_STYLE_PROVIDER (cssProvider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_object_unref (cssProvider);

  GtkWidget *window;
  GtkWidget *globalOverlay;
  GtkWidget *stack;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "PLACEHOLDER_GAME_NAME");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_window_maximize (GTK_WINDOW (window));

  globalOverlay = gtk_overlay_new ();

  stack = gtk_stack_new ();

  // Page transition animation (WIP)
  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 300);

  gtk_overlay_set_child (GTK_OVERLAY (globalOverlay), stack);

  GtkWidget *notifBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *notifLabel = gtk_label_new ("");

  gtk_box_append (GTK_BOX (notifBox), notifLabel);

  gtk_widget_set_valign (notifBox, GTK_ALIGN_START);
  gtk_widget_set_halign (notifBox, GTK_ALIGN_CENTER);

  gtk_widget_set_visible (notifBox, FALSE);
  gtk_overlay_add_overlay (GTK_OVERLAY (globalOverlay), notifBox);

  GtkWidget *mainMenuPage = createMainMenuPage ();
  GtkWidget *singleplayerMenuPage = createSingleplayerMenuPage ();
  GtkWidget *multiplayerMenuPage = createMultiplayerMenuPage ();
  GtkWidget *configPage = createConfigPage ();
  GtkWidget *infoPage = createInfoPage ();

  gtk_stack_add_named (GTK_STACK (stack), mainMenuPage, "page_main_menu");
  gtk_stack_add_named (GTK_STACK (stack), singleplayerMenuPage,
                       "page_singleplayer");
  gtk_stack_add_named (GTK_STACK (stack), multiplayerMenuPage,
                       "page_multiplayer");
  gtk_stack_add_named (GTK_STACK (stack), configPage, "page_config");
  gtk_stack_add_named (GTK_STACK (stack), infoPage, "page_info");

  gtk_window_set_child (GTK_WINDOW (window), globalOverlay);
  gtk_window_present (GTK_WINDOW (window));
}
