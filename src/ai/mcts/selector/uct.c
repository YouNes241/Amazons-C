#include "i_selector.h"
#include "mcts_types.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @struct UCT_Data
 * @brief Contains the parameters of UCT
 * @var UCT_Data::exploration_constant
 * Constant used to give more value to exploration
 */
typedef struct
{
  double exploration_constant;
} UCT_Data;

/**
 * @brief Selects a child node of the given parent node using the UCT (Upper
 * Confidence Bound for Trees) formula.
 * @param self The ISelector instance containing the context for UCT selection.
 * @param parent The MCTS_Node for which to select a child node.
 * @return The selected child MCTS_Node based on the highest UCT score, or NULL
 * if there are no children.
 * @note The UCT score is calculated as: (UCB1)
 * UCT = (score / visit_count) + (exploration_constant *
 * sqrt(log(parent_visit_count+1) / visit_count)),
 * where score is the total score of the child node, visit_count is the number
 * of times the child node has been visited, parent_visit_count is the number
 * of times the parent node has been visited, and exploration_constant is a
 * parameter that controls the balance between exploration and exploitation.
 */
static MCTS_Node *
uct_select_child (const ISelector *self, const MCTS_Node *parent)
{
  UCT_Data *data = (UCT_Data *)self->context;
  MCTS_Node *current = parent->first_child;
  MCTS_Node *best_child = NULL;
  double best_uct_score = -INFINITY;
  double parent_score = data->exploration_constant
                        * sqrt (log ((double)parent->visit_count + 1.0f));

  while (current != NULL)
    {
      if (current->visit_count == 0)
        {
          return current;
        }

      double exploitation
          = (double)current->score / (double)current->visit_count;
      double exploration
          = (parent_score * sqrt (1.0 / (current->visit_count)));

      double uct_score = exploitation + exploration;

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
create_select_uct (ISelector *selector, double c)
{
  UCT_Data *data = malloc (sizeof (UCT_Data));
  data->exploration_constant = c;

  selector->vtable = &uct_vtable;
  selector->context = data;
}