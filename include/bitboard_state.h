#ifndef BITBOARD_STATE_H
#define BITBOARD_STATE_H

#include "game_types.h"

typedef struct
{
  Bitboard white_queens;
  Bitboard black_queens;
  Bitboard arrows;
  uint8_t size;
  bool white_to_move;
} BitboardState;

/**
 * @brief Allocates and initializes a new bitboard state with no pieces or
 * arrows.
 * @param size The dimension of the board.
 * @return An initialized BitboardState structure.
 */
BitboardState create_empty_state (uint8_t size);

/**
 * @brief Converts a GameState from the logic layer into a BitboardState for
 * use in the AI and evaluation functions.
 * @param gs Pointer to the GameState to convert.
 * @return A BitboardState representing the same game state.
 */
BitboardState sync_from_logic (GameState *gs);

/**
 * @brief Applies a given move to a BitboardState, returning the resulting
 * state without modifying the original.
 * @param state The current BitboardState before the move.
 * @param move The Move to apply to the state.
 * @return A new BitboardState reflecting the changes from the move.
 */
BitboardState apply_move_bitboard (BitboardState state, Move move);

/**
 * @brief Calculates the maximum number of moves possible in the current board
 * state.
 * @note It only calculates it once, as the size should not change.
 * (singleplayer)
 * @param state The current board state to evaluate.
 * @return Max number of legal moves possibles
 */
int get_move_limit (BitboardState state);

/**
 * @brief Generates all legal moves for the current player in the given
 * BitboardState.
 * @param state The current BitboardState to evaluate for legal moves.
 * @param moves_array An optional array to be filled with the legal moves. If
 * NULL, the function will only count the number of legal moves without filling
 * an array. Note that this will take a much longer time than is_game_over.
 * @return The total count of legal moves available for the current player.
 */
int get_legal_moves (BitboardState state, Move *moves_array);

/**
 * @brief Prints a visual representation of the game board to standard output.
 * @param state Pointer to the BitboardState to be visualized.
 */
void bitboard_state_print (const BitboardState *state);

/**
 * @brief Checks if the game is over by determining if there are no legal moves
 * available for the current player.
 * @param state The current board state to evaluate.
 * @return true if the game is over (no legal moves), false otherwise.
 */
bool bitboard_state_is_game_over (BitboardState state);
#endif