#include "ai_interface.h"
#include "evaluator.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
random_init (AI_Interface *self, ConfigMap *params)
{
  (void)self;
  (void)params;
  printf ("[Random] Init OK\n");
}

static void
random_clean (AI_Interface *self)
{
  (void)self;
  printf ("[Random] Clean OK\n");
}

static Move
random_get_best_move (AI_Interface *self, GameState state)
{
  (void)self;
  Move best_move = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
  BitboardState bs = sync_from_logic (&state);
  Move moves_array[get_move_limit (bs)];
  int count = get_legal_moves (bs, moves_array);
  if (count > 0)
    {
      int random_index = rand () % count;
      best_move = moves_array[random_index];
    }
  return best_move;
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