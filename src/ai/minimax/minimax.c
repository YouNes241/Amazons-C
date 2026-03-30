#include "minimax.h"
#include "ai_interface.h"
#include "bitboard.h"
#include "evaluator.h"
#include "game_types.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// PARAMS
const int DEFAULT_MAX_DEPTH = 3;
const int DEFAULT_TIME_LIMIT = 5000;
const bool DEFAULT_FORWARD_PRUNING_ACTIVATION = true;
const bool DEFAULT_ITERATIVE_DEEPENING_ACTIVATION = false;
const EvaluatorType DEFAULT_EVALUATOR = EVAL_ME;

// INTERNAL CONSTANTS
static const int NB_BEST_MOVES_PLAYER = 10;
static const int NB_BEST_MOVES_OPPONENT = 25;
static const float BOTTOM_SCORE = -1000000.0f;
static const float TOP_SCORE = 1000000.0f;

/**
 * @brief Get the possible moves for a queen at a given index, based on the
 * occupied squares.
 *
 * @param queen_index The linear index of the queen.
 * @param occupied Bitboard representing all occupied squares (queens and
 * arrows).
 * @param size The dimension of the board.
 * @param moves_array Output array to store the generated moves. Mind that it
 * should be large enough to hold all possible moves.
 *
 * @return int The number of moves generated and stored in moves_array.
 */
static int
get_queen_moves (uint8_t queen_index, Bitboard occupied, uint8_t size,
                 Move *moves_array)
{
  Bitboard reachable = bitboard_get_reachable (queen_index, occupied, size);
  int count = 0;

  while (!bitboard_is_zero (reachable))
    {
      int idx = pop_lsb (&reachable);
      moves_array[count++]
          = (Move){ .queen_src = idx_to_pos (queen_index, size),
                    .queen_dst = idx_to_pos (idx, size),
                    .arrow_dst = { -1, -1 } };
    }
  return count;
}

/**
 * @brief Get the possible moves for the arrow based on the queen moves and the
 * occupied squares.
 *
 * @param queen_moves Array of moves representing the queen's potential moves.
 * @param queen_move_count The number of queen moves in the queen_moves array.
 * @param occupied Bitboard representing all occupied squares (queens and
 * arrows).
 * @param size The dimension of the board.
 * @param final_moves Output array to store the final moves with arrow
 * destinations.
 *
 * @return int The number of moves generated and stored in final_moves.
 */
static int
get_arrow_moves (const Move *queen_moves, int queen_move_count,
                 Bitboard occupied, uint8_t size, Move *final_moves)
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

      Bitboard reachable = bitboard_get_reachable (idx, new_occupied, size);

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

/**
 * @brief Initializes the Minimax AI with the provided configuration
 * parameters.
 * @note Parameters available : "depth" (int), "time_limit" (int), "evaluator"
 * (string, e.g., "me", "rth", "qd&kd")
 * @param self The AI_Interface instance to initialize, which will hold the
 * context for the Minimax algorithm.
 * @param params A ConfigMap containing key-value pairs for configuration.
 */
static void
minimax_init (AI_Interface *self, const ConfigMap *params)
{
  Minimax_Data *data = (Minimax_Data *)malloc (sizeof (Minimax_Data));
  if (!data)
    return;

  data->max_depth = DEFAULT_MAX_DEPTH;
  data->time_limit = DEFAULT_TIME_LIMIT;
  data->forward_pruning_enabled = DEFAULT_FORWARD_PRUNING_ACTIVATION;
  data->iterative_deepening_enabled = DEFAULT_ITERATIVE_DEEPENING_ACTIVATION;
  EvaluatorType evaluator = DEFAULT_EVALUATOR;

  if (params != NULL)
    {
      for (int i = 0; i < params->count; i++)
        {
          check_param (params->list[i], "depth", &data->max_depth);
          check_param (params->list[i], "time_limit", &data->time_limit);
          check_param (params->list[i], "forward_pruning",
                       &data->forward_pruning_enabled);
          check_param (params->list[i], "iterative_deepening",
                       &data->iterative_deepening_enabled);
          check_param (params->list[i], "evaluator", &evaluator);
        }
    }

  printf ("[Minimax] AI initialized with depth=%d, time_limit=%lu ms, "
          "forward_pruning=%s, iterative_deepening=%s, evaluator=%d\n",
          data->max_depth, data->time_limit,
          data->forward_pruning_enabled ? "enabled" : "disabled",
          data->iterative_deepening_enabled ? "enabled" : "disabled",
          evaluator);
  data->evaluate = get_evaluator (evaluator);

  self->context = data;
  printf ("[Minimax] Init OK\n");
}

