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

// Function that will return a ready-to-use NetworkEvents structure
NetworkEvents create_cli_network_events (CliContext *ctx);

#endif // CLI_NETWORK_EVENTS_H