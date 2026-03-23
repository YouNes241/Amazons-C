#include "bitboard_state.h"
#include "evaluator.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

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

/**
 * @brief Applies a move on a BitboardState and returns the resulting state.
 * @param state The current board state before the move.
 * @param move The move to apply, containing source and destination for the
 * queen and the arrow.
 * @return A new BitboardState with the move applied.
 */
BitboardState
apply_move_bitboard (BitboardState *state, Move move)
{
  // Init
  BitboardState next = *state;
  Position q_src = move.queen_src;
  Position q_dst = move.queen_dst;
  Position a_dst = move.arrow_dst;
  int size = state->size;

  // Convert pos for bits
  uint8_t src_idx = q_src.row * size + q_src.col;
  uint8_t dst_idx = q_dst.row * size + q_dst.col;
  uint8_t arr_idx = a_dst.row * size + a_dst.col;

  // Who's moving ?
  Bitboard *my_queens
      = (state->white_to_move) ? &next.white_queens : &next.black_queens;

  // Remove
  bitboard_clear_bit (my_queens, src_idx);

  // Place
  bitboard_set_bit (my_queens, dst_idx);
  bitboard_set_bit (&next.arrows, arr_idx);

  next.white_to_move = !state->white_to_move;

  return next;
}

/**
 * @brief Calculates the maximum number of moves possible in the current board
 * state.
 * @param state The current board state to evaluate.
 * @return Max number of legal moves possibles
 */
int
get_move_limit (BitboardState *state)
{
  int k = bitboard_count_bits (state->white_queens);
  int reach = (state->size - 1) << 2;
  return k * reach * reach;
}

Bitboard
bitboard_state_get_occupied_squares (BitboardState *state)
{
  Bitboard occupied = bitboard_or (state->white_queens, state->black_queens);
  occupied = bitboard_or (occupied, state->arrows);
  return occupied;
}

Bitboard
bitboard_state_get_empty_squares (BitboardState *state)
{
  Bitboard occupied = bitboard_state_get_occupied_squares (state);
  return bitboard_masked_not (occupied, state->size);
}

Bitboard
bitboard_state_get_current_queens (BitboardState *state)
{
  return (state->white_to_move) ? state->white_queens : state->black_queens;
}

/**
 * @brief Checks if the game is over by determining if there are no legal moves
 * available for the current player.
 * @param state The current board state to evaluate.
 * @return true if the game is over (no legal moves), false otherwise.
 */
bool
bitboard_state_is_game_over (BitboardState *state)
{
  // Get the current queens
  Bitboard my_queens = bitboard_state_get_current_queens (state);

  // Get the empty squares
  Bitboard empty = bitboard_state_get_empty_squares (state);

  // Move 1 movement around, from our queens and using the empty tiles
  Bitboard adjacent_moves = expand_king_step (my_queens, empty, state->size);

  // Check if even a single move like this exist
  return bitboard_is_zero (adjacent_moves);
}

int
get_legal_moves (BitboardState *state, Move *moves_array)
{
  int count = 0;
  int size = state->size;
  Bitboard occupied = bitboard_state_get_occupied_squares (state);
  Bitboard my_queens = bitboard_state_get_current_queens (state);

  while (bitboard_count_bits (my_queens) != 0)
    {
      int q_from = pop_lsb (&my_queens);

      Bitboard queen_destinations
          = bitboard_get_reachable (q_from, occupied, size);

      while (bitboard_count_bits (queen_destinations) != 0)
        {
          int q_to = pop_lsb (&queen_destinations);

          Bitboard arrow_occupied = occupied;
          bitboard_clear_bit (&arrow_occupied, q_from);
          bitboard_set_bit (&arrow_occupied, q_to);

          Bitboard arrow_destinations
              = bitboard_get_reachable (q_to, arrow_occupied, size);

          while (bitboard_count_bits (arrow_destinations) != 0)
            {
              int arrow_to = pop_lsb (&arrow_destinations);

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

Move
get_direct_random_move (BitboardState *state)
{
  uint8_t size = (uint8_t)state->size;
  Bitboard occupied = bitboard_state_get_occupied_squares (state);
  Bitboard my_queens = bitboard_state_get_current_queens (state);
  int q_count = bitboard_count_bits (my_queens);

  while (q_count != 0)
    {
      uint32_t target_q = (uint32_t)random_uint ((uint32_t)q_count);

      Bitboard temp_q_bits = my_queens;
      for (uint32_t i = 0; i < target_q; i++)
        {
          pop_lsb (&temp_q_bits);
        }
      uint8_t q_from = (uint8_t)pop_lsb (&temp_q_bits);

      Bitboard q_dests = bitboard_get_reachable (q_from, occupied, size);
      int q_dest_count = bitboard_count_bits (q_dests);

      if (q_dest_count > 0)
        {
          uint32_t target_qd = (uint32_t)random_uint ((uint32_t)q_dest_count);
          for (uint32_t i = 0; i < target_qd; i++)
            {
              pop_lsb (&q_dests);
            }
          uint8_t q_to = (uint8_t)pop_lsb (&q_dests);

          Bitboard arrow_occ = occupied;
          bitboard_clear_bit (&arrow_occ, q_from);
          bitboard_set_bit (&arrow_occ, q_to);

          Bitboard a_dests = bitboard_get_reachable (q_to, arrow_occ, size);
          int a_dest_count = bitboard_count_bits (a_dests);

          uint32_t target_ad = (uint32_t)random_uint ((uint32_t)a_dest_count);
          for (uint32_t i = 0; i < target_ad; i++)
            {
              pop_lsb (&a_dests);
            }
          uint8_t arrow_to = (uint8_t)pop_lsb (&a_dests);

          return (Move){ .queen_src = idx_to_pos (q_from, size),
                         .queen_dst = idx_to_pos (q_to, size),
                         .arrow_dst = idx_to_pos (arrow_to, size) };
        }

      bitboard_clear_bit (&my_queens, q_from);
      q_count = bitboard_count_bits (my_queens);
    }

  return (Move){ 0 }; // Truly no moves for ANY queen
}