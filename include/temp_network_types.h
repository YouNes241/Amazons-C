#ifndef TEMP_NETWORK_TYPES_H
#define TEMP_NETWORK_TYPES_H

/* ==========================================================================
 * TEMPORARY FILE (MOCK)
 * To be replaced by the actual "common.h" file from team 7 later.
 * ==========================================================================
 */

// Maximum size for a player's username
#define MAX_PLAYER_NAME 64

// List for the color assigned to the player
typedef enum
{
  COLOR_WHITE = 0,
  COLOR_BLACK = 1
} player_color_t;

// Structure of a single move in the Amazones
// r = row, c = column, a = arrow
typedef struct
{
  int r1, c1; // Queen's departure coordinates
  int r2, c2; // Queen's arrival coordinates
  int ar, ac; // Coordinates of the cell where the arrow is fired
} move_t;

#endif // TEMP_NETWORK_TYPES_H