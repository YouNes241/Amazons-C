#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H
#include "gui_utils.h"

#define GUI_AI_DELAY 500
#define GUI_NETWORK_DELAY 33

void guiHandlePlayerMove (GuiContext *ctx, Move move);
void guiStartNewGame (GuiContext *ctx);
void guiDisconnectAndRestartNetwork (GuiContext *ctx);
void guiCheckAndTriggerAI (GuiContext *ctx);

#endif // GUI_CONTROLLER_H
