#include "bitboard.h"
#include "evaluator.h"
#include "minimax.h"

BitboardState
create_empty_state (uint8_t size)
{
  BitboardState state = { 0 };
  state.size = size;
  state.white_to_move = true;
  state.white_queens = bitboard_create_empty ();
  state.black_queens = bitboard_create_empty ();
  state.arrows = bitboard_create_empty ();
  return state;
}

void
bitboard_state_print (const BitboardState *state)
{
  uint8_t width = state->size;

  // Safety check for board size
  if (width < 4 || width > 11)
    width = 10;

  printf ("\n--- Amazons Game State ---\n");
  printf ("Turn: %s to move\n\n", state->white_to_move ? "White" : "Black");

  // Header: Column indices
  printf ("    ");
  for (int col = 0; col < width; col++)
    {
      printf ("%c ", 'a' + col);
    }
  printf ("\n");

  // Iterate through rows
  for (int row = 0; row < width; row++)
    {
      printf ("%2d: ", row);

      for (int col = 0; col < width; col++)
        {
          uint8_t index = row * width + col;

          // Priority 1: White Queens
          if (bitboard_get_bit (state->white_queens, index))
            {
              printf ("W ");
            }
          // Priority 2: Black Queens
          else if (bitboard_get_bit (state->black_queens, index))
            {
              printf ("B ");
            }
          // Priority 3: Arrows (Barricades)
          else if (bitboard_get_bit (state->arrows, index))
            {
              printf ("X ");
            }
          // Priority 4: Empty Square
          else
            {
              printf (". ");
            }
        }
      printf ("\n");
    }
  printf ("--------------------------\n\n");
}
