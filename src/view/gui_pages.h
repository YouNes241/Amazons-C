/**
 * @file gui_pages.h
 * @brief Declarations for functions that create the various GTK pages/views.
 *
 * @details This header provides the interface for constructing the different
 * UI screens used in the GTK application, such as the main menu, configuration
 * panels, multiplayer lobbies, and the actual game board.
 */

#ifndef GUI_PAGES_H
#define GUI_PAGES_H

#include "gui_utils.h"
#include <gtk/gtk.h>

/**
 * @brief Creates the main menu page.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the main menu.
 */
GtkWidget *createMainMenuPage (GuiContext *ctx);

/**
 * @brief Creates the single-player setup menu page.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the single-player
 * menu.
 */
GtkWidget *createSingleplayerMenuPage (GuiContext *ctx);

/**
 * @brief Creates the multiplayer selection menu page.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the multiplayer
 * menu.
 */
GtkWidget *createMultiplayerMenuPage (GuiContext *ctx);

/**
 * @brief Creates the configuration and settings page.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the configuration
 * page.
 */
GtkWidget *createConfigPage (GuiContext *ctx);

/**
 * @brief Creates the information/rules page.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the info page.
 */
GtkWidget *createInfoPage (GuiContext *ctx);

/**
 * @brief Creates the main game page (the board and game UI).
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the active game
 * view.
 */
GtkWidget *createGamePage (GuiContext *ctx);

/**
 * @brief Creates the page used to enter server details and join a network
 * game.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the join server
 * form.
 */
GtkWidget *createJoinServerPage (GuiContext *ctx);

/**
 * @brief Creates the lobby panel page for the user hosting the server.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the host's lobby
 * panel.
 */
GtkWidget *createHostPanelPage (GuiContext *ctx);

/**
 * @brief Creates the lobby panel page for a client connected to a server.
 * * @param ctx Pointer to the main GUI context.
 * @return A pointer to the created GtkWidget representing the client's lobby
 * panel.
 */
GtkWidget *createClientPanelPage (GuiContext *ctx);

#endif // GUI_PAGES_H