#ifndef AI_FACTORY_H
#define AI_FACTORY_H

#include "ai_interface.h"

AI_Interface create_ai(const char *name, ConfigMap params);

#endif