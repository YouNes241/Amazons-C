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
  void (*OnGameError) (const char *error_msg);
  void (*OnTimeoutReceived) (void);
} NetworkEvents;

/* ==========================================================================
 * 3. The Manager Structure
 * ==========================================================================
 */

typedef struct
{
  NetworkMode current_mode;
  ServerContext *server_ctx;
  ClientContext *client_ctx;
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
int NetworkManager_SendJoin (NetworkManager *manager, const char *username);
int NetworkManager_SendMove (NetworkManager *manager, Move m);
int NetworkManager_Send (NetworkManager *manager, int target_client,
                         const char *message);
int NetworkManager_SendTimeout (NetworkManager *manager);
int NetworkManager_HasOpponent (NetworkManager *manager);

#endif // NETWORK_COMMON_H