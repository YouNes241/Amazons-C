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
static const int DEFAULT_MAX_DEPTH = 3;
static const int DEFAULT_TIME_LIMIT = 5;
static const int NB_BEST_MOVES_PLAYER = 5;
static const int NB_BEST_MOVES_OPPONENT = 20;
static const float BOTTOM_SCORE = -1000000.0f;
static const float TOP_SCORE = 1000000.0f;
static const char *DEFAULT_EVALUATOR = "me";

typedef struct
{
  Move move;
  int score;
} ScoredMove;

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

/**
 * @brief Applies a move on a BitboardState and returns the resulting state.
 * @param state The current board state before the move.
 * @param move The move to apply, containing source and destination for the
 * queen and the arrow.
 * @return A new BitboardState with the move applied.
 */
static BitboardState
apply_move_bitboard (BitboardState state, Move move)
{
  // Init
  BitboardState next = state;
  Position q_src = move.queen_src;
  Position q_dst = move.queen_dst;
  Position a_dst = move.arrow_dst;
  int size = state.size;

  // Convert pos for bits
  uint8_t src_idx = q_src.row * size + q_src.col;
  uint8_t dst_idx = q_dst.row * size + q_dst.col;
  uint8_t arr_idx = a_dst.row * size + a_dst.col;

  // Who's moving ?
  Bitboard *my_queens
      = (state.white_to_move) ? &next.white_queens : &next.black_queens;

  // Remove
  bitboard_clear_bit (my_queens, src_idx);

  // Place
  bitboard_set_bit (my_queens, dst_idx);
  bitboard_set_bit (&next.arrows, arr_idx);

  next.white_to_move = !state.white_to_move;

  return next;
}

/**
 * @brief Checks if a given parameter key matches the expected key and, if so,
 * parses its value as an integer and assigns it to the target variable.
 * @param p The parameter to check, containing a key and a value.
 * @param key The expected key to match against the parameter's key.
 * @param target The value to set based on the parameter's valuation. *
 */
static void
check_param (Param p, const char *key, int *target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = atoi (p.value);
    }
}

/**
 * @brief Checks if a given parameter key matches the expected key and, if so,
 * parses its value as a string and assigns it to the target variable.
 * @param p The parameter to check, containing a key and a value.
 * @param key The expected key to match against the parameter's key.
 * @param target The value to set based on the parameter's valuation. *
 */
