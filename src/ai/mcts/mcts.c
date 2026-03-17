#include "mcts.h"
#include "ai_interface.h"
#include "bitboard.h"
#include "i_selector.h"
#include "uct.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Checks if the given parent node has a child corresponding to the
 * specified move.
 * @param parent The MCTS_Node to check for the child move.
 * @param move The Move to look for among the children of the parent node.
 * @return true if a child node with the specified move exists, false
 * otherwise.
 */
static bool
has_child_with_move (MCTS_Node *parent, Move move)
{
  if (parent == NULL || parent->first_child == NULL)
    return false;

  MCTS_Node *current = parent->first_child;
  while (current != NULL)
    {
      if (moves_are_equal (current->move_from_parent, move))
        return true;
      current = current->next_sibling;
    }
  return false;
}

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
  while (node->first_child != NULL)
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
  return apply_move_bitboard (state, node->move_from_parent);
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

  while (true)
    {
      Move moves_array[get_move_limit (current)];
      int count = get_legal_moves (current, moves_array);

      if (count == 0)
        break;

      int random_index = rand () % count;
      Move chosen = moves_array[random_index];

      current = apply_move_bitboard (current, chosen);
    }

  bool starting_player_won = !current.white_to_move;

  if (starting_player == starting_player_won)
    return 1.0f;

  return -1.0f;
}

static MCTS_Node *
alloc_mcts_node (MCTS_Node *parent, Move move)
{
  MCTS_Node *node = (MCTS_Node *)calloc (1, sizeof (MCTS_Node));
  if (!node)
    return NULL;

  node->parent = parent;
  node->move_from_parent = move;
  node->visit_count = 0;
  node->score = 0.0f;
  node->first_child = NULL;
  node->next_sibling = NULL;

  return node;
}

/**
 * @brief Creates a new MCTS node with the given parent and move, and adds it
 * to the parent's children list.
 * @param parent The parent MCTS node to which the new node will be added as a
 * child. Can be NULL for the root node.
 * @param move The Move that leads from the parent node to this new node.
 * Ignored if parent is NULL.
 * @return A pointer to the newly created MCTS_Node, or NULL if memory
 * allocation fails.
 */
static MCTS_Node *
create_mcts_node (MCTS_Node *parent, Move move)
{
  MCTS_Node *node = alloc_mcts_node (parent, move);

  if (parent != NULL)
    {
      if (parent->first_child == NULL)
        {
          parent->first_child = node;
        }
      else
        {
          MCTS_Node *sibling = parent->first_child;
          while (sibling->next_sibling != NULL)
            {
              sibling = sibling->next_sibling;
            }
          sibling->next_sibling = node;
        }
    }

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
expand (MCTS_Node *parent, BitboardState state)
{
  if (bitboard_state_is_game_over (state))
    {
      return parent;
    }

  Move moves[get_move_limit (state)];
  int count = get_legal_moves (state, moves);

  if (count == 0)
    return parent;

  MCTS_Node *last_added = NULL;

  for (int i = 0; i < count; i++)
    {
      MCTS_Node *node = alloc_mcts_node (parent, moves[i]);

      if (parent->first_child == NULL)
        parent->first_child = node;
      else
        last_added->next_sibling = node;

      last_added = node;
    }

  int random_idx = rand () % count;
  MCTS_Node *chosen = parent->first_child;

  for (int i = 0; i < random_idx; i++)
    {
      chosen = chosen->next_sibling;
    }

  return chosen;
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

  while ((get_time_ms () - start_time) < time_limit)
    {
      MCTS_Node *picked = select_node (root, data->strategy_select);

      BitboardState current_state = derive_state (root_state, picked);
      picked = expand (picked, current_state);

      current_state
          = apply_move_bitboard (current_state, picked->move_from_parent);
      float result = rollout (current_state);

      backpropagate (picked, 1.0f - result);
    }

  MCTS_Node *best_node = get_child_with_max_visits (root);
  Move final_move = best_node->move_from_parent;

  printf ("[MCTS] Best move : (%d, %d) -> (%d, %d) +-> (%d, %d)\n",
          final_move.queen_src.row, final_move.queen_src.col,
          final_move.queen_dst.row, final_move.queen_dst.col,
          final_move.arrow_dst.row, final_move.arrow_dst.col);

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
  data->max_iterations = 100000;
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