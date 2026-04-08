#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "bitboard.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  const char *key;
  const char *value;
} Param;

typedef struct
{
  int count;
  Param *list;
} ConfigMap;

// Represents the two players
typedef enum
{
  PLAYER_WHITE = 0,
  PLAYER_BLACK = 1,
  NO_PLAYER = 2 // Used for error cases or when there is no winner yet
} PlayerColor;

typedef enum
{
  CONTENT_EMPTY = 0,
  CONTENT_WHITE_QUEEN,
  CONTENT_BLACK_QUEEN,
  CONTENT_ARROW
} CellContent;

typedef enum
{
  GAME_IN_PROGRESS, // The game is still ongoing
  GAME_OVER_WHITE_WINS,
  GAME_OVER_BLACK_WINS,
  GAME_ERROR // Error case (for example network problem)
} GameStatus;

// Represents coordinates on the board
typedef struct
{
  uint8_t row;
  uint8_t col;
} Position;

// Represents a complete move played by a user, this structure will be sent
// over the network
typedef struct
{
  Position queen_src;
  Position queen_dst;
  Position arrow_dst;
} Move;

typedef struct
{
  int top;
  int capacity;
  Move *data;
} MoveStack;
// Represents the public game state (what other modules are allowed to see)
typedef struct
{
  uint8_t size;
  bool blitz_mode;
  uint32_t turn_count; // Counter of the number of turns played
  PlayerColor current_player;
  GameStatus status;
  CellContent **board; // A matrix for the board, used by the view
  double blitz_turn_start;
  double white_time_left; // seconds
  double black_time_left;
  MoveStack undo_stack;
  MoveStack redo_stack;

} GameState;

#endif
