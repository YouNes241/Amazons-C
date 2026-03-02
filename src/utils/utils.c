#include "utils.h"
#include "bitboard.h"
#include "evaluator.h"
#include "minimax.h"
#include <stddef.h>
#include <stdio.h>

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

          if (bitboard_get_bit (state->white_queens, index))
            {
              printf ("W ");
            }
          else if (bitboard_get_bit (state->black_queens, index))
            {
              printf ("B ");
            }
          else if (bitboard_get_bit (state->arrows, index))
            {
              printf ("X ");
            }
          else
            {
              printf (". ");
            }
        }
      printf ("\n");
    }
  printf ("--------------------------\n\n");
}

BitboardState
sync_from_logic (GameState *gs)
{
  BitboardState bs = { 0 };
  bs.size = gs->size;
  bs.white_to_move = (gs->current_player == PLAYER_WHITE);
  for (uint8_t r = 0; r < gs->size; r++)
    {
      for (uint8_t c = 0; c < gs->size; c++)
        {
          uint8_t idx = r * gs->size + c;
          switch (gs->board[r][c])
            {
            case CONTENT_WHITE_QUEEN:
              bitboard_set_bit (&bs.white_queens, idx);
              break;
            case CONTENT_BLACK_QUEEN:
              bitboard_set_bit (&bs.black_queens, idx);
              break;
            case CONTENT_ARROW:
              bitboard_set_bit (&bs.arrows, idx);
              break;
            default:
              break;
            }
        }
    }
  return bs;
}

Bitboard // CHECK
get_reachable (uint8_t start_index, Bitboard occupied, uint8_t size)
{
  Bitboard reachable = bitboard_create_empty ();
  uint8_t start_r = start_index / size;
  uint8_t start_c = start_index % size;

  for (int8_t dr = -1; dr <= 1; dr++)
    for (int8_t dc = -1; dc <= 1; dc++)
      {
        // origin
        if (dr == 0 && dc == 0)
          continue;

        // any movement
        for (int dist = 1; dist < size; dist++)
          {
            int8_t r = start_r + (dr * dist);
            int8_t c = start_c + (dc * dist);

            if (r < 0 || r >= size || c < 0 || c >= size)
              break;

            uint8_t current_idx = r * size + c;

            // If we hit an obstacle, stop.
            if (bitboard_get_bit (occupied, current_idx))
              break;

            bitboard_set_bit (&reachable, current_idx);
          }
      }
  return reachable;
}

Position
idx_to_pos (uint8_t index, uint8_t size)
{
  return (Position){ .row = index / size, .col = index % size };
}

int
get_queen_moves (uint8_t queen_index, Bitboard occupied, uint8_t size,
                 Move *moves_array)
{
  Bitboard reachable = get_reachable (queen_index, occupied, size);
  int count = 0;

  while (!bitboard_is_zero (reachable))
    {
      int idx = pop_lsb (&reachable);
      moves_array[count++]
          = (Move){ .queen_src = idx_to_pos (queen_index, size),
                    .queen_dst = idx_to_pos (idx, size),
                    .arrow_dst = 0 };
    }
  return count;
}

int
get_arrow_moves (Move *queen_moves, Move *final_moves, int queen_move_count,
                 Bitboard occupied, uint8_t size)
{
  int count = 0;

  for (int i = 0; i < queen_move_count; i++)
    {
      Move qm = queen_moves[i];
      uint8_t idx = qm.queen_dst.row * size + qm.queen_dst.col;

      Bitboard new_occupied = occupied;
      bitboard_clear_bit (&new_occupied,
                          qm.queen_src.row * size + qm.queen_src.col);
      bitboard_set_bit (&new_occupied, idx);

      Bitboard reachable = get_reachable (idx, new_occupied, size);

      while (!bitboard_is_zero (reachable))
        {
          int arrow_idx = pop_lsb (&reachable);
          final_moves[count++]
              = (Move){ .queen_src = qm.queen_src,
                        .queen_dst = qm.queen_dst,
                        .arrow_dst = idx_to_pos (arrow_idx, size) };
        }
    }
  return count;
}

int // To optimize | CHECK
get_legal_moves (BitboardState state, Move *moves_array)
{
  int count = 0;
  int size = state.size;
  Bitboard occupied = bitboard_or (
      bitboard_or (state.white_queens, state.black_queens), state.arrows);
  Bitboard my_queens
      = state.white_to_move ? state.white_queens : state.black_queens;

  for (uint8_t q_from = 0; q_from < (size * size); q_from++)
    {
      if (!bitboard_get_bit (my_queens, q_from))
        continue; // TODO : optimize

      Bitboard queen_destinations = get_reachable (q_from, occupied, size);

      for (uint8_t q_to = 0; q_to < (size * size); q_to++)
        {
          if (!bitboard_get_bit (queen_destinations, q_to))
            continue;

          Bitboard arrow_occupied = occupied;
          bitboard_clear_bit (&arrow_occupied, q_from);
          bitboard_set_bit (&arrow_occupied, q_to);

          Bitboard arrow_destinations
              = get_reachable (q_to, arrow_occupied, size);

          for (uint8_t arrow_to = 0; arrow_to < (size * size); arrow_to++)
            {
              if (!bitboard_get_bit (arrow_destinations, arrow_to))
                continue;

              if (moves_array != NULL)
                {
                  moves_array[count]
                      = (Move){ .queen_src = idx_to_pos (q_from, size),
                                .queen_dst = idx_to_pos (q_to, size),
                                .arrow_dst = idx_to_pos (arrow_to, size) };
                }
              count++;
            }
        }
    }
  return count;
}