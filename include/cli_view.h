#ifndef CLI_VIEW_H
#define CLI_VIEW_H

#include "game_types.h"

/**
 * @brief Efface l'écran du terminal
 */
void cli_clear_screen ();

/**
 * @brief Affiche le plateau de jeu actuel avec les coordonnées
 * @param state Le GameState contenant la matrice 2D du plateau
 */
void cli_display_board (const GameState *state);

/**
 * @brief Affiche le menu principal du jeu avec les commandes de base
 */
void cli_display_main_menu (void);
#endif