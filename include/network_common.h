/**
 * @file network_common.h
 * @brief Interface publique du Network Manager.
 */

#ifndef NETWORK_COMMON_H
#define NETWORK_COMMON_H

#include "game_types.h"
#include <stdbool.h>

/* ==========================================================================
 * 1. Enums & Forward Declarations
 * ==========================================================================
 */

typedef enum
{
  NET_MODE_NONE,
  NET_MODE_SERVER,
  NET_MODE_CLIENT
} NetworkMode;

/**
 * @brief Déclarations opaques.
 * Le reste du projet sait qu'elles existent mais pas ce qu'elles contiennent.
 */
typedef struct ServerContext ServerContext;
typedef struct ClientContext ClientContext;

/* ==========================================================================
 * 2. Callback Interface
 * ==========================================================================
 */

typedef struct
{
  void (*OnMoveReceived) (Move move);
  void (*OnPlayerJoined) (const char *player_name);
  void (*OnPlayerDisconnected) (const char *reason);
  void (*OnInvitationReceived) (int from_player_id);
  void (*OnInvitationResult) (bool accepted);
  void (*OnGameStarted) (PlayerColor color);
} NetworkEvents;

/* ==========================================================================
 * 3. The Manager Structure
 * ==========================================================================
 */

typedef struct
{
  NetworkMode current_mode;
  ServerContext *server_ctx; // Pointeur vers structure opaque
  ClientContext *client_ctx; // Pointeur vers structure opaque
  NetworkEvents callbacks;
} NetworkManager;

/* ==========================================================================
 * 4. Public Prototypes
 * ==========================================================================
 */

void NetworkManager_Init (NetworkManager *manager, NetworkEvents callbacks);
void NetworkManager_Cleanup (NetworkManager *manager);
void NetworkManager_Tick (NetworkManager *manager);
int NetworkManager_ServerStart (NetworkManager *manager, int port);
int NetworkManager_ClientJoin (NetworkManager *manager, const char *ip,
                               int port);

#endif // NETWORK_COMMON_H