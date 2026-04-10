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

#include "network_common.h"
#include "tcp_buffer.h"
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

/* ==========================================================================
 * 1. Constants
 * ==========================================================================
 */

// Network Return Codes

/** @brief Indicates a successful network operation.
 * @details Used as a return code to signify that a network operation completed
 * successfully.
 */
#define NET_SUCCESS 0

/** @brief Indicates a generic network error.
 * @details Used as a return code to signify that a network operation
 * encountered an unspecified error.
 */
#define NET_ERROR -1

/** @brief Indicates that the network buffer is full.
 * @details Used as a return code to signify that the buffer has reached its
 * capacity and cannot accept more data.
 */
#define NET_BUFFER_FULL -2

/** @brief Indicates that no data is available to read.
 * @details Used as a return code to signify that the network operation did not
 * retrieve any data.
 */
#define NET_NO_DATA 0

/** @brief Indicates that data was successfully retrieved.
 * @details Used as a return code to signify that the network operation
 * successfully retrieved data.
 */
#define NET_DATA_READ 1

/** @brief Indicates that no message was found in the buffer.
 * @details Used as a return code to signify that the buffer does not contain a
 * complete message.
 */
#define EXTRACT_NO_MSG 0

/** @brief Indicates that a message exceeds the buffer capacity.
 * @details Used as a return code to signify that the message size is larger
 * than the buffer can handle.
 */
#define EXTRACT_MSG_OVERFLOW -1

// Socket & Connection Settings
/** @brief Invalid socket value.
 * @details Used to indicate an invalid socket descriptor.
 */
#define INVALID_SOCKET -1
/** @brief Maximum number of pending connections.
 * @details Used to control the maximum number of connections that can be
 * pending at any given time.
 */
#define MAX_PENDING_CONNECTIONS 10
/** @brief Flag to enable socket options.
 * @details Used to control whether socket options are enabled.
 */
#define ENABLE_SOCKET_OPTION 1
/** @brief Poll timeout for immediate returns.
 * @details Used to control the poll timeout for immediate returns.
 */
#define POLL_TIMEOUT_IMMEDIATE 0
/** @brief Count of single file descriptors.
 * @details Used to control the number of single file descriptors.
 */
#define SINGLE_FD_COUNT 1

/** @brief default port of the udp discovery service.
 * @details Used for UDP discovery of game servers on the local network.
 * This port is used by the server to broadcast its presence and by clients to
 * listen for available servers.
 */
#define UDP_DISCOVERY_PORT 12346

/** @brief Interval for broadcasting UDP discovery messages.
 * @details Used to control how frequently the server broadcasts its presence
 * on the local network.
 */
#define BROADCAST_INTERVAL_SEC 10

/** @brief Timeout for server responses.
 * @details Used to control how long the server will wait for a response from
 * a client before considering the connection stale.
 */
#define SERVER_TIMEOUT_SEC 30

/** @brief Timeout for invite messages.
 * @details Used to control how long an invite message is valid before it is
 * considered expired.
 */
#define TIMEOUT_INVITE 300.0 // 5 minutes in seconds

/** @brief Timeout for client connections.
 * @details Used to control how long a client connection is allowed to remain
 * inactive before it is closed.
 */
#define CLIENT_TIMEOUT_SEC 60

// Buffer Sizes & Limits
/** @brief Size of the temporary receive buffer.
 * @details Used for initial reads from the socket before processing into the
 * client's TCP buffer.
 * why 256 :
  - It's large enough to hold multiple small packets or a single large packet,
    allowing for efficient reads and reducing the number of recv calls needed.
  - It provides a good balance between memory usage and performance, as most
    packets in our protocol are expected to be well under this size.
  - It allows for handling cases where multiple packets arrive in quick
 succession without overwhelming the server's memory resources.
 */
#define RECV_TEMP_BUFFER_SIZE 256

/** @brief Maximum size of a move string representation.
 * @details Used for converting move structures to strings for network
 * transmission. 128 bytes is sufficient to represent a move in our protocol,
 * which typically includes the starting position, ending position, and any
 * additional move details. This size allows for a clear and unambiguous
 * representation of moves while ensuring that we do not exceed typical packet
 * size limits for our protocol.
 */
#define MAX_MOVE_STR_SIZE 128

/** @brief Size of the buffer for storing player information.
 * @details Used for managing player data within the server context.
 * 64 bytes is chosen to accommodate typical player information such as
 * username, status, and other relevant details without being excessively
 * large. This size allows for efficient memory usage while providing enough
 * space for necessary player attributes.
 */
#define PLAYER_INFO_BUFFER_SIZE 64

/** @brief Size of the buffer for storing scoreboard entry information.
 * @details Used for managing scoreboard data within the server context.
 * 128 bytes is chosen to accommodate typical scoreboard entry information such
 * as username, wins, losses, and other relevant details without being
 * excessively large. This size allows for efficient memory usage while
 * providing enough space for necessary scoreboard attributes.
 */
