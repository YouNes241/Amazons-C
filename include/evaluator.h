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

BitboardState sync_from_logic (GameState *gs);
#endif