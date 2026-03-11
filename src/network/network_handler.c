#include "network_handlers.h"
#include "network_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/**
 * @brief Common helper to process MOVE packets for both Server and Client.
 */
static void
process_move_packet (NetworkManager *manager, const char *payload)
{
  Move parsed_move;
  if (Protocol_StringToMove (payload, &parsed_move))
    {
      if (manager->callbacks.OnMoveReceived)
        manager->callbacks.OnMoveReceived (parsed_move);
    }
}

/**
 * @brief Handles the complex logic of a player joining the server.
 */
static void
process_join_packet (NetworkManager *manager, ServerContext *ctx,
                     int client_index, const char *username)
{
  char out_buffer[MAX_FORMATTED_PKT_SIZE];
  Packet resp;

  // 1. Check if the username is already taken.
  bool name_taken = false;
  for (int j = FIRST_CLIENT_INDEX; j <= MAX_CLIENTS; j++)
    {
      if (j != client_index && ctx->fds[j].fd != INVALID_SOCKET
          && ctx->clients[j].is_joined)
        {
          if (strcmp (ctx->clients[j].username, username) == 0)
            {
              name_taken = true;
              break;
            }
        }
    }

  if (name_taken)
    {
      resp.type = S2C_REJECT;
      strncpy (resp.payload, "Username already taken",
               MAX_PAYLOAD_SIZE - NULL_TERMINATOR_OFFSET);
      SerializePacket (&resp, out_buffer, sizeof (out_buffer));
      NetworkManager_Send (manager, client_index, out_buffer);
      return; // On arrête là si le nom est pris
    }

  // 2. Accept player
  strncpy (ctx->clients[client_index].username, username,
           MAX_USERNAME_LENGTH - NULL_TERMINATOR_OFFSET);
  ctx->clients[client_index].username[MAX_USERNAME_LENGTH] = '\0'; // Security
  ctx->clients[client_index].is_joined = true;

  // --- 1ER : LE LOG RÉSEAU (avec \n pour pousser le prompt) ---
  printf ("\n[SERVER] Player '%s' successfully joined Slot %d!\n", username,
          client_index);

  // --- 2EME : ENVOI DES PAQUETS AU CLIENT ---
  resp.type = S2C_WELCOME;
  strncpy (resp.payload, "Welcome !",
           MAX_PAYLOAD_SIZE - NULL_TERMINATOR_OFFSET);
  SerializePacket (&resp, out_buffer, sizeof (out_buffer));
  NetworkManager_Send (manager, client_index, out_buffer);

  Packet start_pkt;
  start_pkt.type = S2C_GAME_START;
  strcpy (start_pkt.payload,
          ROLE_BLACK_STR); // "1" pour dire au client qu'il est NOIR
  char out_buf[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&start_pkt, out_buf, sizeof (out_buf));
  NetworkManager_Send (manager, client_index, out_buf);

  // --- 3EME : MISE À JOUR DE L'INTERFACE LOCALE DU SERVEUR ---
  // C'est ce callback qui appelle refresh_prompt() final.
  if (manager->callbacks.OnGameStarted)
    manager->callbacks.OnGameStarted (PLAYER_WHITE);

  if (manager->callbacks.OnPlayerJoined)
    manager->callbacks.OnPlayerJoined (ctx->clients[client_index].username);
}

/**
 * @brief Relays a packet only to the correct opponent (same * game_id).
 */
static void
route_packet_to_opponent (NetworkManager *manager, ServerContext *ctx,
                          int sender_index, Packet *pkt)
{
  int sender_game_id = ctx->clients[sender_index].game_id;

  // If the player is not in a game (idle), nothing is routed.
  if (sender_game_id == NO_GAME_ID)
    return;

  // 1. We prepare the packet to be relayed.
  // A small detail: if the client sent C2S_MOVE, we transform it into
  // S2C_MOVE so that the other client understands it!
  Packet relay_pkt = *pkt;
  if (relay_pkt.type == C2S_MOVE)
    {
      relay_pkt.type = S2C_MOVE;
    }

  char out_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&relay_pkt, out_buffer, sizeof (out_buffer)) < 0)
    return;

  // 2. Loop: Browse clients[]
  for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
    {
      if (ctx->fds[i].fd != INVALID_SOCKET && ctx->clients[i].is_joined)
        {
          // 3. Condition: if (client.game_id == sender.game_id && client.fd
          // != sender.fd)
          if (ctx->clients[i].game_id == sender_game_id && i != sender_index)
            {
              // 4. Action: send() the package
              NetworkManager_Send (manager, i, out_buffer);
            }
        }
    }
}

void
handle_server_packet (NetworkManager *manager, ServerContext *ctx,
                      int client_index, Packet *pkt)
{

  switch (pkt->type)
    {
    case C2S_JOIN:
      process_join_packet (manager, ctx, client_index, pkt->payload);
      break;
    case PKT_TIMEOUT:
      // ROUTING : The opponent is notified that time is up.
      route_packet_to_opponent (manager, ctx, client_index, pkt);
      if (manager->callbacks.OnTimeoutReceived)
        manager->callbacks.OnTimeoutReceived ();
      break;

    case C2S_MOVE:
      // ROUTING: The move is relayed to the opponent
      route_packet_to_opponent (manager, ctx, client_index, pkt);
      process_move_packet (manager, pkt->payload);
      break;

    default:
      printf ("[SERVER] Unmanaged package (Type: %d)\n", pkt->type);
      break;
    }
}

void
handle_client_packet (NetworkManager *manager, Packet *pkt)
{
  switch (pkt->type)
    {
    case S2C_WELCOME:
      printf ("\n[CLIENT] Connection accepted by server: %s\n", pkt->payload);
      if (manager->callbacks.OnPlayerJoined)
        {

          manager->callbacks.OnPlayerJoined ("Server");
        }
      break;
    case PKT_TIMEOUT:
      if (manager->callbacks.OnTimeoutReceived)
        manager->callbacks.OnTimeoutReceived ();
      break;
    case S2C_REJECT:
      printf ("[CLIENT] Connection rejected: %s\n", pkt->payload);
      NetworkManager_Cleanup (manager);
      if (manager->callbacks.OnPlayerDisconnected)
        {
          manager->callbacks.OnPlayerDisconnected ("Username already taken");
        }
      break;

    case S2C_ERROR:
      if (manager->callbacks.OnGameError)
        {
          manager->callbacks.OnGameError (pkt->payload);
        }
      else
        {
          // Sinon on fait juste un print "sale"
          printf ("\n[CLIENT] Server Error: %s\n", pkt->payload);
        }
      break;
    case S2C_GAME_START:
      printf ("[CLIENT] Game is starting!\n");
      int color_val = atoi (pkt->payload);
      PlayerColor my_color
          = (color_val == ROLE_WHITE_VAL) ? PLAYER_WHITE : PLAYER_BLACK;

      if (manager->callbacks.OnGameStarted)
        {
          manager->callbacks.OnGameStarted (my_color);
        }
      break;

    case S2C_MOVE:
      process_move_packet (manager, pkt->payload);
      break;
    case S2C_INVITE:
      if (manager->callbacks.OnInvitationReceived)
        {
          manager->callbacks.OnInvitationReceived (atoi (pkt->payload));
        }

      break;

    default:
      printf ("[CLIENT] Unmanaged package (Type: %d)\n", pkt->type);
      break;
    }
}