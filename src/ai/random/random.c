#include "ai_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
random_init (AI_Interface *self, ConfigMap *params)
{
  printf ("[Random] Init OK\n");
}

static void
random_clean (AI_Interface *self)
{
  printf ("[Random] Clean OK\n");
}

static Move
random_get_best_move (AI_Interface *self, GameState state)
{
  Move best_move = { 0, 0, 0, 0, 0, 0 };
  // TODO : using bitmap, gen random move
  return best_move;
}

static const AI_VTable random_vtable
    = { .init = random_init,
        .clean = random_clean,
        .get_best_move = random_get_best_move };

void
create_random_ai (AI_Interface *ai)
{
  ai->vtable = &random_vtable;
  ai->context = NULL;
}