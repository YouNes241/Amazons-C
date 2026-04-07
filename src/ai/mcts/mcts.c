/**
 * @file mcts.c
 * @author tdecabrat
 * @brief This file implements a Monte Carlo Tree Search (MCTS) AI for the game
 * of Amazons. MCTS is a heuristic search algorithm for decision-making in
 * games, which builds a search tree incrementally and uses random simulations
 * to evaluate the potential of moves. The implementation includes support for
 * multi-threading, allowing multiple worker threads to perform MCTS in
 * parallel. The rollout is also heuristic-based, using an evaluation function
 * to guide the random playouts and cut them off sooner, which can improve the
 * quality of the search results while reducing complexity.
 */

/* Test mode support: make static functions visible to unit tests */
#ifdef MCTS_TEST_MODE
#define MCTS_STATIC
#else
#define MCTS_STATIC static
#endif

#include "mcts.h"
#include "ai_interface.h"
#include "bitboard.h"
#include "evaluator.h"
#include "i_selector.h"
#include "mcts_includes/mcts_internal.h"
#include "mcts_types.h"
#include "uct.h"
#include "utils.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

const uint64_t DEFAULT_TIME_CONSTRAINT = 5;
const uint32_t DEFAULT_MAX_ITERATIONS = INT32_MAX;
const int DEFAULT_MAX_WORKERS = 1;

/**
 * @brief Checks if a move is null (queen source and destination are the same).
 * Body defined in mcts.c under MCTS_STATIC.
 *
 * @param m The move to check.
 * @return true if the move is null, false otherwise.
 */
MCTS_STATIC bool
is_move_null (const Move *m)
{
  return m->queen_src.col == m->queen_dst.col
         && m->queen_src.row == m->queen_dst.row;
}

/**
 * @brief Aggregates search results found using MCTS, by simply taking the move
 * that has been explored the most between multiple threads, if any.
 *
 * @param data The MCTS config and memory
 * @param num_threads Number of threads used for the calculation
 * @return Move Best move found in that iteration
 */
MCTS_STATIC Move
merge_and_get_best (const MCTS_Data *data, int num_threads)
{
  // TODO : change to use all nodes instead (though, if a node has been
  // explored a lot on one thread, it *should* most likely had been explored
  // once by another thread), especially due to sorting thanks to UCT (and ML
  // doesn't need it)

  MCTS_Node *master_root = data->roots[0];
  MCTS_Node *best_child = NULL;
  long max_total_visits = -1;

  // Iterate through moves found by the first thread
  for (MCTS_Node *curr = master_root->first_child; curr != NULL;
       curr = curr->next_sibling)
    {
      long combined_visits = curr->visit_count;

      // Look for this same move in other threads
      for (int i = 1; i < num_threads; i++)
        {
          MCTS_Node *other_root = data->roots[i];
          for (MCTS_Node *other_c = other_root->first_child; other_c != NULL;
               other_c = other_c->next_sibling)
            {
              if (moves_are_equal (curr->move_from_parent,
                                   other_c->move_from_parent))
                {
                  combined_visits += other_c->visit_count;
                  break;
                }
            }
        }

      if (combined_visits > max_total_visits)
        {
          max_total_visits = combined_visits;
          best_child = curr;
        }
    }

  return best_child ? best_child->move_from_parent : (Move){ 0 };
}

/**
 * @brief Select a node to expand from the given root node using the provided
 * selection strategy.
 * @param selector The selection strategy to use for selecting child nodes.
 * @param root The root node of the MCTS tree to start selection from.
 * @return The selected MCTS_Node to expand.
 */
MCTS_STATIC MCTS_Node *
select_node (const ISelector *selector, MCTS_Node *root)
{
  MCTS_Node *node = root;

  while (node->is_fully_expanded && node->first_child != NULL)
    {
      node = selector->vtable->select_child (selector, node);
    }

  return node;
}

/**
 * @brief Backpropagates the result of a rollout up the MCTS tree, updating
 * visit counts and scores for each node along the path from the given node to
 * the root.
 * @param node The MCTS_Node from which to start backpropagation.
 * @param result The result of the rollout.
 */
