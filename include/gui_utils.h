#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include "config.h"
#include "game_types.h"
#include "network_common.h"
#include <gtk/gtk.h>

#define SHORTCUTS_SIZE 16
#define SHORTCUTS_COUNT 10

typedef enum
{
  PHASE_MOVE_QUEEN,
  PHASE_SHOOT_ARROW
} GuiPhase;

typedef struct
{
  char key_new_game[SHORTCUTS_SIZE];
  char key_load_game[SHORTCUTS_SIZE];
  char key_save_game[SHORTCUTS_SIZE];
  char key_config[SHORTCUTS_SIZE];
  char key_info[SHORTCUTS_SIZE];
  char key_quit[SHORTCUTS_SIZE];
  char key_undo[SHORTCUTS_SIZE];
  char key_redo[SHORTCUTS_SIZE];
  char key_pause[SHORTCUTS_SIZE];
  char key_hint[SHORTCUTS_SIZE];
} KeyBindings;

typedef struct
{
  game_config_t *gameConfig;
  GameState *gameState;
  KeyBindings keyBindings;
  GtkWindow *mainWindow;
  GtkStack *mainStack;
  GtkWidget **gameBoard;
  GuiPhase currentPhase;
  Position tmpQueenSrc;
  Position tmpQueenDst;
  NetworkManager *networkManager;
  bool isNetworkMatch;
  PlayerColor selfPlayerColor;
} AppContext;

AppContext *initAppContext (game_config_t *config);
void freeAppContext (AppContext *ctx);
void drawPiece (AppContext *ctx, int x, int y, CellContent content);
void drawBoard (AppContext *ctx);
void clearHighlights (AppContext *ctx);
void highlightReachableSquares (AppContext *ctx, int srcX, int srcY);
void setupBoard (AppContext *ctx, GtkWidget *boardGrid);
void setupBindingButton (GtkWidget *btn, char *target_str, AppContext *ctx);
void updateGameShortcuts (AppContext *ctx);
void updateTurnDisplay (AppContext *ctx);

#endif // GUI_UTILS_H
