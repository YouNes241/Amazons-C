/**
 * @file network_senders.c
 * @brief Contient toutes les fonctions d'aide pour formater et envoyer des
 * requêtes réseau.
 */

#include "network_common.h"
#include "network_internal.h"
#include "network_packet.h"
#include "network_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/**
 * @brief Constructs and sends a JOIN packet cleanly.
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
 * @brief Converts a Move to text, encapsulates it in a Packet and sends it.
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
 * @brief Constructs and sends a TIMEOUT packet cleanly.
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
 * @brief Sends an invitation to a specific player ID.
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
 * Used for AWAY, BACK, CANCEL, PING, etc.
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

int
NetworkManager_SendAway (NetworkManager *manager)
{
  return NetworkManager_SendSimplePacket (manager, C2S_AWAY);
}

int
NetworkManager_SendBack (NetworkManager *manager)
{
  return NetworkManager_SendSimplePacket (manager, C2S_BACK);
}

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