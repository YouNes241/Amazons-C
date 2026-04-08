#include "ml.h"
#include "bitboard.h" // Assuming you have a way to read the board
#include "i_selector.h"
#include "mcts.h"
#include "tf_wrapper.h"

typedef struct
{
  double c_puct;
  AmazonsAI *tf_brain;
} ML_Selector_Data;

#ifdef TF_IS_USABLE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Converts a bitboard into a 4-channel float tensor for the ML model.
 * Channels: [White Queens, Black Queens, Arrows, Empty]
 */
static void
bitboard_to_tensor (BitboardState *state, float *tensor)
{
  // Initialize everything to 0
  memset (tensor, 0, 11 * 11 * 4 * sizeof (float));

  for (int r = 0; r < 11; r++)
    {
      for (int c = 0; c < 11; c++)
        {
          int sq = r * 11 + c;
          int idx = sq * 4;

          // Replace 'test_bit' with whatever function you use in bitboard.h
          if (bitboard_get_bit (state->white_queens, sq))
            tensor[idx + 0] = 1.0f;
          else if (bitboard_get_bit (state->black_queens, sq))
            tensor[idx + 1] = 1.0f;
          else if (bitboard_get_bit (state->arrows, sq))
            tensor[idx + 2] = 1.0f;
          else
            tensor[idx + 3] = 1.0f;
        }
    }
}

/**
 * @brief Selects a child using the PUCT formula, replacing standard UCT.
 */
static MCTS_Node *
ml_select_child (const ISelector *self, const MCTS_Node *parent)
{
  ML_Selector_Data *data = (ML_Selector_Data *)self->context;
  MCTS_Node *current = parent->first_child;
  MCTS_Node *best_child = NULL;

  double best_puct_score = -INFINITY;
  double sqrt_parent_visits = sqrt ((double)parent->visit_count);

  while (current != NULL)
    {
      // Standard MCTS rule: explore unvisited nodes first
      if (current->visit_count == 0)
        {
          return current;
        }

      if (current->ai_eval <= -2.0f)
        {
          float tensor[484];
          bitboard_to_tensor (&current->state, tensor);
          current->ai_eval = predict_board_value (data->tf_brain, tensor);
        }

      // 2. PERSPECTIVE CORRECTION
      // Assuming you use BCE (White = 1.0, Black = 0.0)
      // Assuming parent->player_to_move is 1 for White, 2 for Black
      double q_nn = current->ai_eval;
      double q_rollout = (double)current->score / (double)current->visit_count;

      bool is_black_turn = false; // Adapt to your struct!

      if (is_black_turn)
        {
          q_nn
              = 1.0 - q_nn; // Invert NN: 0.9 (White win) becomes 0.1 for Black
          // If current->score is also absolute (White wins), invert it too:
          q_rollout = 1.0 - q_rollout;
        }

      // 3. THE FIXED FORMULA (Value-Guided MCTS)
      // Exploitation: Average the Rollout Winrate and the Neural Net Value
      // (You can weight these! e.g., 0.8 * q_nn + 0.2 * q_rollout)
      double exploitation = (q_rollout + q_nn) / 2.0;

      // Exploration: Standard UCB1 since you lack a Policy (Probability) head
      double exploration
          = data->c_puct * (sqrt_parent_visits / (1.0 + current->visit_count));

      double puct_score = exploitation + exploration;

      if (puct_score > best_puct_score)
        {
          best_puct_score = puct_score;
          best_child = current;
        }

      current = current->next_sibling;
    }

  return best_child;
}

static void
ml_selector_clean (ISelector *self)
{
  ML_Selector_Data *data = (ML_Selector_Data *)self->context;
  // Make sure to free the TF session safely before freeing the context
  if (data->tf_brain)
    {
      free_amazons_ai (data->tf_brain);
    }
  free (data);
}

const ISelector_VTable ml_vtable
    = { .select_child = ml_select_child, .clean = ml_selector_clean };

void
create_select_ml (ISelector *selector, double c_puct, const char *model_path)
{
  ML_Selector_Data *data = malloc (sizeof (ML_Selector_Data));
  data->c_puct = c_puct;

  data->tf_brain = load_amazons_ai (model_path);

  selector->vtable = &ml_vtable;
  selector->context = data;
}

#else

void
create_select_ml (ISelector *selector, double c_puct, const char *model_path)
{
  (void)selector;
  (void)c_puct;
  (void)model_path;
}
#endif
