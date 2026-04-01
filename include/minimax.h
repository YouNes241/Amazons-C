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
#endif