#ifndef SAVE_H
#define SAVE_H

#include "config.h"
#include "game_engine.h"

typedef enum
{
  SAVE_OK = 0,
  ERR_FILE_NOT_FOUND = -1,
  ERR_BOARD_SIZE_MISMATCH = -2,
  ERR_CORRUPT_GAME_SECTION = -3,
  ERR_CORRUPT_HISTORY_SECTION = -4,
  ERR_PREMATURE_EOF = -5,
  ERR_INVALID_QUEEN_COUNT = -6
} SaveReturn;

/**
 * @brief Saves the current game into a text file using the required format.
 * @param filepath Destination file path (for example, "save1.txt").
 * @param config   Pointer to the current configuration.
 * @param state    Pointer to the current game state.
 * @return 0 if the save is successful, -1 on failure.
 */
int save_game (const char *filepath, game_config_t *config, GameState *state);

/**
 * @brief Loads a saved game file into the current configuration and state.
 * @param filepath Path to the saved game file.
 * @param config   Pointer to the configuration to populate.
 * @param state    Pointer to the game state to populate.
 * @return SAVE_OK on success or an error code on failure.
 */
SaveReturn load_game (const char *filepath, game_config_t *config,
                      GameState *state);

#endif // SAVE_H
