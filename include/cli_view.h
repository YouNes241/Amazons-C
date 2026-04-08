#ifndef CLI_VIEW_H
#define CLI_VIEW_H

#include "game_types.h"
#include "i18n.h"

/**
 * @brief Clears the terminal screen
 */
void cli_clear_screen ();

/**
 * @brief Displays the current game board with coordinates
 * @param state The GameState containing the 2D board matrix
 */
void cli_display_board (const GameState *state);

/**
 * @brief Affiche le menu principal du jeu avec les commandes de base
 */
void cli_display_main_menu (void);
#endif