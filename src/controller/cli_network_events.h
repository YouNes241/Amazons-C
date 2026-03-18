#ifndef CLI_NETWORK_EVENTS_H
#define CLI_NETWORK_EVENTS_H

#include "../../include/network_common.h"
#include "cli_context.h"

// Function that will return a ready-to-use NetworkEvents structure
NetworkEvents create_cli_network_events (CliContext *ctx);

#endif // CLI_NETWORK_EVENTS_H