#ifndef GUI_APPLICATION_H
#define GUI_APPLICATION_H

#include "config.h"
#include <gtk/gtk.h>

GtkApplication *createApplication (game_config_t *config);
int runApplication (GtkApplication *app, int argc, char **argv);

#endif // GUI_APPLICATION_H