/**
 * @brief Cleans up the Minimax AI by freeing allocated resources and resetting
 * the context.
 * @param self The AI_Interface instance to clean, which holds the context for
 * the Minimax algorithm.
 */
static void
minimax_clean (AI_Interface *self)
{
  Minimax_Data *data = (Minimax_Data *)self->context;

  free (data);
  self->context = NULL;

  printf ("[Minimax] Clean OK\n");
}

/**
 * @brief Gets the optimal beam width for the forward pruning  based on
 * max_depth.
 * @param maximizing_player Indicates if the current player is the maximizing
 * player.
 * @param current_depth The current depth in the search tree.
 * @param max_depth The maximum depth of the search tree.
 * @return The calculated beam width for the current search depth and player.
 */
static inline int
get_beam_width (int maximizing_player, int current_depth, int max_depth)
{
  int base_width
      = maximizing_player ? NB_BEST_MOVES_PLAYER : NB_BEST_MOVES_OPPONENT;

  int depth_bonus = (max_depth - current_depth) * 3;

  return base_width + depth_bonus;
}

/**
 * @brief Scores a move based on its proximity to the center of the board.
 * @param state The current board state before the move.
 * @param move The move to score, containing source and destination for the
 * queen and the arrow.
 * @return Score
 */
static inline float
score_move (BitboardState state, Move move)
{
  // TODO : try to fix, this evaluation is not efficient
  int center = state.size / 2;
  int dist_to_center
      = abs (move.queen_dst.row - center) + abs (move.queen_dst.col - center);

  float base_score = (float)(20 - dist_to_center);
  float randomness = (float)(next_xoshiro () % 101) / 100.0f;

  return base_score + randomness;
}

/**
 * @brief Picks the top moves from a list of raw moves based on their scores
 * and whether the current player is maximizing or minimizing.
 * @param state The current board state to evaluate the moves against.
 * @param raw_moves An array of potential moves to evaluate.
 * @param count The total number of raw moves available.
 * @param limit The maximum number of top moves to select.
 * @param maximizing A boolean indicating if the current player is the
 * maximizing player.
 * @param best_moves An array to store the top moves that are selected based
 * on their scores.
 * @return The actual number of moves selected as the best moves.
 */
static int
pick_top_moves (const BitboardState *state, const Move *raw_moves, int count,
                int limit, ScoredMove *best_moves)
{
  int best_count = 0;
  for (int i = 0; i < count; i++)
    {
      float score = score_move (*state, raw_moves[i]);
      bool qualify
          = (best_count < limit) || (score > best_moves[best_count - 1].score);

      if (qualify)
        {
          if (best_count < limit)
            best_count++;
          int j = best_count - 1;
          while (j > 0 && score > best_moves[j - 1].score)
            {
              best_moves[j] = best_moves[j - 1];
              j--;
            }
          best_moves[j].move = raw_moves[i];
          best_moves[j].score = score;
        }
    }
  return best_count;
}

static inline bool
alpha_beta_conditions (const Minimax_Data *data, const BitboardState *state,
                       int depth, unsigned long start_time)
{
  return (get_time_ms () - start_time >= data->time_limit) || depth <= 0
         || bitboard_state_is_game_over (state);
}

static int
minimax_get_queen_moves (Bitboard player_queens, Bitboard occupied, int size,
                         Move *moves)
{
  int queen_count = 0;
  Bitboard test = player_queens;
  while (!bitboard_is_zero (test))
    {
      uint8_t q_idx = pop_lsb (&test);
      queen_count
          += get_queen_moves (q_idx, occupied, size, &moves[queen_count]);
    }
  return queen_count;
}

