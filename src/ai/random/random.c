#include "ai_interface.h"
#include "evaluator.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
random_init (AI_Interface *self, ConfigMap *params)
{
  printf ("[Random] Init OK\n");
}

static void
random_clean (AI_Interface *self)
{
  printf ("[Random] Clean OK\n");
}

/**
 * @brief Calculates the maximum number of moves possible in the current board
 * state.
 * @note It only calculates it once, as the size should not change.
 * (singleplayer)
 * @param state The current board state to evaluate.
 * @return Max number of legal moves possibles
 */
static int
get_move_limit (BitboardState state)
{
  static int possible_moves = -1;
  if (possible_moves == -1)
    {
      BitboardState temp_state = create_empty_state (state.size);
      uint8_t center_idx
          = (temp_state.size / 2) * temp_state.size + (temp_state.size / 2);
      bitboard_set_bit (&temp_state.white_queens, center_idx);
      possible_moves = get_legal_moves (temp_state, NULL)
                       * bitboard_count_bits (state.white_queens);
    }
  return possible_moves;
}

static Move
random_get_best_move (AI_Interface *self, GameState state)
{
  Move best_move = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
  BitboardState bs = sync_from_logic (&state);
  Move moves_array[get_move_limit (bs)];
  int count = get_legal_moves (bs, moves_array);
  for (int i = 0; i < count; i++)
    {
      printf ("Legal move %d: Queen from (%d,%d) to (%d,%d) with arrow to "
              "(%d,%d)\n",
              i + 1, moves_array[i].queen_src.row,
              moves_array[i].queen_src.col, moves_array[i].queen_dst.row,
              moves_array[i].queen_dst.col, moves_array[i].arrow_dst.row,
              moves_array[i].arrow_dst.col);
    }
  if (count > 0)
    {
      int random_index = rand () % count;
      best_move = moves_array[random_index];
    }
  return best_move;
}

const AI_VTable random_vtable = { .init = random_init,
                                  .clean = random_clean,
                                  .get_best_move = random_get_best_move };

void
create_random_ai (AI_Interface *ai)
{
  ai->vtable = &random_vtable;
  ai->context = NULL;
}