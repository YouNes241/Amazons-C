#ifndef GUI_PAGES_H
#define GUI_PAGES_H

#include "gui_utils.h"
#include <gtk/gtk.h>

GtkWidget *createMainMenuPage (AppContext *ctx);
GtkWidget *createSingleplayerMenuPage (AppContext *ctx);
GtkWidget *createMultiplayerMenuPage (AppContext *ctx);
GtkWidget *createConfigPage (AppContext *ctx);
GtkWidget *createInfoPage (AppContext *ctx);
GtkWidget *createGamePage (AppContext *ctx);

#endif // GUI_PAGES_H