#include "mcts.h"
#include "ai_interface.h"
#include "bitboard.h"
#include "evaluator.h"
#include "i_selector.h"
#include "uct.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Select a node to expand from the given root node using the provided
 * selection strategy.
 * @param root The root node of the MCTS tree to start selection from.
 * @param selector The selection strategy to use for selecting child nodes.
 * @return The selected MCTS_Node to expand.
 */
static MCTS_Node *
select_node (MCTS_Node *root, ISelector *selector)
{
  MCTS_Node *node = root;

  while (node->is_fully_expanded && node->first_child != NULL)
    {
      node = selector->vtable->select_child (selector, node);
    }

  return node;
}

/**
 * @brief Plays the move sequence from the root to the given node to derive the
 * resulting BitboardState.
 * @param root_state The initial BitboardState corresponding to the root of the
 * MCTS tree.
 * @param node The MCTS_Node for which to derive the resulting state.
 * @return The BitboardState resulting from applying the move sequence from the
 * root to the given node.
 */
static BitboardState
derive_state (BitboardState root_state, MCTS_Node *node)
{
  if (node->parent == NULL)
    return root_state;

  BitboardState state = derive_state (root_state, node->parent);
  return apply_move_bitboard (&state, node->move_from_parent);
}

/**
 * @brief Backpropagates the result of a rollout up the MCTS tree, updating
 * visit counts and scores for each node along the path from the given node to
 * the root.
 * @param node The MCTS_Node from which to start backpropagation.
 * @param result The result of the rollout.
 */
static void
backpropagate (MCTS_Node *node, float result)
{
  MCTS_Node *current = node;
  float current_score = result;

  while (current != NULL)
    {
      current->visit_count++;
      current->score += current_score;
      current_score = 1.0f - current_score;
      current = current->parent;
    }
}

// TODO : doc
static float
get_mcts_win_probability (BitboardState *state, bool starting_player_was_white)
{
  float raw_score = eval_me (state);

  if (!starting_player_was_white)
    {
      raw_score = -raw_score;
    }

  // TODO : calculate using board size (state->size) for more
  // accurate results within different board sizes.
  const float MAX_MOBILITY_DIFF = 40.0f;

  float win_probability = 0.5f + (raw_score / (2.0f * MAX_MOBILITY_DIFF));

  if (win_probability > 1.0f)
    return 1.0f;
  if (win_probability < 0.0f)
    return 0.0f;

  return win_probability;
}

// TODO : doc
static inline bool
is_move_null (Move *m)
{
  // If the source and destination are the same, it's not a legal move in
  // Amazons
  return m->queen_src.col == m->queen_dst.col
         && m->queen_src.row == m->queen_dst.row;
}

/**
 * @brief Performs a random rollout (simulation) from the given state until the
 * game ends, and returns the result from the perspective of the player to move
 * at the start of the rollout.
 * @param state The initial BitboardState from which to start the rollout.
 * @return A float value representing the result of the rollout: 1.0 if the
 * starting player wins, 0.0 if the starting player loses.
 */
static float
rollout (BitboardState state)
{
  BitboardState current = state;
  bool starting_player = state.white_to_move;
  Move m = (Move){ 0 };

  int current_depth = 0;
  int max_depth = 5; // May be set by a parameter

  // Play random moves until reaching max_depth
  while (current_depth < max_depth)
    {
      if (bitboard_state_is_game_over (&current))
        break;

      m = get_direct_random_move (&current);

      if (!is_move_null (&m))
        {
          current = apply_move_bitboard (&current, m);
          current_depth++;
        }
      else
        break;
    }

  if (bitboard_state_is_game_over (&current))
    {
      bool starting_player_won = (starting_player != current.white_to_move);
      return starting_player_won ? 1.0f : 0.0f;
    }

  return get_mcts_win_probability (&current, starting_player);
}

static MCTS_Node *
alloc_mcts_node (MCTS_Node *parent, Move *move)
{
  MCTS_Node *node = (MCTS_Node *)calloc (1, sizeof (MCTS_Node));
  if (!node)
    return NULL;

  node->parent = parent;
  node->move_from_parent = *move;
  node->visit_count = 0;
  node->score = 0.0f;
  node->first_child = NULL;
  node->next_sibling = NULL;
  node->is_fully_expanded = false;

  return node;
}

/**
 * @brief Expands the given MCTS node by adding a new child node for an untried
 * move.
 * @param parent The MCTS node to expand.
 * @param state The current BitboardState corresponding to the parent node,
 * used to determine legal moves.
 * @return The newly created child node corresponding to an untried move, or
 * the parent node if all moves have already been tried.
 */
static MCTS_Node *
expand (MCTS_Node *parent, BitboardState *state)
{
  if (bitboard_state_is_game_over (state))
    return parent;

  // Generate the moves
  Move moves[get_move_limit (state)];
  int total_possible_moves = get_legal_moves (state, moves);

  if (total_possible_moves == 0)
    return parent;

  // Count the number of children
  int num_existing_children = 0;
  MCTS_Node *current = parent->first_child;
  MCTS_Node *last_child = NULL;

  while (current != NULL)
    {
      num_existing_children++;
      last_child = current;
      current = current->next_sibling;
    }

  // If there are unexplored children, take the next available one and
  // initialize it
  if (num_existing_children < total_possible_moves)
    {
      Move new_move = moves[num_existing_children];
      MCTS_Node *new_node = alloc_mcts_node (parent, &new_move);

      if (last_child == NULL)
        parent->first_child = new_node;
      else
        last_child->next_sibling = new_node;

      if (num_existing_children + 1 == total_possible_moves)
        {
          parent->is_fully_expanded = true;
        }

      return new_node;
    }

  return parent;
}