static void
check_str_param (Param p, const char *key, char **target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = strdup (p.value);
    }
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
static void // TODO : run a thread (or more) on the side to compute moves
minimax_init (AI_Interface *self, ConfigMap *params)
{
  Minimax_Data *data = (Minimax_Data *)malloc (sizeof (Minimax_Data));
  if (!data)
    return;

  data->max_depth = DEFAULT_MAX_DEPTH;
  data->time_limit = DEFAULT_TIME_LIMIT;
  char *evaluator = (char *)DEFAULT_EVALUATOR;

  if (params != NULL)
    {
      for (int i = 0; i < params->count; i++)
        {
          check_param (params->list[i], "depth", &data->max_depth);
          check_param (params->list[i], "time_limit", &data->time_limit);
          check_str_param (params->list[i], "evaluator", &evaluator);
        }
    }

  data->evaluate = get_evaluator_by_name (evaluator);

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
 * @brief Checks if the game is over by determining if there are no legal moves
 * available for the current player.
 * @param state The current board state to evaluate.
 * @return true if the game is over (no legal moves), false otherwise.
 */
static bool
is_game_over (BitboardState state)
{
  return get_legal_moves (state, NULL) == 0;
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
static int
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
static float
score_move (BitboardState state, Move move)
{
  int8_t center = state.size / 2;
  int dist_to_center
      = abs (move.queen_dst.row - center) + abs (move.queen_dst.col - center);

  return (float)(20 - dist_to_center);
}

/**
 * @brief Picks the top moves from a list of raw moves based on their scores
 * and whether the current player is maximizing or minimizing.
 * @param state The current board state to evaluate the moves against.
 * @param raw_moves An array of potential moves to evaluate.
 * @param count The total number of raw moves available.
 * @param best_moves An array to store the top moves that are selected based on
 * their scores.
 * @param limit The maximum number of top moves to select.
 * @param maximizing A boolean indicating if the current player is the
 * maximizing player.
 * @return The actual number of moves selected as the best moves.
 */
static int
pick_top_moves (BitboardState *state, Move *raw_moves, int count,
                ScoredMove *best_moves, int limit, bool maximizing)
{
  int best_count = 0;

  for (int i = 0; i < count; i++)
    {
      float score = score_move (*state, raw_moves[i]);

      bool qualify
          = (best_count < limit)
            || (maximizing ? (score > best_moves[best_count - 1].score)
                           : (score < best_moves[best_count - 1].score));

      if (qualify)
        {
          if (best_count < limit)
            best_count++;

          int j = best_count - 1;

          while (j > 0
                 && (maximizing ? (score > best_moves[j - 1].score)
                                : (score < best_moves[j - 1].score)))
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

/**
 * @brief Implements the Minimax algorithm with alpha-beta pruning and forward
 * pruning to evaluate the best move for the current player.
 * @param data The Minimax_Data containing configuration and evaluation
 * function.
 * @param state The current board state to evaluate.
 * @param depth The current depth in the search tree.
 * @param alpha The best score that the maximizing player can guarantee at this
 * level or above.
 * @param beta The best score that the minimizing player can guarantee at this
 * level or above.
 * @param maximizing A boolean indicating if the current player is the
 * maximizing player.
 * @return The best score for the current player at this level of the search
 * tree.
 */
static float
alpha_beta_forward (Minimax_Data *data, BitboardState state, int depth,
                    float alpha, float beta, bool maximizing)
{
  if (depth <= 0 || is_game_over (state))
    {
      return data->evaluate (&state);
    }

  Move raw_moves[get_move_limit (state)];
  int total_count = get_legal_moves (state, raw_moves);

  if (total_count == 0)
    {
      return maximizing ? BOTTOM_SCORE : TOP_SCORE;
    }

  int limit = get_beam_width (maximizing, depth, data->max_depth);
  int moves_to_scan = (total_count < limit) ? total_count : limit;
  ScoredMove best_moves[moves_to_scan];

  int actual_count = pick_top_moves (&state, raw_moves, total_count,
                                     best_moves, moves_to_scan, maximizing);

  float best_score = maximizing ? BOTTOM_SCORE : TOP_SCORE;

  for (int i = 0; i < actual_count; i++)
    {
      BitboardState next_state
          = apply_move_bitboard (state, best_moves[i].move);
      float score = alpha_beta_forward (data, next_state, depth - 1, alpha,
                                        beta, !maximizing);

      if (maximizing)
        {
          if (score > best_score)
            best_score = score;
          if (best_score > alpha)
            alpha = best_score;
        }
      else
        {
          if (score < best_score)
            best_score = score;
          if (best_score < beta)
            beta = best_score;
        }

      if (beta <= alpha)
        break;
    }

  return best_score;
}

/**
 * @brief Gets the best move for the current player using the Minimax algorithm
 * with alpha-beta pruning and forward pruning.
 * @param self The AI_Interface instance containing the context for the Minimax
 * algorithm.
 * @param state The current GameState to evaluate and find the best move for.
 * @return The best Move for the current player based on the Minimax
 * evaluation.
 */
static Move
minimax_get_best_move (AI_Interface *self, GameState state)
{
  Minimax_Data *data = (Minimax_Data *)self->context;

  printf ("[Minimax] Searching a move (Depth=%d)...\n", data->max_depth);

  BitboardState fast_state = sync_from_logic (&state);

  float alpha = BOTTOM_SCORE;
  float beta = TOP_SCORE;

  Move best_move = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
  float best_score = BOTTOM_SCORE;

  Move available_moves[get_move_limit (fast_state)];
  int count = get_legal_moves (fast_state, available_moves);

  for (int i = 0; i < count; i++)
    {
      BitboardState next_state
          = apply_move_bitboard (fast_state, available_moves[i]);

      float score = alpha_beta_forward (data, next_state, data->max_depth - 1,
                                        alpha, beta, false);

      if (score > best_score)
        {
          best_score = score;
          best_move = available_moves[i];
        }
      alpha = MAX (alpha, best_score);
    }

  printf ("[Minimax] Best move found with score: %.2f\n", best_score);
  return best_move;
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