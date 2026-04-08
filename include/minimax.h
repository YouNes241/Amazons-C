/**
 * @file minimax.h
 * @author tdecabrat
 * @brief This file defines all types necessary to a MINIMAX ai and gives the
 * method to factory a MINIMAX AI.
 */
#ifndef MINIMAX_H
#define MINIMAX_H

#include "ai_interface.h"
#include "evaluator.h"

/**
 * @struct ScoredMove
 * @brief Structure used to compact moves with their score.
 * @var ScoredMove::move
 * The associated move
 * @var ScoredMove::score
 * Score of the move
 */
typedef struct
{
  Move move;
  double score;
} ScoredMove;

// Constants and names
extern const int DEFAULT_MAX_DEPTH;                   // depth
extern const int DEFAULT_TIME_LIMIT;                  // time_limit
extern const bool DEFAULT_FORWARD_PRUNING_ACTIVATION; // forward_pruning
extern const bool
    DEFAULT_ITERATIVE_DEEPENING_ACTIVATION;   // iterative_deepening
extern const EvaluatorType DEFAULT_EVALUATOR; // evaluator (me, qd&kd, rth)

/**
 * @struct Minimax_Data
 * @brief Contains the options of minimax
 * @var Minimax_Data::max_depth
 * Max depth of Minimax
 * @var Minimax_Data::time_limit
 * Time limit for the move calculation
 * @var Minimax_Data::forward_pruning_enabled
 * Enables forward pruning, which cuts of a major amount of moves to improve
 * calculation. Can miss some moves that necessitates losing first.
 * @var Minimax_Data::iterative_deepening_enabled
 * Enables iterative deepening, which will return the best move of the last
 * fully explored depth within the time limit.
 * @var Minimax_Data::evaluate
 * Evaluation function used by Minimax
 */
typedef struct
{
  int max_depth;
  uint64_t time_limit;
  bool forward_pruning_enabled;
  bool iterative_deepening_enabled;
  Evaluator evaluate;
} Minimax_Data;

extern const AI_VTable minimax_vtable;

/**
 * @brief Initializes the Minimax AI with parameters for depth, time limit, and
 * evaluator choice.
 * @param self The AI_Interface instance to initialize, which will hold the
 * context for the Minimax algorithm.
 */
void create_minimax_ai (AI_Interface *ai);

/* Test-mode declarations: expose internal functions for unit testing */
#ifdef MINIMAX_TEST_MODE

/**
 * @brief Gets the optimal beam width for forward pruning based on depth.
 * @param maximizing_player Indicates if the current player is the maximizing
 * player.
 * @param current_depth The current depth in the search tree.
 * @param max_depth The maximum depth of the search tree.
 * @return The calculated beam width for the current search depth and player.
 */
int get_beam_width (int maximizing_player, int current_depth, int max_depth);

/**
 * @brief Evaluates a move based on the reachable squares from the target.
 * @param state The current board state before the move.
 * @param move The move to score.
 * @return Score based on mobility from the destination square.
 */
float score_move (const BitboardState *state, Move move);

/**
 * @brief Determines if the alpha-beta search should stop at this node.
 * @param data The Minimax_Data containing time limits and state.
 * @param state The current board state.
 * @param depth The current depth in the search tree.
 * @param start_time The starting time of the algorithm.
 * @return True if search should terminate, false otherwise.
 */
bool alpha_beta_conditions (const Minimax_Data *data,
                            const BitboardState *state, int depth,
                            unsigned long start_time);

/**
 * @brief Updates the best move and alpha-beta bounds.
 * @param is_maximizing Indicates if the current player is the maximizing
 * player.
 * @param score The score of the candidate move.
 * @param candidate The move being evaluated.
 * @param best_move Pointer to update with the best move found.
 * @param best_score Pointer to update with the best score.
 * @param alpha Pointer to the alpha value (lower bound).
 * @param beta Pointer to the beta value (upper bound).
 */
void update_best_move (bool is_maximizing, float score, const Move candidate,
                       Move *best_move, float *best_score, float *alpha,
                       float *beta);

#endif /* MINIMAX_TEST_MODE */

#endif
