#ifndef MINIMAX_H
#define MINIMAX_H

#include "ai_interface.h"
#include "evaluator.h"

typedef struct
{
  int max_depth;
  int time_limit;
  Evaluator evaluate;
} Minimax_Data;

extern const AI_VTable minimax_vtable;

void create_minimax_ai (AI_Interface *ai);

int get_legal_moves (BitboardState state, Move *moves_array);

#endif