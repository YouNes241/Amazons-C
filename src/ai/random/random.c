#include "ai_interface.h"
#include "evaluator.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Inits the random AI, which doesn't need any parameter.
 *
 * @param self any
 * @param params any
 */
static void
random_init (AI_Interface *self, const ConfigMap *params)
{
  (void)self;
  (void)params;
  printf ("[Random] Init OK\n");
}

/**
 * @brief Cleans the random AI, which doesn't have any memory to free.
 *
 * @param self any
 */
static void
random_clean (AI_Interface *self)
{
  (void)self;
  printf ("[Random] Clean OK\n");
}

/**
 * @brief Gets a random move for the current state, without any evaluation.
 *
 * @param self any
 * @param state Current game state.
 * @return Move
 */
static Move
random_get_best_move (const AI_Interface *self, GameState state)
{
  (void)self;
  BitboardState bstate = sync_from_logic (&state);
  return get_direct_random_move (&bstate);
}

const AI_VTable random_vtable = { .init = random_init,
                                  .clean = random_clean,
                                  .get_best_move = random_get_best_move };

void
create_random_ai (AI_Interface *ai)
{
  ai->vtable = &random_vtable;
  ai->context = NULL;
}