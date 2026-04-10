#include "selector.h"
#ifdef TF_IS_USABLE
#include "ml.h"
#endif
#include "uct.h"

ISelector
get_selector (SelectorID id, double c_uct, double c_puct,
              const char *model_path)
{
  ISelector selector = { 0 };

#ifdef TF_IS_USABLE
  // If TF is enabled and ML is requested, build it and return immediately.
  if (id == SELECTOR_ML)
    {
      create_select_ml (&selector, c_puct, model_path);
      return selector;
    }
#else
  (void)id;
  (void)c_puct;
  (void)model_path;
#endif

  // Default case: If UCT was requested, OR if ML was requested but TF is
  // disabled.
  create_select_uct (&selector, c_uct);

  return selector;
}