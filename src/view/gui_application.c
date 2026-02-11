#include "gui_application.h"
#include <gtk/gtk.h>

GtkApplication *
createApplication (void)
{
  GtkApplication *app;

  app = gtk_application_new ("fr.bordeaux.amazons",
                             G_APPLICATION_DEFAULT_FLAGS);
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
  GtkWidget *window;
  GtkWidget *stack;

  // PLACEHOLDER PAGES
  GtkWidget *page1;
  GtkWidget *page2;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "PLACEHOLDER_GAME_NAME");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

  stack = gtk_stack_new ();

  // Page transition animation (WIP)
  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 300);

  page1 = gtk_label_new ("TEST PAGE 1");
  page2 = gtk_label_new ("TEST PAGE 2");

  gtk_window_set_child (GTK_WINDOW (window), stack);

  gtk_window_present (GTK_WINDOW (window));
}