#include "../include/cli_view.h"
#include <stdio.h>

void
cli_display_board (const GameState *state)
{
  if (!state)
    return;

  printf ("\n   ");
  // Affichage des lettres des colonnes
  for (int col = 0; col < state->size; col++)
    {
      printf (" %c ", 'a' + col);
    }
  printf ("\n");

  // Affichage des lignes
  for (int row = state->size - 1; row >= 0; row--)
    {
      printf ("%2d ", row + 1); // Numéro de ligne

      for (int col = 0; col < state->size; col++)
        {
          CellContent content = state->board[row][col];

          switch (content)
            {
            case CONTENT_WHITE_QUEEN:
              printf ("[W]");
              break;
            case CONTENT_BLACK_QUEEN:
              printf ("[B]");
              break;
            case CONTENT_ARROW:
              printf (" X ");
              break;
            case CONTENT_EMPTY:
              printf (" . ");
              break;
            default:
              printf (" ? ");
              break;
            }
        }
      printf (" %d\n", row + 1);
    }

  printf ("   ");
  for (int col = 0; col < state->size; col++)
    {
      printf (" %c ", 'a' + col);
    }
  printf ("\n\n");
}

void
cli_clear_screen ()
{
  printf ("\033[H\033[J");
}