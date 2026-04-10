/**
 * @file mcts.h
 * @author tdecabrat
 * @brief This file defines all types necessary to an MCTS ai and gives the
 * method to factory a MCTS AI.
 */
#ifndef MCTS_H
#define MCTS_H

#include "ai_interface.h"
#include "i_selector.h"

/**
 * @struct MCTS_Node
 * @brief
 * Represents a node in the Monte Carlo Tree Search (MCTS) algorithm,
 * storing the move that led to this node, its parent and children in the
 * search tree, and statistics for the MCTS algorithm (visit count and total
 * value).
 * @var MCTS_Node::parent
 * The parent node of this node
 * @var MCTS_Node::first_child
 * First child of this node
 * @var MCTS_Node::next_sibling
 * Next sibling of move (so with the same parent)
 * @var MCTS_Node::move_from_parent
 * Move done by the parent to reach this node
 * @var MCTS_Node::visit_count
 * Number of times this node was visited
 * @var MCTS_Node::score
 * Score of the move
 */
typedef struct MCTS_Node
{
  struct MCTS_Node *parent;
  struct MCTS_Node *first_child;
  struct MCTS_Node *next_sibling;
  Move move_from_parent;
  BitboardState state;

  Move *untried_moves;
  int num_untried;

  int visit_count;
  float score;
  bool is_fully_expanded;
  float ai_eval;
} MCTS_Node;

/**
 * @struct MCTS_Data
 * @brief Represents the main data structure for the MCTS AI, containing
 * configuration parameters such as time constraints and the selection strategy
 * to use during the tree search.
 * @var MCTS_Data::time_constraint
 * The time limit of MCTS
 * @var MCTS_Data::max_iterations
 * The max number of iterations for MCTS
 * @var MCTS_Data::max_workers
 * The max number of workers for MCTS
 * @var MCTS_Data::strategy_select
 * Strategy selected to select moves
 */
typedef struct MCTS_Data
{
  uint64_t time_constraint;
  uint32_t max_iterations;
  int max_workers;
  ISelector *strategy_select;

  MCTS_Node **roots;
} MCTS_Data;

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
