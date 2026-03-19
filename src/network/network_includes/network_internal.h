/**
 * @file network_internal.h
 * @brief Internal structures, constants, and definitions for the network
 * module.
 *
 * This header defines the internal data structures, constants, and helper
 * types used across the network module's implementation files. It is not
 * intended to be included by external code or unit tests directly, but rather
 * serves as a common internal reference for the various components of the
 * network layer.
 *
 * It includes definitions for client connection management, UDP discovery
 * contexts, and core server/client context structures that encapsulate the
 * state and resources needed for network communication and game management.
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

/** @brief Enumeration for client status within the server context. */
typedef enum
{
  /** @brief Client is connected to the server but has not yet joined a game.
   */
  CLIENT_STATUS_CONNECTED,
  /** @brief Client is in the lobby and waiting for an invite. */
  CLIENT_STATUS_IDLE,
  /** @brief Client is in the lobby but marked as away (not available for
     invites). */
  CLIENT_STATUS_AWAY,
  /** @brief Client has sent an invite and is waiting for acceptance. */
  CLIENT_STATUS_WAITGAME,
  /** @brief Client is currently in a game (either as challenger or invitee).
   */
  CLIENT_STATUS_INGAME
} ClientStatus;

/** @brief Structure representing a connected client. */
typedef struct
{
  /** @brief The client's socket file descriptor. */
  int fd;
  /** @brief Dedicated TCP buffer for this client to handle partial reads and
   * message assembly. */
  tcp_buffer_t buffer;
  /** @brief Flag indicating whether the client has completed the JOIN process
   * and is fully recognized by the server. */
  bool is_joined;
  /** @brief The username provided by the client during the JOIN process. */
  char username[MAX_USERNAME_LENGTH];
  /** @brief The ID of the game the client is currently playing, or NO_GAME_ID
   * if not in a game. */
  int game_id;
  /** @brief The client's current status in the server (e.g., idle, away,
   * ingame). */
  ClientStatus status;
  /** @brief The board size for the game the client is waiting to start (used
   * when status is WAITGAME). */
  int board_size;

  /** @brief Timestamp of the client's last activity, used for timeout
   * management. */
  time_t last_activity;
  /** @brief Player's statistics (can be used for a scoreboard feature). */
  int wins;
  int losses;
  int games_played;
} ConnectedClient;

// --- UDP Discovery Structures ---

/** @brief Structure for managing UDP discovery context on the server side. */
typedef struct
{
  /** @brief The UDP socket file descriptor for discovery. */
  int udp_socket_fd;
  /** @brief The broadcast address for sending discovery messages. */
  struct sockaddr_in broadcast_addr;
  /** @brief Timestamp of the last broadcast message sent. */
  time_t last_broadcast_time;
} ServerDiscoveryContext;

/** @brief Structure for representing a discovered server. */
typedef struct
{
  /** @brief The IP address of the discovered server. */
  char ip[INET_ADDRSTRLEN];
  /** @brief The name of the discovered server. */
  char name[MAX_SERVER_NAME_LEN];
  /** @brief The port on which the discovered server is listening. */
  int port;
  /** @brief Timestamp of when the server was last seen. */
  time_t last_seen;
} DiscoveredServer;

/** @brief Structure for managing UDP discovery context on the client side. */
struct ClientDiscoveryContext
{
  /** @brief The UDP socket file descriptor for listening to discovery
   * messages. */
  int udp_listen_socket;
  /** @brief Array of discovered servers. */
  DiscoveredServer servers[MAX_DISCOVERED_SERVERS];
  /** @brief The number of discovered servers. */
  int server_count;
};

// --- Core Context Structures ---

/** @brief Internal structure of the Server Context. */
struct ServerContext
{
  /** @brief The main TCP socket file descriptor for accepting new client
   * connections. */
  int listen_socket;
  /** @brief The port on which the server is listening. */
  int port;
  /** @brief The name of the server. */
  char server_name[MAX_SERVER_NAME_LEN];
  /**
   * @brief Monitoring table for poll().
   * @details Index 0 is ALWAYS the listen_socket (server entry point).
   * Indexes 1 to MAX_CLIENTS are dedicated to connected players.
   */
  struct pollfd fds[MAX_CLIENTS + 1];

  /** @brief Context for managing UDP discovery. */
  ServerDiscoveryContext discovery;
  /**
   * @brief Array of connected client data.
   * @details Index matches the fds array (e.g., clients[1] corresponds to
   * fds[1]).
   */
  ConnectedClient clients[MAX_CLIENTS + 1];
  /** @brief The number of currently connected clients. */
  int num_clients;
  /** @brief The ID of the next game to be created. */
  int next_game_id;
};

/**
 * @brief Internal structure of the Client Context.
 */
struct ClientContext
{
  /** @brief The TCP socket file descriptor for the connection to the server.
   */
  int server_socket;
  /** @brief The pollfd structure for monitoring the server socket. */
  struct pollfd fd_info; // The client only monitors one thing: its connection
                         // to the server
  /** @brief The TCP buffer for receiving messages from the server. */
  tcp_buffer_t
      buffer; // Personal buffer used by the client to receive server messages
};

void SaveScoreboard (NetworkManager *manager);
void LoadScoreboardForClient (ConnectedClient *client);
void NetworkManager_UpdatePlayerStats (const char *username, bool won);

#endif // NETWORK_INTERNAL_H