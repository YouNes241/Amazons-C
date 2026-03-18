#include "network_includes/network_handlers.h"
#include "network_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

/**
 * @brief Helper to quickly build, serialize, and send a simple message packet.
 */
static void
send_server_message (NetworkManager *manager, int target_index,
                     PacketType type, const char *msg)
{
  Packet pkt;
  pkt.type = type;
  strncpy (pkt.payload, msg, MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char out_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, out_buffer, sizeof (out_buffer)) >= 0)
    {
      NetworkManager_Send (manager, target_index, out_buffer);
    }
}

/**
 * @brief Common helper to process MOVE packets for both Server and Client.
 */
static void
process_move_packet (NetworkManager *manager, const char *payload)
{
  if (payload == NULL)
    return;
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
  if (username == NULL)
    return;
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
      return; // We'll stop there if the name is already taken.
    }

  // 2. Accept player
  strncpy (ctx->clients[client_index].username, username,
           MAX_USERNAME_LENGTH - NULL_TERMINATOR_OFFSET);
  ctx->clients[client_index].username[MAX_USERNAME_LENGTH] = '\0'; // Security
  ctx->clients[client_index].is_joined = true;

  // --- LE FIX EST ICI : On place le joueur dans le Lobby ---
  ctx->clients[client_index].status = CLIENT_STATUS_IDLE;

  printf ("\n[SERVER] Player '%s' successfully joined Slot %d!\n", username,
          client_index);

  // packets sent to client (JUST THE WELCOME PACKET)
  resp.type = S2C_WELCOME;
  strncpy (resp.payload, "Welcome to the Lobby!",
           MAX_PAYLOAD_SIZE - NULL_TERMINATOR_OFFSET);
  SerializePacket (&resp, out_buffer, sizeof (out_buffer));
  NetworkManager_Send (manager, client_index, out_buffer);

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

/**
 * @brief Manages the receipt of an invitation request (C2S_INVITE)
 */
static void
process_invite_packet (NetworkManager *manager, ServerContext *ctx,
                       int sender_index, const char *payload)
{
  if (payload == NULL || payload[0] == '\0')
    return;
  int target_index = 0, board_size = 10;
  if (sscanf (payload, "%d %d", &target_index, &board_size) < 1)
    {
      target_index = atoi (payload);
    }

  // 1. Basic checks
  if (target_index < FIRST_CLIENT_INDEX || target_index > MAX_CLIENTS
      || target_index == sender_index)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: Invalid player ID.");
      return;
    }

  // 2. Advanced checks (exists and is joined)
  if (ctx->fds[target_index].fd == INVALID_SOCKET
      || !ctx->clients[target_index].is_joined)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: Player not found.");
      return;
    }

  // 3. Verify that the target player is indeed in the lobby (IDLE).
  if (ctx->clients[target_index].status == CLIENT_STATUS_AWAY)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: Player is AWAY.");
      return;
    }
  else if (ctx->clients[target_index].status != CLIENT_STATUS_IDLE)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: Player is not available.");
      return;
    }

  // 4. Everything is good: We put the sender on hold
  ctx->clients[sender_index].status = CLIENT_STATUS_WAITGAME;
  ctx->clients[sender_index].board_size = board_size;

  // 5. The invitation is sent to the target (Payload = the sender's ID)
  char sender_id_str[32];
  snprintf (sender_id_str, sizeof (sender_id_str), "%d %d", sender_index,
            board_size);
  send_server_message (manager, target_index, S2C_INVITE, sender_id_str);
}

/**
 * @brief Handles the acceptance of an invitation (C2S_ACCEPT)
 */
static void
process_accept_packet (NetworkManager *manager, ServerContext *ctx,
                       int sender_index, const char *payload)
{
  if (payload == NULL || payload[0] == '\0')
    return;

  int challenger_index = atoi (payload);

  if (challenger_index < FIRST_CLIENT_INDEX || challenger_index > MAX_CLIENTS)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: Invalid ID.");
      return;
    }

  // Check if the challenger is still there and still waiting
  if (ctx->fds[challenger_index].fd == INVALID_SOCKET
      || !ctx->clients[challenger_index].is_joined
      || ctx->clients[challenger_index].status != CLIENT_STATUS_WAITGAME)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: Invite expired or player left.");
      ctx->clients[sender_index].status
          = CLIENT_STATUS_IDLE; // We put the client back in idle.
      return;
    }

  //  invitation accepted : starting the game
  int new_game_id = ctx->next_game_id++; // We take a ticket and increment it.

  int board_size = ctx->clients[challenger_index].board_size;

  // We assign the ID and change the statuses.
  ctx->clients[challenger_index].game_id = new_game_id;
  ctx->clients[challenger_index].status = CLIENT_STATUS_INGAME;

  ctx->clients[sender_index].game_id = new_game_id;
  ctx->clients[sender_index].status = CLIENT_STATUS_INGAME;

  char payload_white[MAX_PAYLOAD_SIZE], payload_black[MAX_PAYLOAD_SIZE];
  snprintf (payload_white, MAX_PAYLOAD_SIZE, "%d %d", ROLE_WHITE_VAL,
            board_size);
  snprintf (payload_black, MAX_PAYLOAD_SIZE, "%d %d", ROLE_BLACK_VAL,
            board_size);
  // Sending to the Challenger (He will play the WHITES)
  send_server_message (manager, challenger_index, S2C_GAME_START,
                       payload_white);

  // Send to the one who accepted (He will play BLACK)
  send_server_message (manager, sender_index, S2C_GAME_START, payload_black);
}