MCTS_STATIC void
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
 * @brief Gets the win probability of a current bitboard state configuration.
 * @param state The game state.
 * @param starting_player_was_white True if the starting player is white, False
 * otherwise.
 * @param result The probability of win in that configuration.
 */
MCTS_STATIC float
get_mcts_win_probability (const BitboardState *state,
                          bool starting_player_was_white)
{
  float raw_score = eval_me (state);

  if (!starting_player_was_white)
    {
      raw_score = -raw_score;
    }
  // Simply is 4*(n-1)
  const float MAX_MOBILITY_DIFF = (float)((state->size - 1) << 2);
  float win_probability = 0.5f + (raw_score / (2.0f * MAX_MOBILITY_DIFF));

  if (win_probability > 1.0f)
    return 1.0f;
  if (win_probability < 0.0f)
    return 0.0f;

  return win_probability;
}

/**
 * @brief Performs a random rollout (simulation) from the given state until the
 * game ends, and returns the result from the perspective of the player to move
 * at the start of the rollout.
 * @param state The initial BitboardState from which to start the rollout.
 * @return A float value representing the result of the rollout: 1.0 if the
 * starting player wins, 0.0 if the starting player loses.
 */
MCTS_STATIC float
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

/**
 * @brief Allocates a node in MCTS. It is a bit costly (calloc) : avoid calling
 * it more than necessary.
 *
 * @param move The move to be played. (Can be nulled out)
 * @param current_state The state of the board before the move was played.
 * @param parent The parent node.
 * @return MCTS_Node* The MCTS node.
 */
MCTS_STATIC MCTS_Node *
alloc_mcts_node (const Move *move, const BitboardState *current_state,
                 MCTS_Node *parent)
{
  MCTS_Node *node = (MCTS_Node *)calloc (1, sizeof (MCTS_Node));
  if (!node)
    return NULL;

  node->parent = parent;
  node->visit_count = 0;
  node->score = 0.0f;
  node->first_child = NULL;
  node->next_sibling = NULL;
  node->is_fully_expanded = false;
  node->move_from_parent = move ? *move : (Move){ 0 };
  node->state
      = parent ? apply_move_bitboard (&parent->state, *move) : *current_state;
  return node;
}

/**
 * @brief Expands the given MCTS node by adding a new child node for an untried
 * move. If there aren't any, it will free the untried moves.
 * @param parent The MCTS node to expand.
 * @return The newly created child node corresponding to an untried move, or
 * the parent node if all moves have already been tried (fully expanded).
 */
MCTS_STATIC MCTS_Node *
expand (MCTS_Node *parent)
{
  if (parent->is_fully_expanded
      || bitboard_state_is_game_over (&parent->state))
    return parent;

  // Cache moves into the parent
  if (parent->untried_moves == NULL)
    {
      Move temp_moves[get_move_limit (&parent->state)];
      int total = get_legal_moves (&parent->state, temp_moves);

      if (total == 0)
        {
          parent->is_fully_expanded = true;
          return parent;
        }

      // Allocate exactly what we need
      parent->untried_moves = (Move *)malloc (total * sizeof (Move));
      memcpy (parent->untried_moves, temp_moves, total * sizeof (Move));
      parent->num_untried = total;
    }

  // Take the next untried move.
  parent->num_untried--;
  Move next_move = parent->untried_moves[parent->num_untried];

  // Create the new child
  MCTS_Node *new_node = alloc_mcts_node (&next_move, NULL, parent);
  if (!new_node)
    {
      parent->num_untried++;
      return NULL;
    }
  new_node->next_sibling = parent->first_child;
  parent->first_child = new_node;

  // Free if we've reached the end to free space during computation
  if (parent->num_untried == 0)
    {
      parent->is_fully_expanded = true;
      free (parent->untried_moves);
      parent->untried_moves = NULL;
    }

  return new_node;
}

/**
 * @brief Recursively frees the memory allocated for the MCTS tree starting
 * from the given node.
 * @param node The root node of the subtree to free. This function will free
 * all descendants of this node as well.
 * @note Should not be called twice on the same node, as it will lead to
 * double-free errors. Please ensure that the caller sets the pointer to NULL
 * after freeing if there's a possibility of it being freed again.
 */
