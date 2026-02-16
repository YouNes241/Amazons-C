#include "minimax.h"
#include "ai_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int DEFAULT_MAX_DEPTH = 3;
static const int DEFAULT_TIME_LIMIT = 5;

<<<<<<< Updated upstream
typedef struct
{
  int max_depth;
  int time_limit;
} Minimax_Data;

=======
>>>>>>> Stashed changes
static void
check_param (Param p, const char *key, int *target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = atoi (p.value);
    }
}

static void
minimax_init (AI_Interface *self, ConfigMap *params)
{
  Minimax_Data *data = (Minimax_Data *)malloc (sizeof (Minimax_Data));
  if (!data)
    return;

  data->max_depth = DEFAULT_MAX_DEPTH;
  data->time_limit = DEFAULT_TIME_LIMIT;

  if (params != NULL)
    {
      for (int i = 0; i < params->count; i++)
        {
          check_param (params->list[i], "depth", &data->max_depth);
          check_param (params->list[i], "time_limit", &data->time_limit);
        }
    }

  self->context = data;
  printf ("[Minimax] Init OK\n");
}

static void
minimax_clean (AI_Interface *self)
{
  Minimax_Data *data = (Minimax_Data *)self->context;

  free (data);
  self->context = NULL;

  printf ("[Minimax] Clean OK\n");
}

static Move
minimax_get_best_move (AI_Interface *self, GameState state)
{
  Minimax_Data *data = (Minimax_Data *)self->context;

  printf ("[Minimax] Searching a move (Depth=%d)...\n", data->max_depth);

  Move best_move = { 0, 0, 0, 0, 0, 0 };

  return best_move;
}

const AI_VTable minimax_vtable = { .init = minimax_init,
                                   .clean = minimax_clean,
                                   .get_best_move = minimax_get_best_move };

void
create_minimax_ai (AI_Interface *ai)
{
  ai->vtable = &minimax_vtable;
  ai->context = NULL;
}