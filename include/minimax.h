#ifndef MINIMAX_H
#define MINIMAX_H

#include "ai_interface.h"
#include "evaluator.h"

typedef struct
{
  int max_depth;
  int time_limit;
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