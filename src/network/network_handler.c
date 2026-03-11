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
  char out_buffer[256];
  Packet resp;

  // 1. Check if the username is already taken.
  bool name_taken = false;
  for (int j = 1; j <= MAX_CLIENTS; j++)
    {
      if (j != client_index && ctx->fds[j].fd != -1
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
      strncpy (resp.payload, "Username already taken", MAX_PAYLOAD_SIZE - 1);
      SerializePacket (&resp, out_buffer, sizeof (out_buffer));
      NetworkManager_Send (manager, client_index, out_buffer);
      return; // On arrête là si le nom est pris
    }

  // 2. Accept player
  strncpy (ctx->clients[client_index].username, username, 31);
  ctx->clients[client_index].username[31] = '\0'; // Security
  ctx->clients[client_index].is_joined = true;

  // --- 1ER : LE LOG RÉSEAU (avec \n pour pousser le prompt) ---
  printf ("\n[SERVER] Player '%s' successfully joined Slot %d!\n", username,
          client_index);

  // --- 2EME : ENVOI DES PAQUETS AU CLIENT ---
  resp.type = S2C_WELCOME;
  strncpy (resp.payload, "Welcome !", MAX_PAYLOAD_SIZE - 1);
  SerializePacket (&resp, out_buffer, sizeof (out_buffer));
  NetworkManager_Send (manager, client_index, out_buffer);

  Packet start_pkt;
  start_pkt.type = S2C_GAME_START;
  strcpy (start_pkt.payload, "1"); // "1" pour dire au client qu'il est NOIR
  char out_buf[256];
  SerializePacket (&start_pkt, out_buf, sizeof (out_buf));
  NetworkManager_Send (manager, client_index, out_buf);

    if (manager->callbacks.OnGameStarted)
    manager->callbacks.OnGameStarted (PLAYER_WHITE);

  if (manager->callbacks.OnPlayerJoined)
    manager->callbacks.OnPlayerJoined (ctx->clients[client_index].username);
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
      if (manager->callbacks.OnTimeoutReceived)
        manager->callbacks.OnTimeoutReceived ();
      break;

    case C2S_MOVE:
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
      PlayerColor my_color = (color_val == 0) ? PLAYER_WHITE : PLAYER_BLACK;

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