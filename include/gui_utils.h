#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#define GRID_WIDTH 10
#define GRID_HEIGHT 10

#include <gtk/gtk.h>

typedef enum
{
  PIECE_EMPTY,
  PIECE_WHITE,
  PIECE_BLACK,
  PIECE_ARROW
} PieceType;

extern GtkWidget *gameBoard[GRID_WIDTH * GRID_HEIGHT];

void drawPiece (unsigned int x, unsigned int y, PieceType pieceType);
PieceType getPiece (unsigned int x, unsigned int y);

#endif // GUI_UTILS_H