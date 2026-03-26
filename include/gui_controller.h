#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H
#include "gui_utils.h"

#define GUI_AI_DELAY 500
#define GUI_NETWORK_DELAY 33

void guiHandlePlayerMove (AppContext *ctx, Move move);
void guiStartNewGame (AppContext *ctx);
void guiDisconnectAndRestartNetwork (AppContext *ctx);

#endif // GUI_CONTROLLER_H
