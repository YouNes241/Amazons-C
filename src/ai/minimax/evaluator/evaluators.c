#include "bitboard.h"
#include "evaluator.h"
#include "game_types.h"
#include "utils.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INF_DIST 255

typedef enum
{
  QUEEN_SLIDE = 0,
  KING_STEP = 1
} SlideType;

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

  Bitboard empty_squares = bitboard_masked_not (obstacles, width);
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
  // Slide all queens
  Bitboard attacks = expand_queen_slide (queens, empty, size);

  // Count the total unique squares they can reach
  return bitboard_count_bits (attacks);
}

/**
 * @brief Evaluates the board state based on mobility.
 * @note This heuristic counts the total number of legal moves available to the
 * current player and subtracts the opponent's mobility.
 * @param state The current board state to evaluate.
 * @return A float score representing the mobility advantage. Positive means
 * the current player has more mobility, negative means less.
 */
float
eval_me (BitboardState *state)
{
  Bitboard empty = bitboard_masked_not (get_obstacles (state), state->size);

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
  bfs_bitboard (state->white_queens, obstacles, state->size, QUEEN_SLIDE,
                dist_white);
  bfs_bitboard (state->black_queens, obstacles, state->size, QUEEN_SLIDE,
                dist_black);

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
 * @brief Helper function used in get_absolute_territory that expands the
 * blocked accessible tiles.
 * Kind of similar to the game of life x flood-fill.
 *
 * @param front Border (Frontier) of occupied tiles
 * @param empty Empty tiles of the board
 * @param width Width of the board
 * @param mode QUEEN_SLIDE/KING_STEP
 * @param w_reached Occupied tiles // Blocked tiles for whites
 * @param b_reached Occupied tiles // Blocked tiles for blacks
 * @return Bitboard
 */
static inline Bitboard
expand_and_mask_front (Bitboard front, Bitboard empty, uint8_t width, int mode,
                       Bitboard w_reached, Bitboard b_reached)
{
  if (bitboard_is_zero (front))
    {
      return bitboard_create_empty ();
    }

  Bitboard moves;
  if (mode == 0)
    moves = expand_queen_slide (front, empty, width);
  else
    moves = expand_king_step (front, empty, width);

  Bitboard unreached_tiles
      = bitboard_masked_not (bitboard_or (w_reached, b_reached), width);

  return bitboard_and (moves, unreached_tiles);
}

/**
 * @brief Helper for simultaneous BFS using bitwise operations.
 * Returns the territory score difference (White claims - Black claims).
 * Can be used in two modes :
 * - Queen slide : Different using queen moves
 * - King step : Different using king moves
 * @param w_starts Starting white positions
 * @param b_starts Starting black positions
 * @param empty Empty tiles of the board
 * @param width Width of the board
 * @param mode QUEEN_SLIDE/KING_STEP
 * @return float Territory score
 */
static float
get_absolute_territory (Bitboard w_starts, Bitboard b_starts, Bitboard empty,
                        uint8_t width, int mode)
{
  // w_reached/b_reached track everyone's territory so far
  Bitboard w_reached = w_starts;
  Bitboard b_reached = b_starts;

  // w_front/b_front are the "active" edges of the expansion
  Bitboard w_front = w_starts;
  Bitboard b_front = b_starts;

  float w_score = 0.0f;
  float b_score = 0.0f;

  while (!bitboard_is_zero (w_front) || !bitboard_is_zero (b_front))
    {
      Bitboard w_next = expand_and_mask_front (w_front, empty, width, mode,
                                               w_reached, b_reached);
      Bitboard b_next = expand_and_mask_front (b_front, empty, width, mode,
                                               w_reached, b_reached);

      // Remove ties
      Bitboard w_exclusive
          = bitboard_and (w_next, bitboard_masked_not (b_next, width));
      Bitboard b_exclusive
          = bitboard_and (b_next, bitboard_masked_not (w_next, width));

      w_score += (float)bitboard_count_bits (w_exclusive);
      b_score += (float)bitboard_count_bits (b_exclusive);

      // We remove ties
      w_reached = bitboard_or (w_reached, w_exclusive);
      b_reached = bitboard_or (b_reached, b_exclusive);

      // If it was a tie, that square is now an "obstacle" for everyone
      Bitboard ties = bitboard_and (w_next, b_next);
      empty = bitboard_and (empty, bitboard_masked_not (ties, width));

      w_front = w_exclusive;
      b_front = b_exclusive;
    }

  return w_score - b_score;
}

static float
eval_qdkd (BitboardState *state)
{
  Bitboard obstacles = get_obstacles (state);

  // Pre-calculate empty squares once
  Bitboard empty = bitboard_masked_not (obstacles, state->size);

  // t1: Queen distance territory (mode = 0)
  float t1 = get_absolute_territory (state->white_queens, state->black_queens,
                                     empty, state->size, QUEEN_SLIDE);

  // t2: King distance territory (mode = 1)
  float t2 = get_absolute_territory (state->white_queens, state->black_queens,
                                     empty, state->size, KING_STEP);

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