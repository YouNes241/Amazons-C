#include "ai_factory.h"
#include <stdio.h>
#include <string.h>

typedef struct
{
  const char *name;
  void (*create_func) (AI_Interface *);
} AI_RegistryEntry;

// Modify this table if you ever add an ai
extern void create_random_ai (AI_Interface *ai);
extern void create_minimax_ai (AI_Interface *ai);
extern void create_mcts_ai (AI_Interface *ai);
static const AI_RegistryEntry AI_REGISTRY[]
    = { { "random", create_random_ai },
        { "minimax", create_minimax_ai },
        { "mcts", create_mcts_ai } };

static const size_t AI_REGISTRY_SIZE
    = sizeof (AI_REGISTRY) / sizeof (AI_REGISTRY[0]);

AI_Interface
create_ai (const char *name, ConfigMap params)
{
  AI_Interface ai;
  ai.vtable = NULL;
  ai.context = NULL;

  for (size_t i = 0; i < AI_REGISTRY_SIZE; i++)
    {
      if (strcmp (name, AI_REGISTRY[i].name) == 0)
        {
          printf ("[Factory] Creating %s AI...\n", name);
          AI_REGISTRY[i].create_func (&ai);

          if (ai.vtable && ai.vtable->init)
            {
              ai.vtable->init (&ai, &params);
            }
          return ai;
        }
    }

  // Should not be reached.
  fprintf (stderr, "[Factory] AI unrecognized : '%s'\n", name);
  return ai;
}