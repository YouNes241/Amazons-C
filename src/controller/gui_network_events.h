/**
 * @file gui_network_events.h
 * @brief Declarations for GUI network event handlers.
 *
 * @details This header provides the interface for creating and binding network
 * event callbacks (such as receiving a move, handling disconnections, or
 * starting a game) specifically tailored for the GTK graphical user interface.
 */

#ifndef GUI_NETWORK_EVENTS_H
#define GUI_NETWORK_EVENTS_H

#include "gui_controller.h"
#include "network_common.h"

/**
 * @brief Creates and initializes the network event callbacks for the GUI.
 * * @details This function populates a `NetworkEvents` structure with the
 * appropriate function pointers needed to update the GTK interface whenever
 * network-related events occur (e.g., transitioning from the lobby to the game
 * board, applying an opponent's move, or handling unexpected disconnections).
 * * @param ctx Pointer to the main GUI context. If NULL is passed, it acts as
 * a reset and returns an empty/zeroed structure.
 * @return A `NetworkEvents` structure containing the mapped GUI callbacks.
 */
NetworkEvents create_gui_network_events (GuiContext *ctx);

#endif // GUI_NETWORK_EVENTS_H