#include "gui_utils.h"
#include <gtk/gtk.h>

GtkWidget *gameBoard[GRID_WIDTH * GRID_HEIGHT];

void
drawPiece (unsigned int x, unsigned int y, PieceType pieceType)
{
  GtkWidget *cellBtn = gameBoard[x + y * GRID_WIDTH];

  gtk_widget_remove_css_class (cellBtn, "piece-white");
  gtk_widget_remove_css_class (cellBtn, "piece-black");

  switch (pieceType)
    {
    case PIECE_WHITE:
      gtk_button_set_label (GTK_BUTTON (cellBtn), "♕");
      gtk_widget_add_css_class (cellBtn, "piece-white");
      break;
    case PIECE_BLACK:
      gtk_button_set_label (GTK_BUTTON (cellBtn), "♛");
      gtk_widget_add_css_class (cellBtn, "piece-black");
      break;
    case PIECE_ARROW:
      gtk_button_set_label (GTK_BUTTON (cellBtn), "🔥");
      break;
    case PIECE_EMPTY:
      gtk_button_set_label (GTK_BUTTON (cellBtn), "");
      break;
    }
}

PieceType
getPiece (unsigned int x, unsigned int y)
{
  GtkWidget *cellBtn = gameBoard[x + y * GRID_WIDTH];

  const char *label = gtk_button_get_label (GTK_BUTTON (cellBtn));

  if (label == NULL || g_strcmp0 (label, "") == 0)
    {
      return PIECE_EMPTY;
    }
  else if (g_strcmp0 (label, "♕") == 0)
    {
      return PIECE_WHITE;
    }
  else if (g_strcmp0 (label, "♛") == 0)
    {
      return PIECE_BLACK;
    }
  else if (g_strcmp0 (label, "🔥") == 0)
    {
      return PIECE_ARROW;
    }

  return PIECE_EMPTY;
}