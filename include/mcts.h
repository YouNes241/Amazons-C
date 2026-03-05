#ifndef MCTS_H
#define MCTS_H

#include "ai_interface.h"
#include "mcts_types.h"

/**
 * @brief Initializes the MCTS AI with default parameters and sets up the
 * context for the MCTS algorithm.
 * @param self The AI_Interface instance to initialize, which will hold the
 * context for the MCTS algorithm.
 */
void create_mcts_ai (AI_Interface *ai);

#endif