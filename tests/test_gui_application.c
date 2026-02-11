#include "gui_application.h"
#include <gtk/gtk.h>

int
main (int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = createApplication ();
  status = runApplication (app, argc, argv);

  return status;
}