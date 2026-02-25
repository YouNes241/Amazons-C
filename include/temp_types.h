#ifndef TEMP_TYPES_H
#define TEMP_TYPES_H
#include "bitboard.h"
#include "game_types.h"
#include <stdbool.h>

typedef struct
{
  const char *key;
  const char *value;
} Param;
typedef struct
{
  Param *list;
  int count;
} ConfigMap;

typedef struct
{
  Bitboard white_queens;
  Bitboard black_queens;
  Bitboard arrows;
  uint8_t size;
  bool white_to_move;
} BitboardState;
#endif