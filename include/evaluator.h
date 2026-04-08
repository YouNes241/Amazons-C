/**
 * @file evaluator.h
 * @author tdecabrat
 * @brief This file defines the Evaluator type and related functions for
 * evaluating the game state in the Amazons AI. Evaluators are heuristics that
 * provide a score for a given board state, which can be used by AI algorithms
 * like Minimax to make informed decisions. The file includes definitions for
 * different types of evaluators, such as mobility and territory control, as
 * well as a function to retrieve an evaluator based on a specified type.
 */
#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "bitboard_state.h"
#include "game_types.h"

typedef enum
{
  EVAL_ME,
  EVAL_RTH,
  EVAL_QD_KD
} EvaluatorType;
typedef struct
{
  EvaluatorType type;
  const char *name;
} EvaluatorMap;

static const EvaluatorMap EVALUATOR_DICT[]
    = { { EVAL_ME, "me" }, { EVAL_RTH, "rth" }, { EVAL_QD_KD, "qd&kd" } };

/**
 * @brief Checks if the given evaluator name exists.
 * @param name Name of evaluator.
 * @return true if the evaluator exists.
 * @return false otherwise.
 */

bool is_valid_evaluator_name (const char *name);

/**
 * @brief Turns an EvaluatorType into a string parameter that can be passed to
 * the AI through the ConfigMap. This is necessary because the AI factory
 * expects string parameters.
 *
 * @param type The EvaluatorType to convert.
 * @return char* A string representation of the EvaluatorType.
 */
static inline char *
get_evaluator_param (EvaluatorType type)
{
  static char param_value[2];

  param_value[0] = (char)type + '0';

  param_value[1] = '\0';

  return param_value;
}

typedef float (*Evaluator) (const BitboardState *state);

/**
 * @brief Retrieves an Evaluator function pointer based on the provided name.
 * @param name A string representing the name of the desired evaluator ("me",
 * "rth", "qd&kd").
 * @return An Evaluator function pointer corresponding to the name. Defaults to
 * mobility evaluation if the name is unrecognized.
 */
Evaluator get_evaluator (const EvaluatorType name);

/**
 * @brief Evaluates the board state based on mobility, calculating the
 * difference in the number of legal moves available to the current player
 * versus the opponent.
 * @param state The current board state to evaluate.
 * @return A float score representing the mobility advantage. Positive means
 * the current player has more mobility, negative means less.
 */
Bitboard expand_king_step (Bitboard sources, Bitboard empty, uint8_t width);
#endif
