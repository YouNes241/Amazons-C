#ifndef GUI_PAGES_H
#define GUI_PAGES_H

#include "gui_utils.h"
#include <gtk/gtk.h>

GtkWidget *createMainMenuPage (GuiContext *ctx);
GtkWidget *createSingleplayerMenuPage (GuiContext *ctx);
GtkWidget *createMultiplayerMenuPage (GuiContext *ctx);
GtkWidget *createConfigPage (GuiContext *ctx);
GtkWidget *createInfoPage (GuiContext *ctx);
GtkWidget *createGamePage (GuiContext *ctx);
GtkWidget *createJoinServerPage (GuiContext *ctx);
GtkWidget *createClientPanelPage (GuiContext *ctx);

#endif // GUI_PAGES_H
