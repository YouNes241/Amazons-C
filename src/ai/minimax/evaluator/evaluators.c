#include "bitboard.h"
#include "evaluator.h"
#include "game_types.h"
#include "utils.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INF_DIST 255

static Bitboard BOARD_MASKS[12];
static bool masks_initialized = false;

/**
 * @brief Pre-compute masks for valid squares based on board size.
 *
 * Initializes board masks used to correct uninitialized squares.
 * The masks are pre-computed to avoid repeated calculations.
 *
 * @return void
 */
static void
init_board_masks ()
{
  for (uint8_t s = 4; s <= 11; s++)
    {
      int total_squares = s * s;
      Bitboard m = { 0, 0 };

      if (total_squares <= 64)
        {
          m.low = (total_squares == 64) ? ~0ULL : (1ULL << total_squares) - 1;
          m.high = 0;
        }
      else
        {
          m.low = ~0ULL;
          m.high = (1ULL << (total_squares - 64)) - 1;
        }
      BOARD_MASKS[s] = m;
    }
  masks_initialized = true;
}

/**
 * @brief Retrieves the pre-computed board mask for a given board size.
 *
 * The mask filters out-of-bounds squares for boards sized 4 to 11.
 * Automatically initializes masks on the first call if necessary.
 *
 * @param size The size of the board (4 to 11).
 * @return A Bitboard with bits set for valid squares on the board of the given
 * size.
 */
static Bitboard
get_board_mask (uint8_t size)
{
  if (!masks_initialized)
    {
      init_board_masks ();
    }

  uint8_t limit = (size > 11) ? 10 : (size < 4 ? 4 : size);
  return BOARD_MASKS[limit];
}

/**
 * @brief Puts all occupied squares (queens and arrows) into a single bitboard.
 * @param state A game state with pos of queens and arrows.
 * @return Bitboard with all occupied squares.
 */
static Bitboard
get_obstacles (const BitboardState *state)
{
  Bitboard occ = bitboard_or (state->white_queens, state->black_queens);
  return bitboard_or (occ, state->arrows);
}

/**
 * @brief Generates all squares reachable by a "King" step from the 'sources'
 * bitboard.
 *
 * @param sources Bitboard with the starting positions (e.g., queens).
 * @param empty Bitboard with all empty squares (not occupied).
 * @param width The width of the board.
 * @return A Bitboard representing all valid reachable squares.
 */
Bitboard
expand_king_step (Bitboard sources, Bitboard empty, uint8_t width)
{
  Bitboard result = bitboard_create_empty ();

  result = bitboard_or (result, bitboard_shift_right (sources, width)); // N
  result = bitboard_or (result, bitboard_shift_left (sources, width));  // S

  // Masks to remove edges for horizontal shifts
  // TODO : optimize by pre-computing
  Bitboard mask_not_a = bitboard_create_full ();
  Bitboard mask_not_h = bitboard_create_full ();
  for (int r = 0; r < width; r++)
    {
      bitboard_clear_bit (&mask_not_a, r * width); // Clear Col 0
      bitboard_clear_bit (&mask_not_h,
                          r * width + (width - 1)); // Clear Col (width-1)
    }

  Bitboard west_moves = bitboard_and (sources, mask_not_a);
  Bitboard east_moves = bitboard_and (sources, mask_not_h);

  result = bitboard_or (result, bitboard_shift_right (west_moves, 1)); // W
  result = bitboard_or (result,
                        bitboard_shift_right (west_moves, width + 1)); // NW
  result = bitboard_or (result,
                        bitboard_shift_left (west_moves, width - 1)); // SW
  result = bitboard_or (result, bitboard_shift_left (east_moves, 1)); // E
  result = bitboard_or (result,
                        bitboard_shift_right (east_moves, width - 1)); // NE
  result = bitboard_or (result,
                        bitboard_shift_left (east_moves, width + 1)); // SE

  return bitboard_and (result, empty);
}

/**
 * @brief Generates all squares reachable by "Queen" slides from the 'sources'
 * bitboard.
 *
 * @param sources Bitboard with the starting positions (e.g., queens).
 * @param empty Bitboard with all empty squares (not occupied).
 * @param width The width of the board.
 * @return A Bitboard representing all valid reachable squares.
 */
