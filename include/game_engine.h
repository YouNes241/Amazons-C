#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "../src/gamemodel/bitboard.h"
#include "game_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Initializes the board with the official starting configuration.
 * Places the 4 white queens and 4 black queens.
 */
void game_init (GameState *state);

/**
 * @brief Converts coordinates (x, y) to an index.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param board_size The current size of the board (e.g., 10 for 10x10).
 * @return The corresponding index.
 */
uint8_t coords_to_index (uint8_t x, uint8_t y, uint8_t board_size);

/**
 * @brief Converts an index to 2D coordinates (x, y).
 * @param index The index to convert.
 * @param board_size The current size of the board.
 * @param out_x Pointer to store the x-coordinate.
 * @param out_y Pointer to store the y-coordinate.
 */
void index_to_coords (uint8_t index, uint8_t board_size, uint8_t *out_x,
                      uint8_t *out_y);

/**
 * @brief Checks if coordinates are within the dynamic board limits.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param board_size The current size of the board.
 * @return True if valid, false otherwise.
 */
bool is_valid_coord (int8_t x, int8_t y, uint8_t board_size);
/**
 * @brief Checks if a move (queen movement + arrow shot) is legal.
 * @param state The current game state.
 * @param move The move to check.
 * @return True if the move is valid, false otherwise.
 */
bool is_move_legal (const GameState *state, Move move);

/**
 * @brief Converts the GameState's 2D matrix into 3 separate Bitboards.
 * This function is designed for the AI, it allows scanning the board
 * only once before running high-performance simulations.
 * @param state The current game state containing the 2D matrix.
 * @param bb_white Pointer to the bitboard that will store the white queens.
 * @param bb_black Pointer to the bitboard that will store the black queens.
 * @param bb_arrows Pointer to the bitboard that will store all the arrows.
 */
void extract_bitboards_from_state (const GameState *state, Bitboard *bb_white,
                                   Bitboard *bb_black, Bitboard *bb_arrows);

/**
 * @brief Officially applies a move to the board and switches to the next turn.
 * Note: This function assumes is_move_legal has already been verified.
 * @param state The game state to modify.
 * @param move The move to apply.
 */
void apply_move (GameState *state, Move move);

/**
 * @brief Calculates all squares reachable in a straight line or diagonal from
 * a given point. This function simulates a queen's line of sight (raycasting)
 * and stops at the first obstacle.
 * @param obstacles A combined bitboard containing ALL pieces (white + black +
 * arrows).
 * @param start_index The starting index of the queen moving or
 * shooting.
 * @param board_size The current size of the board.

 * @return A Bitboard where each '1' bit represents a valid destination square.
 */
Bitboard engine_get_reachable_squares (Bitboard obstacles, uint8_t start_index,
                                       uint8_t board_size);

/**
 * @brief Generates the complete list of all possible legal moves for a player.
 * A complete move includes moving a queen AND shooting an arrow.
 * @param bb_white The current bitboard of white queens.
 * @param bb_black The current bitboard of black queens.
 * @param bb_arrows The current bitboard of arrows.
 * @param current_player The color of the player who has to move.
 * @param out_moves Pre-allocated array to be filled.
 * @return The total number of valid moves found and inserted into the array.
 */
int engine_generate_all_moves (Bitboard bb_white, Bitboard bb_black,
                               Bitboard bb_arrows, PlayerColor current_player,
                               Move *out_moves);

/**
 * @brief Checks if the current player has lost (if they are completely
 * blocked).
 * @param state The game state.
 * @param current_player The player to check.
 * @return True if the game is over for this player, false otherwise.
 */
bool is_game_over (const GameState *state, PlayerColor current_player);

/**
 * @brief Determines the winner of the game.
 * @param state The game state.
 * @return The winning player color.
 */
PlayerColor get_winner (const GameState *state);

/**
 * @brief Frees the dynamically allocated memory for the game board.
 * Must be called at the end of the game to prevent memory leaks.
 * @param state The game state to clean up.
 */
void game_cleanup (GameState *state);

#endif