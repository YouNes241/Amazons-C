/* ==========================================================================
 * network_common.h
 * Header file for the common network manager interface and types.
 * This file defines the NetworkManager structure, callback interfaces, and
 * public function prototypes for managing network operations in both server
 * and client modes. It serves as a central point for including necessary types
 * and function declarations related to the network layer of the Amazons game.
 * ==========================================================================
 */

#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include "game_types.h"
#include "i18n.h"
#include <stddef.h>

/* ==========================================================================
 * 1. Enums & Forward Declarations
 * ==========================================================================
 */

/** @brief Enumeration of network modes. */
typedef enum
{
  /** @brief No network mode is active. */
  NET_MODE_NONE,
  /** @brief Server mode means this instance is hosting a game and waiting for
     clients. */
  NET_MODE_SERVER,
  /** @brief Client mode means this instance is connected to a server and
     possibly in a game. */
  NET_MODE_CLIENT
} NetworkMode;

/** @brief Forward declaration of the NetworkManager structure. */
typedef struct ServerContext ServerContext;
typedef struct ClientContext ClientContext;
typedef struct ClientDiscoveryContext ClientDiscoveryContext;

/* ==========================================================================
 * 2. Callback Interface
 * ==========================================================================
 */

/** @brief Structure containing all the callback functions for network events.
 */
typedef struct
{
  /** @brief Callback for when a move is received from the opponent (both for
   * Server and Client). */
  void (*OnMoveReceived) (Move move);
  /** @brief Callback for when a new player joins the lobby, along with their
   * name. */
  void (*OnPlayerJoined) (const char *player_name);
  /** @brief Callback for when a player disconnects from the lobby or game,
   * along with an optional reason. */
  void (*OnPlayerDisconnected) (const char *reason);
  /** @brief Callback for when an invitation is received from another player,
   * along with the sender's player ID. */
  void (*OnInvitationReceived) (int from_player_id);
  /** @brief Callback for when an invitation is declined by another player,
   * along with an optional message. */
  void (*OnInvitationDeclined) (const char *message);
  /** @brief Callback for when the game starts, providing the player's color
   * and board size. */
  void (*OnGameStarted) (PlayerColor color, int board_size);
  /** @brief Callback for when a game ends, providing the winner's color and an
   * optional message. */
  void (*OnGameError) (const char *error_msg);
  /** @brief Callback for when a timeout is received (when the opponent has
   * timed out). */
  void (*OnTimeoutReceived) (void);
  /** @brief Callback for when a list of players or servers is received. */
  void (*OnListReceived) (const char *list_text);
} NetworkEvents;

/* ==========================================================================
 * 3. The Manager Structure
 * ==========================================================================
 */

/** @brief Structure representing the network manager. */
typedef struct
{
  NetworkMode current_mode;
  ServerContext *server_ctx;
  ClientContext *client_ctx;
  NetworkEvents callbacks;
  ClientDiscoveryContext *discovery_ctx;
} NetworkManager;

/* ==========================================================================
 * 4. Public Prototypes
 * ==========================================================================
 */

// Main API functions for initializing, starting, joining, sending moves, etc.

/** @brief Initialize the network manager.
 *  @param manager Pointer to the network manager.
 *  @param callbacks Structure containing the callback functions.
 * @details This function sets up the network manager with the provided
 * callbacks and prepares it for either server or client mode. It also
 * initializes the discovery context for server discovery. After calling this
 * function, you can start a server or join an existing one using the
 * appropriate functions. Note: This function does not start any network
 * activity by itself; it only initializes the manager's internal state. You
 * must call NetworkManager_ServerStart or NetworkManager_ClientJoin to begin
 * network operations.
 */
void NetworkManager_Init (NetworkManager *manager, NetworkEvents callbacks);

/** @brief Cleanup the network manager.
 *  @param manager Pointer to the network manager.
 * @details This function will close any open connections, free allocated
 * memory, and reset the manager to a clean state. It should be called when
 * shutting down the server or client to ensure that all resources are properly
 * released.
 */
void NetworkManager_Cleanup (NetworkManager *manager);

/** @brief Update the network manager.
 *  @param manager Pointer to the network manager.
 * @details This function should be called periodically to update the network
 * manager and handle any pending operations.
 */
void NetworkManager_Tick (NetworkManager *manager);

/** @brief Start the network server.
 *  @param manager Pointer to the network manager.
 *  @param port The port on which to start the server.
 *  @return 0 on success, -1 on failure.
 * @details This function starts the network server on the specified port.
 */
int NetworkManager_ServerStart (NetworkManager *manager, int port);

/** @brief Join a network client to an existing server.
 *  @param manager Pointer to the network manager.
 *  @param ip The IP address of the server to join.
 *  @param port The port of the server to join.
 *  @return 0 on success, -1 on failure.
 * @details This function attempts to connect the client to the specified
 * server.
 */
int NetworkManager_ClientJoin (NetworkManager *manager, const char *ip,
                               int port);

/** @brief Send a join request to the server.
 *  @param manager Pointer to the network manager.
 *  @param username The username of the player joining.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a join request to the server with the specified
 * username.
 */
int NetworkManager_SendJoin (NetworkManager *manager, const char *username);

/** @brief Send a move to the opponent.
 *  @param manager Pointer to the network manager.
 *  @param m The move to send.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a move to the opponent over the network.
 */
