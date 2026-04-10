/**
 * @file gui_controller.h
 * @brief Declarations for the GUI controller logic.
 *
 * @details This header provides the core controller functions for the GTK
 * graphical interface. It bridges the gap between the user interface (views)
 * and the game engine (model), handling player moves, AI turn triggering,
 * game state transitions, and network lifecycle management.
 */

#ifndef GUI_CONTROLLER_H
#define GUI_CONTROLLER_H

#include "gui_utils.h"

/**
 * @brief Delay in milliseconds before triggering the AI turn.
 * @details This slight delay ensures the GTK UI has enough time to redraw
 * the board (showing the human player's move) before the AI locks the state
 * or begins heavy background calculations.
 */
#define GUI_AI_DELAY 500

/**
 * @brief Polling interval in milliseconds for the network event loop.
 * @details Dictates how often the GUI checks for incoming network packets
 * (approx. 30 times per second, or 33ms).
 */
#define GUI_NETWORK_DELAY 33

/**
 * @brief Processes a move attempt made by the local player through the GUI.
 * * @details This function verifies the legality of the move using the game
 * engine. If valid, it applies the move, updates the UI, sends the move to the
 * server (if in a network match), checks for game over conditions, and passes
 * the turn.
 * * @param ctx Pointer to the main GUI context.
 * @param move The requested move structure (Queen movement and arrow shot).
 */
void guiHandlePlayerMove (GuiContext *ctx, Move move);

/**
 * @brief Initializes and starts a new local game.
 * * @details Cleans up any existing game state or AI instances, re-initializes
 * the game board, sets up AI players based on current configuration settings,
 * and refreshes the visual board.
 * * @param ctx Pointer to the main GUI context.
 */
void guiStartNewGame (GuiContext *ctx);

/**
 * @brief Disconnects from any active network session and cleans up resources.
 * * @details Safely shuts down the client or server network threads, updates
 * the internal GUI state to reflect offline mode, and resets the UI panels
 * accordingly.
 * * @param ctx Pointer to the main GUI context.
 */
void guiDisconnectAndRestartNetwork (GuiContext *ctx);

/**
 * @brief Checks if the current turn belongs to an AI and triggers it if
 * necessary.
 * * @details If the current player has an AI configured and enabled, this
 * function places a lock on the UI (`is_ai_thinking = true`) and schedules the
 * AI worker thread to calculate the next move.
 * * @param ctx Pointer to the main GUI context.
 */
void guiCheckAndTriggerAI (GuiContext *ctx);

#endif // GUI_CONTROLLER_H