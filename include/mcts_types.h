#ifndef MCTS_TYPES_H
#define MCTS_TYPES_H

#include "game_types.h"
#include "i_selector.h"

/**
 * @brief Represents a node in the Monte Carlo Tree Search (MCTS) algorithm,
 * storing the move that led to this node, its parent and children in the
 * search tree, and statistics for the MCTS algorithm (visit count and total
 * value).
 */
typedef struct MCTS_Node
{
  struct MCTS_Node *parent;
  struct MCTS_Node *first_child;
  struct MCTS_Node *next_sibling;
  Move move_from_parent;
  int visit_count;
  float score;
} MCTS_Node;

/**
 * @brief Represents the main data structure for the MCTS AI, containing
 * configuration parameters such as time constraints and the selection strategy
 * to use during the tree search.
 */
typedef struct MCTS_Data
{
  int time_constraint;
  int max_iterations;
  int max_workers;
  ISelector *strategy_select;
} MCTS_Data;

#endif