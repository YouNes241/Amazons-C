/**
 * @file network_handlers.h
 * @brief Manages the application logic (Application Layer) for the Server and
 the Client.

 */

#ifndef NETWORK_HANDLERS_H
#define NETWORK_HANDLERS_H

#include "network_internal.h"
#include "network_packet.h"

/**
 * @brief Processes a packet received by the Server and applies the logic
 (Join, Move...).

 */
void handle_server_packet (NetworkManager *manager, ServerContext *ctx,
                           int client_index, Packet *pkt);

/**
 * @brief Processes a packet received by the Client and triggers the game's
 * behavior.
 */
void handle_client_packet (NetworkManager *manager, Packet *pkt);

#endif // NETWORK_HANDLERS_H