static Bitboard
expand_queen_slide (Bitboard sources, Bitboard empty, uint8_t width)
{
  Bitboard attacks = bitboard_create_empty ();
  Bitboard prop;

  // Pre-compute masks for the specific board width
  Bitboard mask_not_a = bitboard_create_full ();
  Bitboard mask_not_h = bitboard_create_full ();
  for (int r = 0; r < width; r++)
    {
      bitboard_clear_bit (&mask_not_a, r * width);
      bitboard_clear_bit (&mask_not_h, r * width + (width - 1));
    }

  // TODO : optimize
  // 1: North - Vertical expansion
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_shift_right (prop, width);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  // 2: South - Vertical expansion
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_shift_left (prop, width);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  // 3: East - Horizontal expansion (mask last col)
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_and (prop, mask_not_h);
      prop = bitboard_shift_left (prop, 1);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  // 4: West - Horizontal expansion (mask first col)
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_and (prop, mask_not_a);
      prop = bitboard_shift_right (prop, 1);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  // 5: North-East (mask last col)
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_and (prop, mask_not_h);
      prop = bitboard_shift_right (prop, width - 1);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  // 6: North-West (mask first col)
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_and (prop, mask_not_a);
      prop = bitboard_shift_right (prop, width + 1);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  // 7: South-East (mask last col)
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_and (prop, mask_not_h);
      prop = bitboard_shift_left (prop, width + 1);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  // 8: South-West (mask first col)
  prop = sources;
  for (int i = 0; i < width; i++)
    {
      prop = bitboard_and (prop, mask_not_a);
      prop = bitboard_shift_left (prop, width - 1);
      prop = bitboard_and (prop, empty);
      if (bitboard_is_zero (prop))
        break;
      attacks = bitboard_or (attacks, prop);
    }

  return attacks;
}

/**
 * @brief Populates a distance map for every square on the board from 'starts'.
 * @param starts Bitboard with the starting positions (e.g., queens).
 * @param obstacles Bitboard with all occupied squares (queens and arrows).
 * @param width The width of the board.
 * @param mode 0 for Queen Distance (RTH), 1 for King Distance (Chebyshev).
 * @param dist_map Output array, of size at least width*width. Unreachable
 * squares are set to INF_DIST (255).
 * @return void
 */
static void
bfs_bitboard (Bitboard starts, Bitboard obstacles, uint8_t width, int mode,
              uint8_t *dist_map)
{
  // Initialize map
  for (int i = 0; i < width * width; i++)
    dist_map[i] = INF_DIST;

  // Mark starting positions
  Bitboard visited = starts;
  Bitboard frontier = starts;

  // Set distance 0 for starts
  Bitboard temp = starts;
  while (!bitboard_is_zero (temp))
    {
      int idx = pop_lsb (&temp);
      dist_map[idx] = 0;
    }

  Bitboard empty_squares
      = bitboard_and (bitboard_not (obstacles), get_board_mask (width));
  int dist = 0;

  while (!bitboard_is_zero (frontier))
    {
      dist++;
      if (dist >= INF_DIST)
        break;

      Bitboard next_frontier;

      if (mode == 0)
        {
          // Queen Slide Expansion
          next_frontier = expand_queen_slide (frontier, empty_squares, width);
        }
      else
        {
          // King Step Expansion
          next_frontier = expand_king_step (frontier, empty_squares, width);
        }

      // Only keep unvisited
      Bitboard not_visited = bitboard_not (visited);
      next_frontier = bitboard_and (next_frontier, get_board_mask (width));
      next_frontier = bitboard_and (next_frontier, not_visited);

      // Update map
      temp = next_frontier;
      while (!bitboard_is_zero (temp))
        {
          int idx = pop_lsb (&temp);
          dist_map[idx] = dist;
        }

      // Update visited
      visited = bitboard_or (visited, next_frontier);
      frontier = next_frontier;
    }
}

/**
 * @brief Calculates the total mobility (number of legal moves) for the given
 * queens.
 * @param queens Bitboard with the positions of the player's queens.
 * @param empty Bitboard with all empty squares (not occupied).
 * @param size The width of the board.
 * @return The total count of legal moves available to these player's queens.
 */
static int
calculate_mobility_sum (Bitboard queens, Bitboard empty, uint8_t size)
{
  int total = 0;
  while (!bitboard_is_zero (queens))
    {
      int idx = pop_lsb (&queens);
      Bitboard single = bitboard_create_empty ();
      bitboard_set_bit (&single, idx);

      Bitboard moves = expand_queen_slide (single, empty, size);
      total += bitboard_count_bits (moves);
    }
  return total;
}

