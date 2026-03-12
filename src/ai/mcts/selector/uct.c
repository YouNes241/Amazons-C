#include "i_selector.h"
#include "mcts_types.h"
#include <math.h>
#include <stdlib.h>

typedef struct
{
  float exploration_constant;
} UCT_Data;

/**
 * @brief Selects a child node of the given parent node using the UCT (Upper
 * Confidence Bound for Trees) formula.
 * @param self The ISelector instance containing the context for UCT selection.
 * @param parent The MCTS_Node for which to select a child node.
 * @return The selected child MCTS_Node based on the highest UCT score, or NULL
 * if there are no children.
 * @note The UCT score is calculated as:
 * UCT = (score / visit_count) + (exploration_constant *
 * sqrt(log(parent_visit_count) / visit_count)),
 * where score is the total score of the child node, visit_count is the number
 * of times the child node has been visited, parent_visit_count is the number
 * of times the parent node has been visited, and exploration_constant is a
 * parameter that controls the balance between exploration and exploitation.
 */
static MCTS_Node *
uct_select_child (ISelector *self, MCTS_Node *parent)
{
  UCT_Data *data = (UCT_Data *)self->context;
  int unvisited_count = 0;
  MCTS_Node *chosen_unvisited = NULL;
  MCTS_Node *current = parent->first_child;

  while (current != NULL)
    {
      if (current->visit_count == 0)
        {
          /* reservoir sampling to choose uniformly */
          unvisited_count++;
          if ((rand () % unvisited_count) == 0)
            chosen_unvisited = current;
        }
      current = current->next_sibling;
    }

  if (chosen_unvisited != NULL)
    return chosen_unvisited;

  /*
   * All children have been visited; compute the usual UCT value and pick the
   * child with the highest score.
   */
  MCTS_Node *best_child = NULL;
  float best_uct_score = -INFINITY;
  current = parent->first_child;

  while (current != NULL)
    {
      float exploitation = (float)current->score / current->visit_count;
      float exploration
          = data->exploration_constant
            * sqrtf (logf ((float)parent->visit_count) / current->visit_count);
      float uct_score = exploitation + exploration;

      if (uct_score > best_uct_score)
        {
          best_uct_score = uct_score;
          best_child = current;
        }
      current = current->next_sibling;
    }
  return best_child;
}

/**
 * @brief Cleans up the UCT selector by freeing its context data.
 * @param self The ISelector instance to clean, which holds the context for UCT
 * selection.
 */
static void
uct_clean (ISelector *self)
{
  free (self->context);
}

static const ISelector_VTable uct_vtable
    = { .select_child = uct_select_child, .clean = uct_clean };

void
create_select_uct (ISelector *selector, float c)
{
  UCT_Data *data = malloc (sizeof (UCT_Data));
  data->exploration_constant = c;

  selector->vtable = &uct_vtable;
  selector->context = data;
}