/**
 * @brief Handles the rejection of an invitation (C2S_DECLINE)
 */
static void
process_decline_packet (NetworkManager *manager, ServerContext *ctx,
                        int sender_index, const char *payload)
{
  if (payload == NULL || payload[0] == '\0')
    return;
  int challenger_index = atoi (payload);

  // Put the player who refused back into IDLE mode
  ctx->clients[sender_index].status = CLIENT_STATUS_IDLE;

  // If the challenger is still connected, we put them back to IDLE and we warn
  // them
  if (challenger_index >= FIRST_CLIENT_INDEX && challenger_index <= MAX_CLIENTS
      && ctx->fds[challenger_index].fd != INVALID_SOCKET)
    {
      ctx->clients[challenger_index].status = CLIENT_STATUS_IDLE;
      send_server_message (manager, challenger_index, S2C_DECLINE,
                           "Invitation declined.");
    }
}

/**
 * @brief Compiles and sends the list of active players to the requester.
 */
static void
process_list_packet (NetworkManager *manager, ServerContext *ctx,
                     int client_index, const char *payload)
{
  (void)ctx;
  int target_id = -1;
  if (payload && payload[0] != '\0')
    {
      target_id = atoi (payload);
    }

  char list_buffer[MAX_PAYLOAD_SIZE];

  NetworkManager_GetServerPlayerList (manager, list_buffer,
                                      sizeof (list_buffer), target_id);

  Packet resp;
  resp.type = S2C_LIST;
  strncpy (resp.payload, list_buffer, MAX_PAYLOAD_SIZE - 1);
  resp.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char out_buffer[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&resp, out_buffer, sizeof (out_buffer));
  NetworkManager_Send (manager, client_index, out_buffer);
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

    case C2S_INVITE:
      process_invite_packet (manager, ctx, client_index, pkt->payload);
      break;

    case C2S_ACCEPT:
      process_accept_packet (manager, ctx, client_index, pkt->payload);
      break;

    case C2S_PING:
      {
        send_server_message (manager, client_index, S2C_PONG, pkt->payload);
      }
      break;

    case C2S_DECLINE:
      process_decline_packet (manager, ctx, client_index, pkt->payload);
      break;
    case C2S_AWAY:
      if (ctx->clients[client_index].status == CLIENT_STATUS_IDLE)
        {
          ctx->clients[client_index].status = CLIENT_STATUS_AWAY;
          printf ("[SERVER] Player %d is now AWAY.\n", client_index);
          printf ("[LOBBY] >> ");
          fflush (stdout);
        }
      break;

    case C2S_BACK:
      if (ctx->clients[client_index].status == CLIENT_STATUS_AWAY)
        {
          ctx->clients[client_index].status = CLIENT_STATUS_IDLE;
          printf ("[SERVER] Player %d is BACK.\n", client_index);
          printf ("[LOBBY] >> ");
          fflush (stdout);
        }
      break;
    case C2S_CANCEL:
      if (ctx->clients[client_index].status == CLIENT_STATUS_WAITGAME)
        {
          ctx->clients[client_index].status = CLIENT_STATUS_IDLE;

          int target_index = atoi (pkt->payload);
          if (target_index >= FIRST_CLIENT_INDEX && target_index <= MAX_CLIENTS
              && ctx->fds[target_index].fd != INVALID_SOCKET)
            {

              ctx->clients[target_index].status = CLIENT_STATUS_IDLE;

              send_server_message (manager, target_index, S2C_CANCEL,
                                   "Invitation canceled by challenger.");
            }
          printf ("[SERVER] Player %d cancelled their invitation.\n",
                  client_index);
          printf ("[LOBBY] >> ");
          fflush (stdout);
        }
      break;
    case C2S_SCOREBOARD:
      {
        // On envoie l'en-tête
        send_server_message (manager, client_index, S2C_SCOREBOARD,
                             "Name        | Wins | Losses | Played");
        send_server_message (manager, client_index, S2C_SCOREBOARD,
                             "-----------------------------------");

        FILE *f = fopen ("scoreboard.txt", "r");
        bool has_scores = false;

        if (f)
          {
            char line[128];
            char name[32];
            int w, l, g;
            // On envoie chaque joueur ligne par ligne
            while (fscanf (f, "%s %d %d %d", name, &w, &l, &g) != EOF)
              {
                snprintf (line, sizeof (line), "%-10s | %4d | %6d | %6d", name,
                          w, l, g);
                send_server_message (manager, client_index, S2C_SCOREBOARD,
                                     line);
                has_scores = true;
              }
            fclose (f);
          }

        if (!has_scores)
          {
            send_server_message (manager, client_index, S2C_SCOREBOARD,
                                 "No scores recorded yet.");
          }

        // Un petit message "clé" pour dire au client que l'affichage est fini
        send_server_message (manager, client_index, S2C_SCOREBOARD,
                             "END_OF_SCOREBOARD");
      }
      break;
    case C2S_GAME_OVER:
      {
        // On récupère le client qui a envoyé le message
        ConnectedClient *client = &manager->server_ctx->clients[client_index];

        if (strcmp (pkt->payload, "WIN") == 0)
          {
            NetworkManager_UpdatePlayerStats (client->username, true);
          }
        else
          {
            NetworkManager_UpdatePlayerStats (client->username, false);
          }

        // On remet le statut du joueur à IDLE pour qu'il puisse rejouer
        client->status = CLIENT_STATUS_IDLE;
        client->game_id = NO_GAME_ID;
      }
      break;
    case C2S_LIST:
      process_list_packet (manager, ctx, client_index, pkt->payload);
      break;

    case PKT_TIMEOUT:
      // ROUTING : The opponent is notified that time is up.
      route_packet_to_opponent (manager, ctx, client_index, pkt);

      break;

    case C2S_MOVE:
      // ROUTING: The move is relayed to the opponent
      route_packet_to_opponent (manager, ctx, client_index, pkt);

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

    case S2C_LIST:
      if (manager->callbacks.OnListReceived)
        {
          manager->callbacks.OnListReceived (pkt->payload);
        }
      break;
    case S2C_SCOREBOARD:
      {
        if (strcmp (pkt->payload, "END_OF_SCOREBOARD") == 0)
          {
            // Fin de la liste, on redessine proprement le prompt
            printf ("[LOBBY] >> ");
            fflush (stdout);
          }
        else
          {
            // On affiche simplement la ligne reçue avec un retour à la ligne
            // local
            printf ("%s\n", pkt->payload);
          }
      }
      break;
    case S2C_PONG:
      {
        struct timeval tv;
        gettimeofday (&tv, NULL);
        long long now_ms = (long long)tv.tv_sec * 1000 + (tv.tv_usec / 1000);

        long long sent_time = atoll (pkt->payload); // On lit l'heure de départ
        long long rtt = now_ms - sent_time;         // Round-Trip Time

        printf ("\nServer: PONG TIME=%lldms\n", rtt);

        printf ("[LOBBY] >> ");
        fflush (stdout);
      }
      break;

    case S2C_ERROR:
      if (manager->callbacks.OnGameError)
        {
          manager->callbacks.OnGameError (pkt->payload);
        }
      else
        {

          printf ("\n[CLIENT] Server Error: %s\n", pkt->payload);
        }
      break;
    case S2C_DECLINE:
      if (manager->callbacks.OnInvitationDeclined)
        {
          manager->callbacks.OnInvitationDeclined (pkt->payload);
        }
      break;

    case S2C_CANCEL:
      if (manager->callbacks.OnInvitationDeclined)
        {

          manager->callbacks.OnInvitationDeclined (pkt->payload);
        }
      break;
    case S2C_OPPONENT_LEFT:

      if (manager->callbacks.OnPlayerDisconnected)
        {
          manager->callbacks.OnPlayerDisconnected (pkt->payload);
        }
      break;
    case S2C_GAME_START:
      printf ("[CLIENT] Game is starting!\n");
      int color_val = 0, board_size = 10;
      sscanf (pkt->payload, "%d %d", &color_val, &board_size);
      PlayerColor my_color
          = (color_val == ROLE_WHITE_VAL) ? PLAYER_WHITE : PLAYER_BLACK;

      if (manager->callbacks.OnGameStarted)
        {
          manager->callbacks.OnGameStarted (my_color, board_size);
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