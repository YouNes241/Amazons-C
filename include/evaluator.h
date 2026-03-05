#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "game_types.h"

typedef float (*Evaluator) (BitboardState *state);

/**
 * @brief Retrieves an Evaluator function pointer based on the provided name.
 * @param name A string representing the name of the desired evaluator ("me",
 * "rth", "qd&kd").
 * @return An Evaluator function pointer corresponding to the name. Defaults to
 * mobility evaluation if the name is unrecognized.
 */
Evaluator get_evaluator_by_name (const char *name);

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