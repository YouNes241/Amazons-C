#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H

#ifdef __cplusplus
#include <type_traits>
#endif

#include "bitboard_state.h"
#include "game_types.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct AI_Interface AI_Interface;

typedef struct
{
  void (*init) (AI_Interface *self, const ConfigMap *params);
  void (*clean) (AI_Interface *self);
  Move (*get_best_move) (const AI_Interface *self, GameState state);
} AI_VTable;

struct AI_Interface
{
  const AI_VTable *vtable;
  void *context;
};

static inline void
_check_param_int (Param p, const char *key, int *target)
{
  if (strcmp (p.key, key) == 0)
    {
      char *endptr;
      long val = strtol (p.value, &endptr, 10);
      if (*endptr == '\0' && val >= INT_MIN && val <= INT_MAX)
        *target = (int)val;
      else if (*endptr == '\0' && val > INT_MAX)
        *target = INT_MAX;
      else if (*endptr == '\0' && val < INT_MIN)
        *target = INT_MIN;
      else
        *target = 0;
    }
}

static inline void
_check_param_uint32 (Param p, const char *key, uint32_t *target)
{
  if (strcmp (p.key, key) == 0 && p.value != NULL)
    {
      char *endptr;
      unsigned long val = strtoul (p.value, &endptr, 10);
      if (*endptr == '\0' && val <= UINT32_MAX)
        *target = (uint32_t)val;
      else if (*endptr == '\0' && val > UINT32_MAX)
        *target = UINT32_MAX;
      else
        *target = 0;
    }
}

static inline void
_check_param_ul (Param p, const char *key, unsigned long *target)
{
  if (strcmp (p.key, key) == 0)
    {
      char *endptr;
      unsigned long val = strtoul (p.value, &endptr, 10);
      if (*endptr == '\0')
        *target = val;
      else
        *target = 0;
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
_check_param_char (Param p, const char *key, const char **target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = p.value;
    }
}

#ifdef __cplusplus

#define check_param(p, key, target)                                           \
  do                                                                          \
    {                                                                         \
      if (std::is_same<decltype (target), int *>::value)                      \
        {                                                                     \
          _check_param_int ((p), (key), (int *)(target));                     \
        }                                                                     \
      else if (std::is_same<decltype (target), unsigned long *>::value)       \
        {                                                                     \
          _check_param_ul ((p), (key), (unsigned long *)(target));            \
        }                                                                     \
      else if (std::is_same<decltype (target), uint32_t *>::value)            \
        {                                                                     \
          _check_param_uint32 ((p), (key), (uint32_t *)(target));             \
        }                                                                     \
      else if (std::is_same<decltype (target), bool *>::value)                \
        {                                                                     \
          _check_param_bool ((p), (key), (bool *)(target));                   \
        }                                                                     \
      else if (std::is_same<decltype (target), const char **>::value)         \
        {                                                                     \
          _check_param_char ((p), (key), (const char **)(target));            \
        }                                                                     \
    }                                                                         \
  while (0)

#else
/**
 * @brief Macro to check and set a parameter value based on its type. This
 * macro uses C11's _Generic to dispatch to the correct type-specific function
 * for checking and setting the parameter.
 * @param p The parameter to check, of type Param.
 * @param key The key to match against the parameter's key.
 * @param target A pointer to the variable where the parameter value should be
 * stored if the key matches.
 * @note Passing "NULL" to any parameter will cause undefined behavior (mostly
 * a crash).
 *
 */
#define check_param(p, key, target)                                           \
  _Generic ((target),                                                         \
      int *: _check_param_int,                                                \
      unsigned long *: _check_param_ul,                                       \
      uint32_t *: _check_param_uint32,                                        \
      bool *: _check_param_bool,                                              \
      const char **: _check_param_char) ((p), (key), (target))
#endif

#endif // AI_INTERFACE_H