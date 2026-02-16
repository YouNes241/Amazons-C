#ifndef RANDOM_H
#define RANDOM_H

#include "ai_interface.h"

extern const AI_VTable random_vtable;

void create_random_ai (AI_Interface *ai);

#endif