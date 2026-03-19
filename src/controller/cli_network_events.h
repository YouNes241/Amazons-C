/**
 * @file cli_network_events.h
 * @brief Header for CLI network event handlers in the Amazons game.
 *
 * This header defines the function that creates a NetworkEvents structure
 * populated with the appropriate handler functions for processing network
 * events in the command-line interface (CLI) of the Amazons game. It allows
 * for easy integration of network event handling into the CLI context.
 */

#ifndef CLI_NETWORK_EVENTS_H
#define CLI_NETWORK_EVENTS_H

#include "../../include/network_common.h"
#include "cli_context.h"

/** @brief Creates a NetworkEvents structure populated with CLI-specific
 * handler functions.
 *  @param ctx The CLI context.
 *  @return A NetworkEvents structure with the appropriate handler functions.
 */
NetworkEvents create_cli_network_events (CliContext *ctx);

#endif // CLI_NETWORK_EVENTS_H