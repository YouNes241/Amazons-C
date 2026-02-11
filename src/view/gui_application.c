#include "gui_application.h"
#include <gtk/gtk.h>

static void initWindow (GtkApplication *app, gpointer userData);
static void changePage (GtkButton *button, gpointer userData);

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

  GtkWidget *window;
  GtkWidget *stack;

  // PLACEHOLDER PAGES
  GtkWidget *button1;
  GtkWidget *button2;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "PLACEHOLDER_GAME_NAME");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_window_maximize (GTK_WINDOW (window));

  stack = gtk_stack_new ();

  // Page transition animation (WIP)
  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 300);

  // PLACEHOLDER PAGES (test pages switch) //
  button1 = gtk_button_new_with_label ("TEST PAGE 1");
  button2 = gtk_button_new_with_label ("TEST PAGE 2");

  gtk_stack_add_named (GTK_STACK (stack), button1, "page_1");
  gtk_stack_add_named (GTK_STACK (stack), button2, "page_2");

  g_signal_connect (button1, "clicked", G_CALLBACK (changePage), stack);
  g_signal_connect (button2, "clicked", G_CALLBACK (changePage), stack);
  // ------------------------------------ //

  gtk_window_set_child (GTK_WINDOW (window), stack);
  gtk_window_present (GTK_WINDOW (window));
}

static void
changePage (GtkButton *button, gpointer userData)
{
  (void)button; // disable warning

  GtkStack *stack = GTK_STACK (userData);

  const char *currentPage = gtk_stack_get_visible_child_name (stack);

  if (g_strcmp0 (currentPage, "page_1") == 0)
    {
      gtk_stack_set_visible_child_name (stack, "page_2");
    }
  else
    {
      gtk_stack_set_visible_child_name (stack, "page_1");
    }
}
