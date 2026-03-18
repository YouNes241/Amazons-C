/**
 * @file network_internal.h
 * @brief Internal definitions for the Network Manager and Unit Tests.
 */

#ifndef NETWORK_INTERNAL_H
#define NETWORK_INTERNAL_H

#include "../../../include/network_common.h"
#include "tcp_buffer.h"
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

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

#define UDP_DISCOVERY_PORT 12346
#define BROADCAST_INTERVAL_SEC 10
#define SERVER_TIMEOUT_SEC 30

#define CLIENT_TIMEOUT_SEC 60

// Buffer Sizes & Limits
#define RECV_TEMP_BUFFER_SIZE 256
#define MAX_MOVE_STR_SIZE 128
#define PLAYER_INFO_BUFFER_SIZE 64
#define SCOREBOARD_ENTRY_BUFFER_SIZE 128
#define MAX_USERNAME_LENGTH 32
#define MAX_FORMATTED_PKT_SIZE 512
#define UDP_DISCOVERY_BUFFER_SIZE 128
#define NULL_TERMINATOR_OFFSET 1 // Space for '\0'
#define MAX_SERVER_NAME_LEN 32
#define MAX_DISCOVERED_SERVERS 10
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
#define ROLE_BLACK_VAL 1
#define ROLE_WHITE_STR "0"
#define ROLE_BLACK_STR "1"

// no game assigned
#define NO_GAME_ID -1

/* ==========================================================================
 * 2. Full Structure Definitions
 * ==========================================================================
 */

typedef enum
{
  CLIENT_STATUS_CONNECTED, // TCP connection established, but no username sent
                           // yet
  CLIENT_STATUS_IDLE,      //  Player is in the lobby, waiting for an invite
  CLIENT_STATUS_AWAY,      // Player is in the lobby but marked as away (not
                           // available for invites)
  CLIENT_STATUS_WAITGAME,  // Player has sent an invite and is waiting for
                           // acceptance
  CLIENT_STATUS_INGAME // Player is currently in a game (either as challenger
                       // or invitee)
} ClientStatus;

/**
 * @brief Represents a client connected to the server with its own buffer.
 */
typedef struct
{
  int fd;                             // Client socket file descriptor
  tcp_buffer_t buffer;                // Dedicated TCP buffer for this client
  char username[MAX_USERNAME_LENGTH]; // The username sent via C2S_JOIN
  bool is_joined;
  int game_id; // id of game playing now
  ClientStatus status;
  int board_size;

  // Stats for the scoreboard
  int wins;
  int losses;
  int games_played;
  time_t last_activity;
} ConnectedClient;

// --- UDP Discovery Structures ---

typedef struct
{
  int udp_socket_fd;
  struct sockaddr_in broadcast_addr;
  time_t last_broadcast_time;
} ServerDiscoveryContext;

typedef struct
{
  char ip[INET_ADDRSTRLEN];
  char name[MAX_SERVER_NAME_LEN];
  int port;
  time_t last_seen;
} DiscoveredServer;

struct ClientDiscoveryContext // Définition de la structure opaque du client
{
  int udp_listen_socket;
  DiscoveredServer servers[MAX_DISCOVERED_SERVERS];
  int server_count;
};

// --- Core Context Structures ---

/**
 * @brief Internal structure of the Server Context.
 * @details Exposed here to allow unit tests to verify the number of clients
 * and the state of the file descriptor array (fds).
 */
struct ServerContext
{
  int listen_socket;
  int port;
  char server_name[MAX_SERVER_NAME_LEN];
  /**
   * @brief Monitoring table for poll().
   * @details Index 0 is ALWAYS the listen_socket (server entry point).
   * Indexes 1 to MAX_CLIENTS are dedicated to connected players.
   */
  struct pollfd fds[MAX_CLIENTS + 1];

  ServerDiscoveryContext discovery;
  /**
   * @brief Array of connected client data.
   * @details Index matches the fds array (e.g., clients[1] corresponds to
   * fds[1]).
   */
  ConnectedClient clients[MAX_CLIENTS + 1];

  int num_clients; // Current number of active connections

  int next_game_id; // To generate unique game IDs (1, 2, 3...)
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