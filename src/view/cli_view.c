#include "cli_view.h"
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

void
cli_display_main_menu (void)
{
  cli_clear_screen ();

  printf ("========================================================\n");
  printf ("        ___                                             \n");
  printf ("       /   |  ____ ___  ____ _____  ____  ____  ____    \n");
  printf ("      / /| | / __ `__ \\/ __ `/_  / / __ \\/ __ \\/ ___/ \n");
  printf ("     / ___ |/ / / / / / /_/ / / /_/ /_/ / / / (__  )    \n");
  printf ("    /_/  |_/_/ /_/ /_/\\__,_/ /___/\\____/_/ /_/____/   \n");
  printf ("                                                        \n");
  printf ("========================================================\n");
  printf ("\n");
  printf (_ ("  --- Local Game ---\n"));
  printf (_ ("  new         : Start a new local game\n"));
  printf (_ ("  load [filename]      : Load a saved game\n"));
  printf ("\n");
  printf (_ ("  --- Multiplayer ---\n"));
  printf (_ ("  server start [port]        : Host a new server\n"));
  printf (_ ("  join [IP:Port] [username]  : Connect to a server\n"));
  printf ("\n");
  printf (_ ("  --- Playing a Move ---\n"));
  printf (
      _ ("  Format: [QueenStart]-[QueenEnd]([ArrowEnd])  Ex: d10-d9(d8)\n"));
  printf ("\n");
  printf (_ ("  --- Other ---\n"));
  printf (_ ("  help    Show help\n"));
  printf (_ ("  quit    Quit the game\n"));
  printf ("\n");
  printf ("========================================================\n");
}