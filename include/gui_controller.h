#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H
#include "gui_utils.h"

void guiHandlePlayerMove (AppContext *ctx, Move move);
void guiStartNewGame (AppContext *ctx);
void guiDisconnectAndRestartNetwork (AppContext *ctx);

#endif // GUI_CONTROLLER_H
