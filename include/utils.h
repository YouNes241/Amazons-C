#ifndef UTILS_H
#define UTILS_H

#include "game_types.h"

/**
 * @brief Retrieves the current time in milliseconds since the Unix epoch.
 * @return The current time in milliseconds.
 */
unsigned long get_time_ms ();

/**
 * @brief Converts a linear index to a Position struct based on the board size.
 * @param index The linear index to convert (0 to size*size - 1).
 * @param size The dimension of the board (e.g., 10 for a 10x10 board).
 * @return A Position struct with row and column corresponding to the index.
 */
Position idx_to_pos (uint8_t index, uint8_t size);

/**
 * @brief Compares two moves for equality.
 * @param a The first move to compare.
 * @param b The second move to compare.
 * @return true if the moves are identical in terms of queen source, queen
 */
bool moves_are_equal (Move a, Move b);
#endif