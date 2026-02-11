#ifndef GUI_APPLICATION_H
#define GUI_APPLICATION_H

#include <gtk/gtk.h>

GtkApplication *createApplication (void);
int runApplication (GtkApplication *app, int argc, char **argv);
static void initWindow (GtkApplication *app, gpointer userData);

#endif // GUI_APPLICATION_H