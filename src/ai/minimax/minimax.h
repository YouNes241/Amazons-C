#ifndef MINIMAX_H
#define MINIMAX_H

#include "ai_interface.h"

typedef struct
{
  int max_depth;
  int time_limit;
} Minimax_Data;

extern const AI_VTable minimax_vtable;

void create_minimax_ai (AI_Interface *ai);

#endif