int NetworkManager_SendMove (NetworkManager *manager, Move m);

/** @brief Send a message to a specific client.
 *  @param manager Pointer to the network manager.
 *  @param target_client The ID of the client to send the message to.
 *  @param message The message to send.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a message to the specified client over the
 * network.
 */
int NetworkManager_Send (NetworkManager *manager, int target_client,
                         const char *message);

/** @brief Send a timeout message to the opponent.
 *  @param manager Pointer to the network manager.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a timeout message to the opponent over the
 * network.
 */
int NetworkManager_SendTimeout (NetworkManager *manager);

/** @brief Send an invite message to a specific client.
 *  @param manager Pointer to the network manager.
 *  @param target_id The ID of the client to send the invite to.
 *  @param board_size The size of the board for the game.
 *  @return 0 on success, -1 on failure.
 * @details This function sends an invite message to the specified client over
 * the network.
 */
int NetworkManager_SendInvite (NetworkManager *manager, int target_id,
                               int board_size);

/** @brief Send an accept message to the challenger.
 *  @param manager Pointer to the network manager..
 *  @return 0 on success, -1 on failure.
 * @details This function sends an accept message to the specified challenger
 * over the network.
 */
int NetworkManager_SendAccept (NetworkManager *manager);

/** @brief Send a decline message to the challenger.
 *  @param manager Pointer to the network manager.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a decline message to the specified challenger
 * over the network.
 */
int NetworkManager_SendDecline (NetworkManager *manager);

/** @brief Send a list request to the server.
 *  @param manager Pointer to the network manager.
 *  @param target_id The ID of the client to send the list request to.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a list request to the specified client over the
 * network.
 */
int NetworkManager_SendListRequest (NetworkManager *manager, int target_id);

/** @brief Send an "away" message to the server.
 *  @param manager Pointer to the network manager.
 *  @return 0 on success, -1 on failure.
 * @details This function sends an "away" message to the server over the
 * network.
 */
int NetworkManager_SendAway (NetworkManager *manager);

/** @brief Send a "back" message to the server.
 *  @param manager Pointer to the network manager.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a "back" message to the server over the
 * network.
 */
int NetworkManager_SendBack (NetworkManager *manager);

/** @brief Send a cancel message to the server.
 *  @param manager Pointer to the network manager.
 *  @param target_id The ID of the client to send the cancel message to.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a cancel message to the specified client over
 * the network.
 */
int NetworkManager_SendCancel (NetworkManager *manager);

/** @brief Send a ping message to the server.
 *  @param manager Pointer to the network manager.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a ping message to the server over the network.
 */
int NetworkManager_SendPing (NetworkManager *manager);

/** @brief Send a game over message to the server.
 *  @param manager Pointer to the network manager.
 *  @param i_won Whether the player won the game.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a game over message to the server over the
 * network.
 */
int NetworkManager_SendGameOver (NetworkManager *manager, bool i_won);

/** @brief Send a scoreboard request to the server.
 *  @param manager Pointer to the network manager.
 *  @return 0 on success, -1 on failure.
 * @details This function sends a scoreboard request to the server overthe
 * network。
 */
int NetworkManager_SendScoreboardRequest (NetworkManager *manager);

/** @brief Get the list of players from the server.
 *  @param manager Pointer to the network manager.
 *  @param buffer Buffer to store the player list.
 *  @param max_len Maximum length of the buffer.
 *  @param target_id The ID of the client to send the request to.
 * @details This function gets the list of players from the server over the
 * network.
 */
void NetworkManager_GetServerPlayerList (NetworkManager *manager, char *buffer,
                                         size_t max_len, int target_id);

/** @brief Get the list of servers from the network.
 *  @param manager Pointer to the network manager.
 *  @param buffer Buffer to store the server list.
 *  @param max_len Maximum length of the buffer.
 *  @return 0 on success, -1 on failure.
 * @details This function gets the list of servers from the network.
 */
int NetworkManager_GetServerList (NetworkManager *manager, char *buffer,
                                  size_t max_len);

/** @brief Get the status of the server.
 *  @param manager Pointer to the network manager.
 *  @param out_clients Pointer to store the number of connected clients.
 *  @param out_games Pointer to store the number of active games.
 *  @param out_port Pointer to store the server port.
 *  @return 0 on success, -1 on failure.
 * @details This function gets the status of the server over the network.
 */
int NetworkManager_GetServerStatus (NetworkManager *manager, int *out_clients,
                                    int *out_games, int *out_port);

/** @brief Update the statistics for a player.
 *  @param username The username of the player.
 *  @param won Whether the player won the game.
 * @details This function updates the statistics for the specified player.
 */
void NetworkManager_UpdatePlayerStats (const char *username, bool won);

/** @brief Shutdown the game server.
 *  @param manager Pointer to the network manager.
 * @details This function shuts down the game server.
 */
void NetworkManager_ShutdownServer (NetworkManager *manager);

/** @brief Check if the player has an opponent.
 *  @param manager Pointer to the network manager.
 *  @return 1 if the player has an opponent, 0 otherwise.
 * @details This function checks if the player has an opponent connected to the
 * game.
 */
int NetworkManager_HasOpponent (NetworkManager *manager);

#endif // NETWORK_COMMON_H