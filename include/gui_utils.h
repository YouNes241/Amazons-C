#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include "config.h"
#include "game_types.h"
#include <gtk/gtk.h>

typedef enum
{
  PHASE_MOVE_QUEEN,
  PHASE_SHOOT_ARROW
} GuiPhase;

typedef struct
{
  game_config_t *gameConfig;
  GameState *gameState;
  GtkWindow *mainWindow;
  GtkStack *mainStack;
  GtkWidget **gameBoard;
  GuiPhase currentPhase;
  Position tmpQueenSrc;
  Position tmpQueenDst;
} AppContext;

AppContext *initAppContext (game_config_t *config);
void freeAppContext (AppContext *ctx);
void drawPiece (AppContext *ctx, int x, int y, CellContent content);
void drawBoard (AppContext *ctx);
void clearHighlights (AppContext *ctx);
void highlightReachableSquares (AppContext *ctx, int srcX, int srcY);

#endif // GUI_UTILS_H