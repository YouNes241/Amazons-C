/**
 * @file mcts.h
 * @author tdecabrat
 * @brief This file defines all types necessary to an MCTS ai and gives the
 * method to factory a MCTS AI
 */
#ifndef MCTS_H
#define MCTS_H

#include "ai_interface.h"
#include "mcts_types.h"

// Constants and names
extern const uint64_t DEFAULT_TIME_CONSTRAINT; // time_limit
extern const uint32_t DEFAULT_MAX_ITERATIONS;  // max_iterations
extern const int DEFAULT_MAX_WORKERS;          // max_workers

/**
 * @brief Initializes the MCTS AI with default parameters and sets up the
 * context for the MCTS algorithm.
 * @param self The AI_Interface instance to initialize, which will hold the
 * context for the MCTS algorithm.
 */
void create_mcts_ai (AI_Interface *ai);

#endif