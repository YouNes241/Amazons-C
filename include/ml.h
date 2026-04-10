#ifdef TF_IS_USABLE
#ifndef ML_H
#define ML_H

#include "i_selector.h"
#include "tf_wrapper.h"

void create_select_ml (ISelector *selector, double c_puct,
                       const char *model_path);

#endif // ML_H
#endif