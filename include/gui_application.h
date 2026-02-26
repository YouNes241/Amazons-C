#ifndef GUI_APPLICATION_H
#define GUI_APPLICATION_H

#include <gtk/gtk.h>

GtkApplication *createApplication (void);
int runApplication (GtkApplication *app, int argc, char **argv);

#endif // GUI_APPLICATION_H