/**
 * @brief Evaluates the board state based on mobility.
 * @note This heuristic counts the total number of legal moves available to the
 * current player and subtracts the opponent's mobility.
 * @param state The current board state to evaluate.
 * @return A float score representing the mobility advantage. Positive means
 * the current player has more mobility, negative means less.
 */
static float
eval_me (BitboardState *state)
{
  Bitboard mask = get_board_mask (state->size);
  Bitboard empty = bitboard_and (bitboard_not (get_obstacles (state)), mask);

  int white = calculate_mobility_sum (state->white_queens, empty, state->size);
  int black = calculate_mobility_sum (state->black_queens, empty, state->size);

  return (float)(white - black);
}

/**
 * @brief Evaluates the board state based on the Relative Territory Heuristic
 * (RTH).
 * @note RTH calculates the difference in Queen-move distance to every square
 * on the board.
 * @param state The current board state to evaluate.
 * @return A float score representing the territory advantage. Positive means
 * the current player has better access to squares, negative means worse.
 */
static float
eval_rth (BitboardState *state)
{
  int max_moves_nb = state->size * state->size;
  uint8_t dist_white[max_moves_nb];
  uint8_t dist_black[max_moves_nb];
  Bitboard obstacles = get_obstacles (state);

  // BFS for both
  bfs_bitboard (state->white_queens, obstacles, state->size, 0, dist_white);
  bfs_bitboard (state->black_queens, obstacles, state->size, 0, dist_black);

  float score = 0.0f;
  int total_squares = state->size * state->size;
  int potential_bonus = state->size + 2;

  for (int i = 0; i < total_squares; i++)
    {
      // Skip obstacles
      if (bitboard_get_bit (obstacles, i))
        continue;

      uint8_t dw = dist_white[i];
      uint8_t db = dist_black[i];

      if (dw == INF_DIST && db == INF_DIST)
        continue;

      if (dw < db)
        score += (float)((db == INF_DIST ? potential_bonus : db) - dw);
      else if (db < dw)
        score -= (float)((dw == INF_DIST ? potential_bonus : dw) - db);
      // TODO : make MSP later to compare diff between RTH and MSP
    }
  return score;
}

/**
 * @brief Evaluates the board state based on a combination of Queen and King
 * distance to all squares (QDKD).
 * @note This heuristic considers both the number of moves (Queen distance) and
 * the positional advantage (King distance).
 * @param state The current board state to evaluate.
 * @return A float score representing the weighted sum between the queen and
 * king distance. Positive means the current player has better access to
 * squares, negative means worse.
 */
static float
eval_qdkd (BitboardState *state)
{
  int max_moves_nb = state->size * state->size;
  uint8_t q_white[max_moves_nb], q_black[max_moves_nb];
  uint8_t k_white[max_moves_nb], k_black[max_moves_nb];
  Bitboard obstacles = get_obstacles (state);

  // Run 4 BFS passes, to get the shortest paths using queen and king steps.
  bfs_bitboard (state->white_queens, obstacles, state->size, 0, q_white);
  bfs_bitboard (state->black_queens, obstacles, state->size, 0, q_black);
  bfs_bitboard (state->white_queens, obstacles, state->size, 1, k_white);
  bfs_bitboard (state->black_queens, obstacles, state->size, 1, k_black);

  float t1 = 0.0f; // Territory (queen distance)
  float t2 = 0.0f; // Positional (king distance)
  int total_squares = state->size * state->size;

  for (int i = 0; i < total_squares; i++)
    {
      if (bitboard_get_bit (obstacles, i))
        continue;

      // +1 / 0 / -1
      t1 += (q_white[i] < q_black[i]) - (q_black[i] < q_white[i]);
      t2 += (k_white[i] < k_black[i]) - (k_black[i] < k_white[i]);
    }

  return (1.0f * t1) + (0.5f * t2);
}

Evaluator
get_evaluator_by_name (const char *name)
{
  if (strcmp (name, "rth") == 0)
    {
      return eval_rth;
    }
  else if (strcmp (name, "qd&kd") == 0)
    {
      return eval_qdkd;
    }
  return eval_me;
}