#define SCOREBOARD_ENTRY_BUFFER_SIZE 128

/** @brief Maximum length of a username.
 * @details Used for validating and storing usernames within the server
 * context. 32 characters is chosen to provide a reasonable limit for usernames
 * while ensuring that we do not exceed typical buffer sizes for our protocol.
 */
#define MAX_USERNAME_LENGTH 32

/** @brief Maximum size of a formatted packet.
 * @details Used for ensuring that packets do not exceed the maximum allowed
 * size. 512 bytes is chosen to provide a reasonable limit for packet sizes
 * while ensuring that we do not exceed typical network buffer sizes for our
 * protocol.
 */
#define MAX_FORMATTED_PKT_SIZE 512

/** @brief Size of the buffer for UDP discovery messages.
 * @details Used for managing discovery messages within the server context.
 * 128 bytes is chosen to accommodate typical discovery messages while
 * ensuring that we do not exceed typical network buffer sizes for our
 * protocol.
 */
#define UDP_DISCOVERY_BUFFER_SIZE 128

/** @brief Offset for the null terminator in strings.
 * @details Used for ensuring strings are properly null-terminated.
 */
#define NULL_TERMINATOR_OFFSET 1 // Space for '\0'

/** @brief Maximum length of a server name.
 * @details Used for validating and storing server names within the server
 * context. 32 characters is chosen to provide a reasonable limit for server
 * names while ensuring that we do not exceed typical buffer sizes for our
 * protocol.
 */
#define MAX_SERVER_NAME_LEN 32

/** @brief Maximum number of discovered servers to keep track of.
 * @details Used for managing the list of discovered servers within the server
 * context. 10 servers is chosen to provide a reasonable limit for the number
 * of discovered servers while ensuring that we do not exceed typical buffer
 * sizes for our protocol.
 */
#define MAX_DISCOVERED_SERVERS 10

// Client & Target Indexing

/** @brief Maximum number of clients supported by the server.
 * @details Determines the upper limit of simultaneous client connections the
 * server can handle.
 */
#define MAX_CLIENTS 64

/** @brief Index of the first client in the client list.
 * @details Used as the starting index for iterating through connected clients.
 */
#define FIRST_CLIENT_INDEX 1

/** @brief Index of the listening socket in the file descriptor list.
 * @details Used to identify the server's listening socket among other file
 * descriptors.
 */
#define LISTEN_SOCKET_INDEX 0

/** @brief Target index for the server in communication scenarios.
 * @details Represents the server as the target in client-server communication.
 */
#define TARGET_SERVER 0

/** @brief Target index for the first client in communication scenarios.
 * @details Represents the first client as the target in server-client
 * communication.
 */
#define TARGET_CLIENT_1 1

// Game Logic & State
//  Opponent status

/** @brief Indicates that a player has an opponent.
 * @details Used in game logic to signify that a player is currently matched
 * with an opponent.
 */
#define HAS_OPPONENT 1
/** @brief Indicates that a player does not have an opponent.
 * @details Used in game logic to signify that a player is not currently
 * matched with an opponent.
 */
#define NO_OPPONENT 0

// Roles and initialization

/** @brief Value representing the white role.
 * @details Used in game logic to identify the white player.
 */
#define ROLE_WHITE_VAL 0
/** @brief Value representing the black role.
 * @details Used in game logic to identify the black player.
 */
#define ROLE_BLACK_VAL 1
/** @brief String representation of the white role.
 * @details Used in game logic to identify the white player.
 */
#define ROLE_WHITE_STR "0"
/** @brief String representation of the black role.
 * @details Used in game logic to identify the black player.
 */
#define ROLE_BLACK_STR "1"

// no game assigned
/** @brief Represents no game ID assigned.
 * @details Used to indicate that a player is not currently in a game.
 */
#define NO_GAME_ID -1

/** @brief Represents an invalid socket descriptor.
 * @details Used to indicate that a socket operation failed or the socket is
 * not initialized.
 */
#define INVALID_SOCKET -1

/** @brief Maximum number of pending connections in the server's listen queue.
 * @details Determines how many incoming connections can be queued before the
 * server starts rejecting new ones.
 */
#define MAX_PENDING_CONNECTIONS 10

/** @brief Enables or disables a socket option.
 * @details Used to configure socket behavior, such as allowing address reuse.
 */
#define ENABLE_SOCKET_OPTION 1

/** @brief Immediate timeout value for polling.
 * @details Indicates that the poll operation should return immediately without
 * waiting.
 */
#define POLL_TIMEOUT_IMMEDIATE 0

/** @brief Represents a single file descriptor count.
 * @details Used in scenarios where only one file descriptor is being monitored
 * or processed.
 */
#define SINGLE_FD_COUNT 1

