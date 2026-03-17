#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H
#include "bitboard_state.h"
#include "game_types.h"
#include <stdlib.h>
#include <string.h>

typedef struct AI_Interface AI_Interface;

typedef struct
{
  void (*init) (struct AI_Interface *self, ConfigMap *params);
  void (*clean) (AI_Interface *self);
  Move (*get_best_move) (AI_Interface *self, GameState state);
} AI_VTable;

struct AI_Interface
{
  const AI_VTable *vtable;
  void *context;
};

/**
 * @brief Checks if a given parameter key matches the expected key and, if so,
 * parses its value as an integer and assigns it to the target variable.
 * @param p The parameter to check, containing a key and a value.
 * @param key The expected key to match against the parameter's key.
 * @param target The value to set based on the parameter's valuation. *
 */
static inline void
check_param (Param p, const char *key, int *target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = atoi (p.value);
    }
}

#endif