MCTS_STATIC void
free_mcts_tree (MCTS_Node *node)
{
  if (node == NULL)
    return;

  if (node->untried_moves != NULL) // in case computation was cut off by time
    {
      free (node->untried_moves);
    }

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
 * @brief MAIN MCTS FUNCTION
 * MCTS is simply :
 * - Select : Select a path direction to explore (a node)
 * - Expand : Expand the tree by adding the selected node (if it didn't exist)
 * - Rollout : Simulate a game until you reach a certain depth, from which you
 * calculate a score
 * - Backpropagate : Update visits and scores of the nodes of the path you
 * explored
 * @param self The AI_Interface instance containing the context for the MCTS
 * algorithm.
 * @param state The current GameState to evaluate and find the best move for.
 * @return The best Move for the current player based on the MCTS evaluation.
 */
MCTS_STATIC Move
mcts_get_best_move (const AI_Interface *self, GameState state)
{
  MCTS_Data *data = (MCTS_Data *)self->context;
  BitboardState root_state = sync_from_logic (&state);
  int num_threads = data->max_workers;
  unsigned long start_time = get_time_ms ();

  // Allocate roots and data for each thread
  data->roots = calloc (num_threads, sizeof (MCTS_Node *));

  int thread_iterations[num_threads];
  for (int i = 0; i < num_threads; i++)
    thread_iterations[i] = 0;

#pragma omp parallel num_threads(num_threads)
  {
    int tid = omp_get_thread_num ();

    // Each thread creates its own independent root
    data->roots[tid] = alloc_mcts_node (NULL, &root_state, NULL);
    data->roots[tid]->visit_count = 1;
    // Local iteration counter
    uint32_t local_iterations = 0;

    // Main MCTS Loop
    while (((get_time_ms () - start_time) < data->time_constraint)
           && (local_iterations < (data->max_iterations / num_threads)))
      {
        MCTS_Node *picked
            = select_node (data->strategy_select, data->roots[tid]);
        MCTS_Node *expanded_child = expand (picked);

        if (!expanded_child)
          break;

        float result = rollout (expanded_child->state);
        backpropagate (expanded_child, 1.0f - result);

        local_iterations++;
      }
    thread_iterations[tid] = local_iterations;
  }

  int total_iterations = 0;
  for (int i = 0; i < num_threads; i++)
    {
      total_iterations += thread_iterations[i];
    }

  // Merge threads results
  Move best_move = merge_and_get_best (data, num_threads);

  if (best_move.queen_src.col == 0 && best_move.queen_src.row == 0
      && best_move.queen_dst.col == 0 && best_move.queen_dst.row == 0
      && best_move.arrow_dst.col == 0 && best_move.arrow_dst.row == 0)
    {
      best_move = get_direct_random_move (
          &root_state); // Fallback to avoid illegal moves.
    }

  printf ("[MCTS] Threads: %d | Total Iterations: %d | Best move : (%d, %d) "
          "-> (%d, %d)\n",
          num_threads, total_iterations, best_move.queen_src.row,
          best_move.queen_src.col, best_move.queen_dst.row,
          best_move.queen_dst.col);

  // Cleanup
  for (int i = 0; i < num_threads; i++)
    {
      free_mcts_tree (data->roots[i]);
    }
  free (data->roots);

  return best_move;
}

/**
 * @brief Cleans up the MCTS AI by freeing the selector strategy
 * and the main data context.
 * @param self The AI_Interface instance to clean, which holds the context
 * for the MCTS algorithm.
 */
MCTS_STATIC void
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
 * (default inf).
 * @note max_workers: Maximum number of worker threads for parallel MCTS
 * (default 1).
 */
MCTS_STATIC void
mcts_init (AI_Interface *self, const ConfigMap *params)
{
  MCTS_Data *data = (MCTS_Data *)malloc (sizeof (MCTS_Data));
  if (!data)
    return;

  data->time_constraint = DEFAULT_TIME_CONSTRAINT;
  data->max_iterations = DEFAULT_MAX_ITERATIONS;
  data->max_workers = DEFAULT_MAX_WORKERS;

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
  printf ("[MCTS] Init OK (Time: %ldms)\n", data->time_constraint);
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