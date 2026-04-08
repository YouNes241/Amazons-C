#ifndef SAVE_H
#define SAVE_H

#include "config.h"
#include "game_engine.h"

/**
 * @brief Saves the current game into a text file using the required format.
 * * @param filepath The destination file name (e.g., "save1.txt").
 * @param config   Pointer to the current configuration (for the board size).
 * @param state    Pointer to the game state (for the board and current turn).
 * @return 0 if the save is successful, -1 in case of an error (e.g., write
 * permissions).
 */
int save_game (const char *filepath, game_config_t *config, GameState *state);
int load_game (const char *filepath, game_config_t *config, GameState *state);

#endif // SAVE_H
