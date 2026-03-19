/**
 * @file network_senders.c
 * @brief Functions to construct and send various types of packets from the
 * Client and Server.
 *
 * This file contains the implementation of functions that build specific types
 * of packets (e.g., Join, Move, Invite) and send them using the
 * NetworkManager's send function. Each function ensures that the packet is
 * properly formatted according to the protocol before sending it to the
 * intended recipient.
 */

#include "network_includes/network_internal.h"
#include "network_includes/network_packet.h"
#include "network_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* ==========================================================================
 * Public API Implementation
 * ==========================================================================
 */

/* --- Client-side senders --- */

/**
 * @brief Sends a Join packet to the server with the specified username.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param username The username to join with.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendJoin (NetworkManager *manager, const char *username)
{
  if (manager == NULL || username == NULL)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_JOIN;
  strncpy (pkt.payload, username, MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    {
      return NET_ERROR;
    }

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Constructs and sends a Move packet to the server or opponent.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param m The Move structure containing the move details.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendMove (NetworkManager *manager, Move m)
{
  if (manager == NULL || manager->current_mode == NET_MODE_NONE)
    return NET_SUCCESS;

  char move_str[MAX_MOVE_STR_SIZE];
  Protocol_MoveToString (m, move_str, sizeof (move_str));
  move_str[strcspn (move_str, "\n")] = 0;

  Packet pkt;
  pkt.type = (manager->current_mode == NET_MODE_CLIENT) ? C2S_MOVE : S2C_MOVE;
  strncpy (pkt.payload, move_str, MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  int target = (manager->current_mode == NET_MODE_CLIENT)
                   ? TARGET_SERVER
                   : TARGET_CLIENT_1; // TODO
  return NetworkManager_Send (manager, target, formatted_buffer);
}

/**
 * @brief Helper function to send a simple text message from the server to a
 * specific client.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param target_index The index of the target client (1 to MAX_CLIENTS).
 * @param type The PacketType to send (e.g., S2C_ERROR, S2C_WELCOME).
 * @param msg The message content to include in the packet payload.
 */
int
NetworkManager_SendTimeout (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode == NET_MODE_NONE)
    return NET_ERROR;

  Packet pkt;
  pkt.type = PKT_TIMEOUT;
  strncpy (pkt.payload, "TIMEOUT", MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    {
      return NET_ERROR;
    }

  int target = (manager->current_mode == NET_MODE_CLIENT)
                   ? TARGET_SERVER
                   : TARGET_CLIENT_1; // TODO
  return NetworkManager_Send (manager, target, formatted_buffer);
}

/**
 * @brief Helper function to send a simple text message from the server to a
 * specific client.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param target_index The index of the target client (1 to MAX_CLIENTS).
 * @param type The PacketType to send (e.g., S2C_ERROR, S2C_WELCOME).
 * @param msg The message content to include in the packet payload.
 */
int
NetworkManager_SendInvite (NetworkManager *manager, int target_id,
                           int board_size)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_INVITE;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d %d", target_id, board_size);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Accepts an invitation from a specific player ID.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param challenger_id The ID of the player who sent the invitation.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendAccept (NetworkManager *manager, int challenger_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_ACCEPT;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", challenger_id);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Declines an invitation from a specific player ID.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param challenger_id The ID of the player who sent the invitation.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendDecline (NetworkManager *manager, int challenger_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_DECLINE;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", challenger_id);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Requests the list of connected players from the server.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param target_id Optional player ID to get specific info about (or -1 for
 * all).
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendListRequest (NetworkManager *manager, int target_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_LIST;

  if (target_id >= 0)
    snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", target_id);
  else
    pkt.payload[0] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Requests the current scoreboard from the server.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendScoreboardRequest (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_SCOREBOARD;
  pkt.payload[0] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Sends a simple network packet (without a payload) to the server.
 * Used for AWAY, BACK, etc.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param type The PacketType to send (e.g., C2S_AWAY, C2S_BACK, C2S_PING).
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendSimplePacket (NetworkManager *manager, PacketType type)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = type;
  pkt.payload[0] = '\0'; // No payload for simple packets

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Sends an "Away" status update to the server.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendAway (NetworkManager *manager)
{
  return NetworkManager_SendSimplePacket (manager, C2S_AWAY);
}

/**
 * @brief Sends a "Back" status update to the server.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendBack (NetworkManager *manager)
{
  return NetworkManager_SendSimplePacket (manager, C2S_BACK);
}

/**
 * @brief Sends a "Cancel" message to the server to cancel an outstanding
 * invitation.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param target_id The ID of the player whose invitation is being canceled.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendCancel (NetworkManager *manager, int target_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_CANCEL;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", target_id);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Sends a "Ping" message to the server with the current timestamp to
 * measure latency.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendPing (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  // On récupère le temps actuel en millisecondes
  struct timeval tv;
  gettimeofday (&tv, NULL);
  long long now_ms = (long long)tv.tv_sec * 1000 + (tv.tv_usec / 1000);

  Packet pkt;
  pkt.type = C2S_PING;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%lld",
            now_ms); // On glisse l'heure dans le message

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Informs the server that the game is over and whether the client won
 * or lost.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param won True if the client won, false if they lost.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_SendGameOver (NetworkManager *manager, bool won)
{
  Packet pkt;
  pkt.type = C2S_GAME_OVER;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, won ? "WIN" : "LOSE");

  char out_buffer[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&pkt, out_buffer, sizeof (out_buffer));
  return NetworkManager_Send (manager, TARGET_SERVER, out_buffer);
}