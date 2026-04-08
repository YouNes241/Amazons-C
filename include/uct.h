/**
 * @file uct.h
 * @author tdecabrat
 * @brief This file defines the interface for the UCT (Upper Confidence Bound
 * for Trees) selection strategy used in the MCTS AI.
 */
#ifndef UCT_H
#define UCT_H

#include "i_selector.h"

/**
 * @brief Initializes an ISelector instance to use the UCT (Upper Confidence
 * Bound for Trees) selection strategy.
 * @param selector The ISelector instance to initialize. The function will
 * allocate and set up the context for UCT selection.
 * @param c The exploration constant to use in the UCT formula. A common
 * default value is sqrt(2) (~1.414).
 */
void create_select_uct (ISelector *selector, float c);

#endif