/**
 * @brief Get the top moves for a given board state, applying forward pruning
 * to limit the number of moves considered at each level of the Minimax
 * search. The moves are scored and sorted based on their proximity to the
 * center + a tiny of randomness.
 *
 * @param state The current board state to evaluate.
 * @param beam_limit The maximum number of top moves to return.
 * @param maximizing Indicates if the current player is the maximizing
 * player.
 * @param top_moves Output array to store the top moves with their scores.
 * Should be at least of size beam_limit.
 * @return int The actual number of top moves returned, which may be less
 * than or equal to beam_limit depending on the number of legal moves
 * available.
 */
static int
get_top_moves (const BitboardState *state, int beam_limit,
               ScoredMove *top_moves)
{
  Move queen_moves[get_move_limit (state)];

  Bitboard player_queens
      = state->white_to_move ? state->white_queens : state->black_queens;
  Bitboard occupied = bitboard_or (
      bitboard_or (state->white_queens, state->black_queens), state->arrows);

  int queen_count = minimax_get_queen_moves (player_queens, occupied,
                                             state->size, queen_moves);

  if (queen_count == 0)
    return 0;

  ScoredMove filtered_queens[beam_limit];
  int actual_q_count = pick_top_moves (state, queen_moves, queen_count,
                                       beam_limit, filtered_queens);

  Move filtered_full_moves[get_move_limit (state)];
  int total_full_moves = 0;

  for (int i = 0; i < actual_q_count; i++)
    {
      total_full_moves += get_arrow_moves (
          &filtered_queens[i].move, 1, occupied, state->size,
          &filtered_full_moves[total_full_moves]);
    }

  return pick_top_moves (state, filtered_full_moves, total_full_moves,
                         beam_limit, top_moves);
}

/**
 * @brief Updates the best move and score based on the new candidate move and
 * its score, while also updating the alpha and beta values for pruning.
 *
 * @param is_maximizing Indicates if the current player is the maximizing
 * player.
 * @param score The score of the candidate move being evaluated.
 * @param candidate The move being evaluated as a potential best move.
 * @param best_move Pointer to the current best move, which may be updated if
 * the candidate move is better.
 * @param best_score Pointer to the current best score, which may be updated
 * if the candidate move has a better score.
 * @param alpha Pointer to the current alpha value for alpha-beta pruning,
 * which may be updated if the candidate move is better for the maximizing
 * player.
 * @param beta Pointer to the current beta value for alpha-beta pruning,
 * which may be updated if the candidate move is better for the minimizing
 * player.
 */
static void
update_best_move (bool is_maximizing, float score, Move candidate,
                  Move *best_move, float *best_score, float *alpha,
                  float *beta)
{
  if (is_maximizing)
    {
      if (score > *best_score)
        {
          *best_score = score;
          if (best_move)
            *best_move = candidate;
        }
      *alpha = MAX (*alpha, *best_score);
    }
  else
    {
      if (score < *best_score)
        {
          *best_score = score;
          if (best_move)
            *best_move = candidate;
        }
      *beta = MIN (*beta, *best_score);
    }
}

/**
 * @brief Implements the Minimax algorithm with alpha-beta pruning and
 * forward pruning to evaluate the best move for the current player.
 * @param data The Minimax_Data containing configuration and evaluation
 * function.
 * @param state The current board state to evaluate.
 * @param depth The current depth in the search tree.
 * @param alpha The best score that the maximizing player can guarantee at
 * this level or above.
 * @param beta The best score that the minimizing player can guarantee at
 * this level or above.
 * @param maximizing A boolean indicating if the current player is the
 * maximizing player.
 * @param start_time The starting time of the algorithm
 * @return The best score for the current player at this level of the search
 * tree.
 */
static float
alpha_beta_forward (const Minimax_Data *data, const BitboardState *state,
                    int depth, float alpha, float beta, bool maximizing,
                    unsigned long start_time)
{
  if (alpha_beta_conditions (data, state, depth, start_time))
    return data->evaluate (state);

  int beam_limit = data->forward_pruning_enabled
                       ? get_beam_width (maximizing, depth, data->max_depth)
                       : get_move_limit (state);

  ScoredMove top_moves[beam_limit];
  int actual_count = get_top_moves (state, beam_limit, top_moves);

  if (actual_count == 0)
    return maximizing ? BOTTOM_SCORE : TOP_SCORE;

  float best_score = maximizing ? BOTTOM_SCORE : TOP_SCORE;

  for (int i = 0; i < actual_count; i++)
    {
      BitboardState next_state
          = apply_move_bitboard (state, top_moves[i].move);
      float score = alpha_beta_forward (data, &next_state, depth - 1, alpha,
                                        beta, !maximizing, start_time);
      update_best_move (maximizing, score, top_moves[i].move, NULL,
                        &best_score, &alpha, &beta);

      if (beta <= alpha)
        break;
    }

  return best_score;
}

