/**
 * @file network_common.h
 * @brief Common definitions, structures, and prototypes for the Network
 * Manager.
 *
 * @details This header defines the public interface of the networking module,
 * including operation modes, event callbacks, and the main manager structure.
 */

#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include "game_types.h"
#include <stdbool.h>

/* ==========================================================================
 * 1. Enums
 * ==========================================================================
 */

/**
 * @enum NetworkMode
 * @brief Defines the current operational state of the network manager.
 */
typedef enum
{
  NET_MODE_NONE,
  NET_MODE_SERVER,
  NET_MODE_CLIENT
} NetworkMode;

/* ==========================================================================
 * 2. Forward Declarations
 * ==========================================================================
 */

/**
 * @typedef ServerContext
 * @brief Opaque forward declaration for server-specific internal state.
 * @details Defined in the implementation to hide server socket details from
 * the public API.
 */
typedef struct ServerContext ServerContext;

/**
 * @typedef ClientContext
 * @brief Opaque forward declaration for client-specific internal state.
 * @details Defined in the implementation to hide client socket details from
 * the public API.
 */
typedef struct ClientContext ClientContext;

/* ==========================================================================
 * 3. Callback Interface
 * ==========================================================================
 */

/**
 * @struct NetworkEvents
 * @brief Interface for handling network events via callbacks.
 *
 * @details This structure implements the Dependency Injection pattern,
 * allowing the NetworkManager to notify the Controller (Game Logic) of events
 * without knowing the internal implementation of the game.
 */
typedef struct
{
  /**
   * @brief Called when a move is received from the opponent.
   * @param move The move data containing source and destination.
   */
  void (*OnMoveReceived) (Move move);

  /**
   * @brief Called when a new player successfully connects to the server.
   * @param player_name The identifier or name of the joining player.
   */
  void (*OnPlayerJoined) (const char *player_name);

  /**
   * @brief Called when a player leaves the game or the connection is lost.
   * @param reason A string description of why the disconnection occurred.
   */
  void (*OnPlayerDisconnected) (const char *reason);

  /**
   * @brief Called when an invitation to play is received from another player.
   * @param from_player_id The unique ID of the sender.
   */
  void (*OnInvitationReceived) (int from_player_id);

  /**
   * @brief Called when the opponent responds to an invitation sent by us.
   * @param accepted true if the invitation was accepted, false otherwise.
   */
  void (*OnInvitationResult) (bool accepted);

  /**
   * @brief Called when the game session officially starts.
   * @param color The assigned color for the local player (e.g., 0 for White, 1
   * for Black).
   */
  void (*OnGameStarted) (PlayerColor color);

} NetworkEvents;

/* ==========================================================================
 * 4. The Manager
 * ==========================================================================
 */

/**
 * @struct NetworkManager
 * @brief The main context structure for the Network Module.
 *
 * @details This is the primary object manipulated by the Controller. It holds
 * the current state (Server/Client), pointers to internal contexts, and
 * the event callback table.
 */
typedef struct
{
  NetworkMode current_mode; // The current mode of operation.

  /**
   * @brief Pointer to the internal server context.
   * @note Only allocated if current_mode is NET_MODE_SERVER.
   */
  ServerContext *server_ctx;

  /**
   * @brief Pointer to the internal client context.
   * @note Only allocated if current_mode is NET_MODE_CLIENT.
   */
  ClientContext *client_ctx;

  NetworkEvents callbacks; // The set of registered event callback functions.

} NetworkManager;

/* ==========================================================================
 * 5. Public Function Prototypes
 * ==========================================================================
 */

/**
 * @brief Initializes the NetworkManager instance.
 *
 * @param manager Pointer to the NetworkManager structure to initialize.
 * @param callbacks The structure containing the game logic event handlers.
 */
void NetworkManager_Init (NetworkManager *manager, NetworkEvents callbacks);

/**
 * @brief Cleans up network resources and memory.
 *
 * @details Should be called at the end of the program or when resetting the
 * network. It closes open sockets and frees allocated contexts.
 *
 * @param manager Pointer to the NetworkManager instance.
 */
void NetworkManager_Cleanup (NetworkManager *manager);

/**
 * @brief Processing function to be called every frame (Game Loop).
 *
 * @details Handles incoming data, processes buffers, and triggers callbacks.
 * This drives the "heartbeat" of the network machine.
 *
 * @param manager Pointer to the NetworkManager instance.
 */
void NetworkManager_Tick (NetworkManager *manager);

/**
 * @brief Starts the network module in Server mode.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param port The port number to listen on.
 * @return int 0 on success, -1 on failure.
 */
int NetworkManager_ServerStart (NetworkManager *manager, int port);

/**
 * @brief Starts the network module in Client mode and connects to a server.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param ip The IP address of the server as a string (e.g., "127.0.0.1").
 * @param port The port number of the server.
 * @return int 0 on success, -1 on failure.
 */
int NetworkManager_ClientJoin (NetworkManager *manager, const char *ip,
                               int port);

#endif // NETWORK_COMMON_H