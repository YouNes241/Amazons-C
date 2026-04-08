#ifndef SELECTOR_H
#define SELECTOR_H

#include "i_selector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  SELECTOR_UCT,

#ifdef TF_IS_USABLE
  SELECTOR_ML
#endif
} SelectorID;

typedef struct
{
  SelectorID id;
  const char *name;
} SelectorMap;

// Le dictionnaire global
static const SelectorMap SELECTOR_DICT[] = { { SELECTOR_UCT, "uct" },
#ifdef TF_IS_USABLE
                                             { SELECTOR_ML, "ml" }
#endif
};

/**
 * @brief Get the selector id from the name of said selector
 * @note Defaults to UCT if there is an error.
 *
 * @param name Name of selector
 * @return SelectorID The ID of said selector
 */
static inline SelectorID
get_selector_id_from_name (const char *name)
{
  int dict_size = sizeof (SELECTOR_DICT) / sizeof (SELECTOR_DICT[0]);

  if (name)
    {
      for (int i = 0; i < dict_size; i++)
        {
          if (strcmp (name, SELECTOR_DICT[i].name) == 0)
            return SELECTOR_DICT[i].id;
        }
      fprintf (stderr, "[Warning] Unknown selector name '%s'.\n", name);
    }
  else
    {
      fprintf (stderr, "[Warning] Selector name is NULL.\n");
    }

  fprintf (stderr,
           "[Info] Falling back to 'uct'. Correct choices of selectors are: ");
  for (int i = 0; i < dict_size; i++)
    {
      fprintf (stderr, "\"%s\"%s", SELECTOR_DICT[i].name,
               (i < dict_size - 1) ? ", " : "\n");
    }

  return SELECTOR_UCT;
}

/**
 * @brief Retrieves the string name corresponding to a SelectorID.
 * @param id The SelectorID to look up (e.g., SELECTOR_UCT, SELECTOR_ML).
 * @return The corresponding name as a string. Defaults to "uct" if the ID is
 * unrecognized.
 */
static inline const char *
get_selector_name_from_id (SelectorID id)
{
  int dict_size = sizeof (SELECTOR_DICT) / sizeof (SELECTOR_DICT[0]);
  for (int i = 0; i < dict_size; i++)
    {
      if (SELECTOR_DICT[i].id == id)
        return SELECTOR_DICT[i].name;
    }

  return "uct";
}

/**
 * @brief Retrieves an Selector function pointer based on the provided id.
 * @param name An id of the desired selector (UCT, ML)
 * @return A Selection function pointer corresponding to the name. Defaults
 * to UCT if the name is unrecognized.
 */
ISelector get_selector (SelectorID id, double c_uct, double c_puct,
                        const char *model_path);
#endif