/** @brief Port number used for UDP discovery.
 * @details This port is used by the server and clients to broadcast and
 * discover each other on the network.
 */
#define UDP_DISCOVERY_PORT 12346

/** @brief Interval for broadcasting UDP discovery messages.
 * @details Specifies the time in seconds between successive broadcast messages
 * sent by the server.
 */
#define BROADCAST_INTERVAL_SEC 10

/** @brief Timeout duration for server operations.
 * @details Specifies the maximum time in seconds the server will wait for
 * certain operations before timing out.
 */
#define SERVER_TIMEOUT_SEC 30

/** @brief Timeout duration for game invitations.
 * @details Specifies the time in seconds before a game invitation expires if
 * not accepted or declined.
 */
#define TIMEOUT_INVITE 300.0 // 5 minutes in seconds

/** @brief Timeout duration for client inactivity.
 * @details Specifies the time in seconds before a client is considered
 * inactive and disconnected.
 */
#define CLIENT_TIMEOUT_SEC 60

/** @brief Maximum number of clients supported by the server.
 * @details Determines the upper limit of simultaneous client connections the
 * server can handle.
 */
#define MAX_CLIENTS 64

/** @brief Index of the first client in the client list.
 * @details Used as the starting index for iterating through connected clients.
 */
#define FIRST_CLIENT_INDEX 1

/** @brief Index of the listening socket in the file descriptor list.
 * @details Used to identify the server's listening socket among other file
 * descriptors.
 */
#define LISTEN_SOCKET_INDEX 0

/** @brief Target index for the server in communication scenarios.
 * @details Represents the server as the target in client-server communication.
 */
#define TARGET_SERVER 0

/** @brief Target index for the first client in communication scenarios.
 * @details Represents the first client as the target in server-client
 * communication.
 */
#define TARGET_CLIENT_1 1

/** @brief Indicates that a player has an opponent.
 * @details Used in game logic to signify that a player is currently matched
 * with an opponent.
 */
#define HAS_OPPONENT 1

/** @brief Indicates that a player does not have an opponent.
 * @details Used in game logic to signify that a player is not currently
 * matched with an opponent.
 */
#define NO_OPPONENT 0

/** @brief Value representing the white player's role.
 * @details Used in game initialization and role assignment.
 */
#define ROLE_WHITE_VAL 0

/** @brief Value representing the black player's role.
 * @details Used in game initialization and role assignment.
 */
#define ROLE_BLACK_VAL 1

/** @brief String representation of the white player's role.
 * @details Used in network communication and role assignment.
 */
#define ROLE_WHITE_STR "0"

/** @brief String representation of the black player's role.
 * @details Used in network communication and role assignment.
 */
#define ROLE_BLACK_STR "1"

/** @brief Value indicating no game is assigned to a player.
 * @details Used in game logic to signify that a player is not currently part
 * of any game.
 */
#define NO_GAME_ID -1

/* ==========================================================================
 * 2. Full Structure Definitions
 * ==========================================================================
 */

/** @brief Enumeration for client status within the server context. */
typedef enum
{
  /** @brief Client is connected to the server but has not yet joined a
   * game.
   */
  CLIENT_STATUS_CONNECTED,
  /** @brief Client is in the lobby and waiting for an invite. */
  CLIENT_STATUS_IDLE,
  /** @brief Client is in the lobby but marked as away (not available for
     invites). */
  CLIENT_STATUS_AWAY,
  /** @brief Client has sent an invite and is waiting for acceptance. */
  CLIENT_STATUS_WAITGAME,
  /** @brief Client is currently in a game (either as challenger or
   * invitee).
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
  /** @brief The ID of the player who invited this client. -1 if none. */
  int pending_invite_from;
  /** @brief The timestamp when the invitation was received (for the 300s
   * timeout). */
  time_t invite_timestamp;
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

  /** * @brief Array of active games on the server.
   * @details We can have up to MAX_CLIENTS / 2 simultaneous games.
   */
  GameState active_games[MAX_CLIENTS / 2];
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
  /** @brief Timestamp of the last heartbeat of the client. */
  time_t last_heartbeat;
  /** @brief The TCP buffer for receiving messages from the server. */
  tcp_buffer_t
      buffer; // Personal buffer used by the client to receive server messages
};

/** @brief Saves the scoreboard data to a file.
 * @param manager Pointer to the network manager.
 */
void SaveScoreboard (NetworkManager *manager);
/** @brief Loads the scoreboard data for a specific client.
 * @param client Pointer to the connected client.
 */
void LoadScoreboardForClient (ConnectedClient *client);
/** @brief Updates the statistics for a player.
 * @param username The username of the player.
 * @param won Indicates whether the player won the game.
 */
void NetworkManager_UpdatePlayerStats (const char *username, bool won);

#endif // NETWORK_INTERNAL_H