/**
 * @brief Recursively frees the memory allocated for the MCTS tree starting
 * from the given node.
 * @param node The root node of the subtree to free. This function will free
 * all descendants of this node as well.
 */
static void
free_mcts_tree (MCTS_Node *node)
{
  if (node == NULL)
    return;

  MCTS_Node *curr = node->first_child;
  while (curr != NULL)
    {
      MCTS_Node *next = curr->next_sibling;
      free_mcts_tree (curr);
      curr = next;
    }
  free (node);
}

/**
 * @brief Gets the child node with the highest visit count from the given
 * root node.
 * @param root The root node of the MCTS tree to search through.
 * @return The child node with the highest visit count, or NULL if there are
 * no children.
 */
static MCTS_Node *
get_child_with_max_visits (MCTS_Node *root)
{
  MCTS_Node *best_node = NULL;
  int max_visits = -1;

  for (MCTS_Node *curr = root->first_child; curr != NULL;
       curr = curr->next_sibling)
    {
      if (curr->visit_count > max_visits)
        {
          max_visits = curr->visit_count;
          best_node = curr;
        }
    }
  return best_node;
}

/**
 * @brief Gets the best move using MCTS.
 * @param self The AI_Interface instance containing the context for the MCTS
 * algorithm.
 * @param state The current GameState to evaluate and find the best move for.
 * @return The best Move for the current player based on the MCTS evaluation.
 */
static Move
mcts_get_best_move (AI_Interface *self, GameState state)
{
  MCTS_Data *data = (MCTS_Data *)self->context;
  BitboardState root_state = sync_from_logic (&state);
  MCTS_Node *root = calloc (1, sizeof (MCTS_Node));
  root->visit_count = 1;

  unsigned long start_time = get_time_ms ();
  unsigned long time_limit = data->time_constraint;
  int max_iters = data->max_iterations;
  int current_iterations = 0;

  while (((get_time_ms () - start_time) < time_limit)
         && (current_iterations < max_iters))
    {
      MCTS_Node *picked = select_node (root, data->strategy_select);

      BitboardState current_state = derive_state (root_state, picked);
      picked = expand (picked, &current_state);

      current_state
          = apply_move_bitboard (&current_state, picked->move_from_parent);
      float result = rollout (current_state);

      backpropagate (picked, 1.0f - result);

      current_iterations++;
    }

  MCTS_Node *best_node = get_child_with_max_visits (root);
  Move final_move = best_node->move_from_parent;

  printf ("[MCTS] Iterations: %d | Best move : (%d, %d) -> (%d, %d)\n",
          current_iterations, final_move.queen_src.row,
          final_move.queen_src.col, final_move.queen_dst.row,
          final_move.queen_dst.col);

  free_mcts_tree (root);
  return final_move;
}

/**
 * @brief Cleans up the MCTS AI by freeing the selector strategy
 * and the main data context.
 * @param self The AI_Interface instance to clean, which holds the context
 * for the MCTS algorithm.
 */
static void
mcts_clean (AI_Interface *self)
{
  MCTS_Data *data = (MCTS_Data *)self->context;
  if (!data)
    return;

  if (data->strategy_select)
    {
      if (data->strategy_select->vtable
          && data->strategy_select->vtable->clean)
        {
          data->strategy_select->vtable->clean (data->strategy_select);
        }
      free (data->strategy_select);
    }

  free (data);
  self->context = NULL;
  printf ("[MCTS] Clean OK\n");
}

/**
 * @brief Initializes the MCTS AI with parameters for time, iterations, and
 * strategy.
 * @param self The AI_Interface instance to initialize, which will hold the
 * context for the MCTS algorithm.
 * @param params A ConfigMap containing optional parameters:
 * @note time_limit: Time in milliseconds for the MCTS search (default
 * 5000ms).
 * @note max_iterations: Maximum number of iterations for the MCTS search
 * (default 100000).
 * @note max_workers: Maximum number of worker threads for parallel MCTS
 * (default 1).
 */
static void
mcts_init (AI_Interface *self, ConfigMap *params)
{
  MCTS_Data *data = (MCTS_Data *)malloc (sizeof (MCTS_Data));
  if (!data)
    return;

  data->time_constraint = 5000;
  // TODO : Finish implementing missing parameters
  data->max_iterations = INT32_MAX;
  data->max_workers = 1;

  data->strategy_select = (ISelector *)malloc (sizeof (ISelector));
  create_select_uct (data->strategy_select, 1.414f);
  // ^ (sqrt2) is a common default for the exploration constant

  if (params != NULL)
    {
      for (int i = 0; i < params->count; i++)
        {
          check_param (params->list[i], "time_limit", &data->time_constraint);
          check_param (params->list[i], "max_iterations",
                       &data->max_iterations);
          check_param (params->list[i], "max_workers", &data->max_workers);
        }
    }

  self->context = data;
  printf ("[MCTS] Init OK (Time: %dms)\n", data->time_constraint);
}

const AI_VTable mcts_vtable = { .init = mcts_init,
                                .clean = mcts_clean,
                                .get_best_move = mcts_get_best_move };

void
create_mcts_ai (AI_Interface *ai)
{
  ai->vtable = &mcts_vtable;
  ai->context = NULL;
}