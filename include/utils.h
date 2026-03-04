#ifndef UTILS_H
#define UTILS_H

#include "bitboard.h"
#include "game_types.h"
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

Bitboard get_reachable (uint8_t start_index, Bitboard occupied, uint8_t size);

int get_queen_moves (uint8_t queen_index, Bitboard occupied, uint8_t size,
                     Move *moves_array);

int get_arrow_moves (Move *queen_moves, Move *final_moves,
                     int queen_move_count, Bitboard occupied, uint8_t size);

int get_legal_moves (BitboardState state, Move *moves_array);
#endif