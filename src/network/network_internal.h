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

// Network Return Codes
#define NET_SUCCESS 0           // Generic success code
#define NET_ERROR -1            // Generic error code
#define NET_BUFFER_FULL -2      // Buffer capacity reached
#define NET_NO_DATA 0           // No data available to read
#define NET_DATA_READ 1         // Data successfully retrieved
#define EXTRACT_NO_MSG 0        // No message found in buffer
#define EXTRACT_MSG_OVERFLOW -1 // Message exceeds buffer capacity

// Socket & Connection Settings
#define INVALID_SOCKET -1
#define MAX_PENDING_CONNECTIONS 10
#define ENABLE_SOCKET_OPTION 1
#define POLL_TIMEOUT_IMMEDIATE 0
#define SINGLE_FD_COUNT 1

// Socket & Connection Settings
#define INVALID_SOCKET -1
#define MAX_PENDING_CONNECTIONS 10
#define ENABLE_SOCKET_OPTION 1
#define POLL_TIMEOUT_IMMEDIATE 0
#define SINGLE_FD_COUNT 1
// Buffer Sizes & Limits
#define RECV_TEMP_BUFFER_SIZE 256
#define MAX_MOVE_STR_SIZE 128
#define MAX_USERNAME_LENGTH 32
#define MAX_FORMATTED_PKT_SIZE 512
#define NULL_TERMINATOR_OFFSET 1 // Space for '\0'
// Client & Target Indexing
#define MAX_CLIENTS 64
#define FIRST_CLIENT_INDEX 1
#define LISTEN_SOCKET_INDEX 0
#define TARGET_SERVER 0
#define TARGET_CLIENT_1 1
// Game Logic & State
//  Opponent status
#define HAS_OPPONENT 1
#define NO_OPPONENT 0

// Roles and initialization
#define ROLE_WHITE_VAL 0
#define ROLE_BLACK_STR "1"

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