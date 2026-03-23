/**
 * @file network_handlers.h
 * @brief Handlers for processing incoming packets on both Server and Client
 * sides.
 *
 * This header defines the functions responsible for interpreting and acting
 * upon the packets received by the Server and Client. It includes logic for
 * handling various message types such as Join, Move, etc., and ensures that
 * the game state is updated accordingly based on the received data.
 */

#ifndef NETWORK_HANDLERS_H
#define NETWORK_HANDLERS_H

#include "network_internal.h"
#include "network_packet.h"

/** @brief Processes a packet received by the Server and applies the logic
 * (Join, Move...).
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext containing the server state.
 * @param client_index The index of the client that sent the packet.
 * @param pkt Pointer to the Packet structure containing the received data.
 */
void handle_server_packet (NetworkManager *manager, ServerContext *ctx,
                           int client_index, Packet *pkt);

/** @brief Processes a packet received by the Client and triggers the game's
 * behavior.
 * @param manager Pointer to the NetworkManager instance.
 * @param pkt Pointer to the Packet structure containing the received data.
 */
void handle_client_packet (NetworkManager *manager, Packet *pkt);

/** @brief Processes packets in the TCP buffer.
 * @param manager Pointer to the NetworkManager instance.
 * @param buffer Pointer to the TCP buffer containing the packets.
 * @param client_index The index of the client whose packets are being
 * processed.
 */
void process_buffer_packets (NetworkManager *manager, tcp_buffer_t *buffer,
                             int client_index);

/** @brief Disconnects a client from the server.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext containing the server state.
 * @param client_index The index of the client to disconnect.
 */
void disconnect_server_client (NetworkManager *manager, ServerContext *ctx,
                               int client_index);

#endif // NETWORK_HANDLERS_H