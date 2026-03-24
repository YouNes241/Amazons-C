#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H
#include "bitboard_state.h"
#include "game_types.h"
#include <stdlib.h>
#include <string.h>
#define check_param(p, key, target)                                           \
  _Generic ((target),                                                         \
      int *: _check_param_int,                                                \
      unsigned long *: _check_param_ul,                                       \
      bool *: _check_param_bool,                                              \
      const char **: _check_param_char) (p, key, target)

typedef struct AI_Interface AI_Interface;

/**
 * @struct AI_VTable
 * @brief Contains the different functions usable with any AI.
 * @var AI_VTable::init
 * Initialisation function, takes parameters and itself in order to
 * initialize itself.
 * @var AI_VTable::clean
 * Cleans the memory of the AI.
 * @var AI_VTable::get_best_move
 * Gets the best move using the specified AI.
 */
typedef struct
{
  void (*init) (struct AI_Interface *self, ConfigMap *params);
  void (*clean) (AI_Interface *self);
  Move (*get_best_move) (AI_Interface *self, GameState state);
} AI_VTable;

/**
 * @struct AI_Interface
 * @brief Acts as an interface between the developer and the AI methods.
 * @var AI_Interface::vtable
 * Contains init, clean and get_best_move functions.
 * @var AI_Interface::context
 * Context of the AI (parameters)
 */
struct AI_Interface
{
  const AI_VTable *vtable;
  void *context;
};

/**
 * @brief Checks if a given parameter key matches the expected key and, if
 * so, parses its value as an integer and assigns it to the target
 * variable.
 * @param p The parameter to check, containing a key and a value.
 * @param key The expected key to match against the parameter's key.
 * @param target The value to set based on the parameter's valuation. *
 */
static inline void
_check_param_int (Param p, const char *key, int *target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = atoi (p.value);
    }
}

static inline void
_check_param_ul (Param p, const char *key, unsigned long *target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = strtoul (p.value, NULL, 10);
    }
}

static inline void
_check_param_bool (Param p, const char *key, bool *target)
{
  if (strcmp (p.key, key) == 0)
    {
      int val = atoi (p.value);
      *target = (val != 0);
    }
}

static inline void
_check_param_u64 (Param p, const char *key, uint64_t *target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = (uint64_t)strtoull (p.value, NULL, 10);
    }
}

static inline void
_check_param_char (Param p, const char *key, const char **target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = p.value;
    }
}

#endif