/**
 * @file network_internal.h
 * @brief Internal definitions for the Network Manager and Unit Tests.
 */

#ifndef NETWORK_INTERNAL_H
#define NETWORK_INTERNAL_H

#include "../../include/network_common.h"
#include "tcp_buffer.h"
#include <poll.h>

/* ==========================================================================
 * 1. Constants
 * ==========================================================================
 */

#define MAX_CLIENTS 64

/* ==========================================================================
 * 2. Full Structure Definitions
 * ==========================================================================
 */

/**
 * @brief Represents a client connected to the server with its own buffer.
 */
typedef struct
{
  int fd;              // Client socket file descriptor
  tcp_buffer_t buffer; // Dedicated TCP buffer for this client
  char username[32];   // The username sent via C2S_JOIN
  bool is_joined;
  // TODO add later: player_id, name, status (IDLE, INGAME), etc.
} ConnectedClient;

/**
 * @brief Internal structure of the Server Context.
 * @details Exposed here to allow unit tests to verify the number of clients
 * and the state of the file descriptor array (fds).
 */
struct ServerContext
{
  int listen_socket;
  /**
   * @brief Monitoring table for poll().
   * @details Index 0 is ALWAYS the listen_socket (server entry point).
   * Indexes 1 to MAX_CLIENTS are dedicated to connected players.
   */
  struct pollfd fds[MAX_CLIENTS + 1];

  /**
   * @brief Array of connected client data.
   * @details Index matches the fds array (e.g., clients[1] corresponds to
   * fds[1]).
   */
  ConnectedClient clients[MAX_CLIENTS + 1];

  int num_clients; // Current number of active connections
};

/**
 * @brief Internal structure of the Client Context.
 */
struct ClientContext
{
  int server_socket;
  struct pollfd fd_info; // The client only monitors one thing: its connection
                         // to the server
  tcp_buffer_t
      buffer; // Personal buffer used by the client to receive server messages
};

#endif // NETWORK_INTERNAL_H