/**
 * @brief Searches for the best move from the root of the Minimax search
 * tree, applying forward pruning and alpha-beta pruning to efficiently
 * evaluate the best move within the given time constraints. Called when
 * initializing a move search.
 *
 * @param data The Minimax_Data containing configuration and evaluation
 * function.
 * @param state The current board state to evaluate and find the best move
 * for.
 * @param depth The maximum depth to search in the Minimax tree.
 * @param start_time The starting time of the search, used to enforce time
 * limits.
 * @return Move The best move found for the current player based on the
 * Minimax evaluation. If no moves are available, returns a default move with
 * all fields set to zero.
 */
static Move
run_root_search (const Minimax_Data *data, const BitboardState *state,
                 int depth, unsigned long start_time, Move *available_moves,
                 int count) // <--- Added array and count
{
  float alpha = BOTTOM_SCORE;
  float beta = TOP_SCORE;
  bool root_is_white = state->white_to_move;

  Move best_move = { 0 };
  float best_score = root_is_white ? BOTTOM_SCORE : TOP_SCORE;

  // REMOVED: get_legal_moves is no longer called here!

  for (int i = 0; i < count; i++)
    {
      if (get_time_ms () - start_time >= data->time_limit)
        break;

      BitboardState next_state
          = apply_move_bitboard (state, available_moves[i]);

      float score = alpha_beta_forward (data, &next_state, depth - 1, alpha,
                                        beta, !root_is_white, start_time);

      update_best_move (root_is_white, score, available_moves[i], &best_move,
                        &best_score, &alpha,
                        &beta); // Using your new All-In-One updater

      if (beta <= alpha)
        break;
    }

  return best_move;
}
/**
 * @brief Gets the best move using Minimax and the given parameters.
 *
 * @param self The AI_Interface instance containing the context for the
 * Minimax algorithm.
 * @param state The current GameState to evaluate and find the best move for.
 * @return Move The current best move for the current player based on the
 * Minimax evaluation. If no moves are available, returns a default random
 * move.
 */
static Move
minimax_get_best_move (const AI_Interface *self, GameState state)
{
  Minimax_Data *data = (Minimax_Data *)self->context;
  BitboardState root_state = sync_from_logic (&state);
  unsigned long start_time = get_time_ms ();

  // Generate all legal moves at the root level to avoid redundant generation
  // in the search
  Move available_moves[get_move_limit (&root_state)];
  int count = get_legal_moves (&root_state, available_moves);

  if (count == 0)
    return get_direct_random_move (&root_state); // Fallback

  Move best_move_overall = available_moves[0];

  if (!data->iterative_deepening_enabled)
    {
      best_move_overall = run_root_search (data, &root_state, data->max_depth,
                                           start_time, available_moves, count);
    }
  else
    {
      for (int d = 1; d <= data->max_depth; d++)
        {
          if (get_time_ms () - start_time >= data->time_limit)
            break;

          // Put the previous best move at index 0
          if (d > 1)
            {
              for (int i = 0; i < count; i++)
                {
                  if (moves_are_equal (available_moves[i], best_move_overall))
                    {
                      Move temp = available_moves[0];
                      available_moves[0] = available_moves[i];
                      available_moves[i] = temp;
                      break;
                    }
                }
            }

          Move move_at_depth = run_root_search (
              data, &root_state, d, start_time, available_moves, count);

          if (get_time_ms () - start_time < data->time_limit)
            {
              best_move_overall = move_at_depth;
            }
        }
    }

  return best_move_overall;
}

const AI_VTable minimax_vtable = { .init = minimax_init,
                                   .clean = minimax_clean,
                                   .get_best_move = minimax_get_best_move };

void
create_minimax_ai (AI_Interface *ai)
{
  ai->vtable = &minimax_vtable;
  ai->context = NULL;
}