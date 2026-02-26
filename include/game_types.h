#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Represents the two players
typedef enum
{
  PLAYER_WHITE = 0,
  PLAYER_BLACK = 1
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

// Represents the public game state (what other modules are allowed to see)
typedef struct
{
  uint8_t size;
  CellContent **board; // A matrix for the board, used by the view
  PlayerColor current_player;
  GameStatus status;
  uint32_t turn_count; // Counter of the number of turns played
} GameState;

#endif