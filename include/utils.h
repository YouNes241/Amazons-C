#ifndef UTILS_H
#define UTILS_H

#include "bitboard.h"
#include "temp_types.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Prints a visual representation of the game board to standard output.
 * @param state Pointer to the BitboardState to be visualized.
 */
void bitboard_state_print (const BitboardState *state);

/**
 * @brief Allocates and initializes a new bitboard state with no pieces or
 * arrows.
 * @param size The dimension of the board.
 * @return An initialized BitboardState structure.
 */
BitboardState create_empty_state (uint8_t size);

#endif