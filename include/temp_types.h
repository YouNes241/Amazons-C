#ifndef TEMP_TYPES_H
#define TEMP_TYPES_H

typedef struct
{
  const char *key;
  const char *value;
} Param;
typedef struct
{
  int r1, c1, r2, c2, ar, ac;
} Move;
typedef struct
{
  Param *list;
  int count;
} ConfigMap;
typedef struct
{
} GameState;
#endif