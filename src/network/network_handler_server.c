/**
 * @file network_handler_server.c
 * @brief Implementation of packet handlers for the Server.
 *
 * This file contains the logic for processing incoming packets on the
 * Server side. It includes functions to handle various message types such as
 * Join, Move, etc., and ensures that the game state is updated accordingly
 * based on the received data. It also manages client disconnections and the
 * routing of messages to the appropriate clients.
 */

#include "game_engine.h"
#include "network_includes/network_handlers.h"
#include "network_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * @brief Disconnects a client from the server.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param client_index Index of the client to disconnect.
 */
void
disconnect_server_client (NetworkManager *manager, ServerContext *ctx,
                          int client_index)
{
  printf ("\n[SERVER] Client %d has disconnected.\n", client_index);

  int game_id = ctx->clients[client_index].game_id;
  if (game_id != NO_GAME_ID)
    {
      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          // if there is still another player in the game
          if (i != client_index && ctx->fds[i].fd != INVALID_SOCKET
              && ctx->clients[i].game_id == game_id)
            {
              // we send him back to the lobby
              ctx->clients[i].status = CLIENT_STATUS_IDLE;
              ctx->clients[i].game_id = NO_GAME_ID;

              Packet pkt;
              pkt.type = S2C_OPPONENT_LEFT;
              strncpy (pkt.payload, "Opponent disconnected.",
                       MAX_PAYLOAD_SIZE - 1);
              char out_buf[MAX_FORMATTED_PKT_SIZE];
              if (SerializePacket (&pkt, out_buf, sizeof (out_buf)) >= 0)
                {
                  NetworkManager_Send (manager, i, out_buf);
                }
            }
        }
    }

  close (ctx->fds[client_index].fd);
  ctx->fds[client_index].fd = INVALID_SOCKET;
  ctx->clients[client_index].is_joined = false;
  ctx->clients[client_index].status = CLIENT_STATUS_CONNECTED;
  ctx->clients[client_index].game_id = NO_GAME_ID;
  ctx->num_clients--;

  if (manager->callbacks.OnPlayerDisconnected)
    manager->callbacks.OnPlayerDisconnected ("Network disconnection");
}

/** @brief Processes all complete packets extracted from a TCP buffer.
 * @param manager Pointer to the NetworkManager instance.
 * @param buffer Pointer to the TCP buffer containing the data.
 * @param client_index Index of the client associated with the buffer.
 */
void
process_buffer_packets (NetworkManager *manager, tcp_buffer_t *buffer,
                        int client_index)
{
  char extracted_msg[RECV_TEMP_BUFFER_SIZE];
  int res;
  const char *log_prefix
      = (manager->current_mode == NET_MODE_SERVER) ? "[SERVER]" : "[CLIENT]";

  while ((res = tcp_buffer_extract_line (buffer, extracted_msg,
                                         sizeof (extracted_msg)))
         != EXTRACT_NO_MSG)
    {
      if (res == EXTRACT_MSG_OVERFLOW)
        {
          printf ("%s Warning: oversized message discarded.\n", log_prefix);
          continue;
        }

      Packet pkt;
      if (!DeserializePacket (extracted_msg, &pkt))
        {
          printf ("%s Incorrect format of received package : %s\n", log_prefix,
                  extracted_msg);
          continue;
        }

      // Route the packet to the correct handler
      if (manager->current_mode == NET_MODE_SERVER)
        {
          handle_server_packet (manager, manager->server_ctx, client_index,
                                &pkt);
        }
      else
        {
          handle_client_packet (manager, &pkt);
        }
    }
}

/** @brief Processes a MOVE packet received from a client.
 * @param manager Pointer to the NetworkManager instance.
 * @param payload The payload of the MOVE packet.
 */
static void
process_move_packet (NetworkManager *manager, ServerContext *ctx,
                     int client_index, const char *payload)
{
  if (payload == NULL)
    return;
  int game_idx = ctx->clients[client_index].game_id;
  if (game_idx == NO_GAME_ID || game_idx < 0 || game_idx >= (MAX_CLIENTS / 2))
    return;

  int opponent_idx = -1;
  for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
    {
      if (i != client_index && ctx->clients[i].game_id == game_idx
          && ctx->clients[i].status == CLIENT_STATUS_INGAME)
        {
          opponent_idx = i;
          break;
        }
    }

  if (opponent_idx == -1)
    return;

  GameState *server_board = &ctx->active_games[game_idx];

  Move parsed_move;
  if (!Protocol_StringToMove (payload, &parsed_move))
    {
      // if text format is wrong, we ignore the move and we warn the client
      // that he sent a wrong format (but we don't disconnect him, maybe it was
      // just a typo)
      printf ("[SERVER] Hack attempt! Invalid move format from Client %d.\n",
              client_index);
      return;
    }

  if (is_move_legal (server_board, parsed_move))
    {
      // 1. On met VRAIMENT à jour le plateau de l'arbitre (le serveur)
      apply_move (server_board, parsed_move);

      // 2. On prépare un paquet propre pour l'adversaire
      Packet relay_pkt;
      relay_pkt.type = S2C_MOVE;
      strncpy (relay_pkt.payload, payload, MAX_PAYLOAD_SIZE - 1);
      relay_pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

      char out_buffer[MAX_FORMATTED_PKT_SIZE];

      // SerializePacket va ajouter le type ET le \n vital à la fin !
      if (SerializePacket (&relay_pkt, out_buffer, sizeof (out_buffer)) >= 0)
        {
          NetworkManager_Send (manager, opponent_idx, out_buffer);
        }
    }
  else
    {
      // if the move is illegal, we ignore it and we warn the client that he
      // tried to cheat (but we don't disconnect him, maybe it was just a
      // mistake)
      printf ("\n[SERVER ANTI-CHEAT] Client %d tried an illegal move: %s\n",
              client_index, payload);

      Packet err_pkt;
      err_pkt.type = S2C_ERROR;
      strncpy (err_pkt.payload,
               "Illegal move detected! You have been desynced.",
               MAX_PAYLOAD_SIZE - 1);

      char out_buf[MAX_FORMATTED_PKT_SIZE];
      SerializePacket (&err_pkt, out_buf, sizeof (out_buf));
      NetworkManager_Send (manager, client_index, out_buf);

      // Optionally, we could disconnect the client here if we want to be
      // strict: disconnect_server_client(manager, ctx, client_index);
    }
}

/** @brief Sends a message to a specific client.
 * @param manager Pointer to the NetworkManager instance.
 * @param target_index Index of the target client.
 * @param type Type of the packet to send.
 * @param msg Message to send.
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

/** @brief Processes a JOIN packet received from a client.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param client_index Index of the client sending the packet.
 * @param username The username of the player attempting to join.
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

  ctx->clients[client_index].username[MAX_USERNAME_LENGTH - 1]
      = '\0'; // Security
  ctx->clients[client_index].is_joined = true;
  ctx->clients[client_index].pending_invite_from = -1;
  ctx->clients[client_index].status = CLIENT_STATUS_IDLE;
  ctx->clients[client_index].wins = 0;
  ctx->clients[client_index].losses = 0;
  ctx->clients[client_index].games_played = 0;
  LoadScoreboardForClient (&ctx->clients[client_index]);

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

/** @brief Routes a packet only to the correct opponent (same game_id).
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param sender_index Index of the client sending the packet.
 * @param pkt Pointer to the packet to route.
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

/** @brief Processes an invitation packet received from a client.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param sender_index Index of the client sending the packet.
 * @param payload The payload of the invitation packet.
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
  if (ctx->clients[target_index].pending_invite_from != -1)
    {
      send_server_message (
          manager, sender_index, S2C_ERROR,
          "Error: Player is already considering another invitation.");
      return;
    }
  // 4. Everything is good: We put the sender on hold
  ctx->clients[sender_index].status = CLIENT_STATUS_WAITGAME;
  ctx->clients[sender_index].board_size = board_size;
  ctx->clients[target_index].pending_invite_from = sender_index;
  ctx->clients[target_index].invite_timestamp = time (NULL);

  // 5. The invitation is sent to the target (Payload = the sender's ID)
  char sender_id_str[32];
  snprintf (sender_id_str, sizeof (sender_id_str), "%d %d", sender_index,
            board_size);
  send_server_message (manager, target_index, S2C_INVITE, sender_id_str);
}

/** @brief Processes an acceptance packet received from a client.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param sender_index Index of the client sending the packet.
 * @param payload The payload of the acceptance packet.
 */
static void
process_accept_packet (NetworkManager *manager, ServerContext *ctx,
                       int sender_index, const char *payload)
{
  if (payload == NULL)
    return;
  int challenger_index = ctx->clients[sender_index].pending_invite_from;

  if (challenger_index == -1)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: You have no pending invitations.");
      return;
    }
  // Clear the pending invite to prevent multiple acceptances
  ctx->clients[sender_index].pending_invite_from = -1;

  // Check if the challenger is still there and still waiting
  if (ctx->fds[challenger_index].fd == INVALID_SOCKET
      || !ctx->clients[challenger_index].is_joined
      || ctx->clients[challenger_index].status != CLIENT_STATUS_WAITGAME)
    {
      send_server_message (
          manager, sender_index, S2C_ERROR,
          "Error: The challenger left or the invite expired.");
      ctx->clients[sender_index].status
          = CLIENT_STATUS_IDLE; // We put the client back in idle.
      return;
    }

  //  invitation accepted : starting the game
  int new_game_id = ctx->next_game_id++; // We take a ticket and increment it.

  int game_index = new_game_id % (MAX_CLIENTS / 2);

  ctx->active_games[game_index].size
      = ctx->clients[challenger_index]
            .board_size; // board size is decided by the challenger and
                         // stored in the client context when he sends
                         // the invite. We retrieve it here.
  game_init (&ctx->active_games[game_index]);
  ctx->active_games[game_index].current_player = PLAYER_WHITE;
  ctx->clients[sender_index].game_id = game_index;
  ctx->clients[challenger_index].game_id = game_index;

  ctx->clients[sender_index].status = CLIENT_STATUS_INGAME;
  ctx->clients[challenger_index].status = CLIENT_STATUS_INGAME;

  char payload_white[MAX_PAYLOAD_SIZE], payload_black[MAX_PAYLOAD_SIZE];
  snprintf (payload_white, MAX_PAYLOAD_SIZE, "%d %d", ROLE_WHITE_VAL,
            ctx->clients[challenger_index].board_size);
  snprintf (payload_black, MAX_PAYLOAD_SIZE, "%d %d", ROLE_BLACK_VAL,
            ctx->clients[challenger_index].board_size);
  // Sending to the Challenger (He will play the WHITES)
  send_server_message (manager, challenger_index, S2C_GAME_START,
                       payload_white);

  // Send to the one who accepted (He will play BLACK)
  send_server_message (manager, sender_index, S2C_GAME_START, payload_black);
}

/** @brief Processes a decline packet received from a client.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param sender_index Index of the client sending the packet.
 * @param payload The payload of the decline packet.
 */
static void
process_decline_packet (NetworkManager *manager, ServerContext *ctx,
                        int sender_index, const char *payload)
{
  if (payload == NULL)
    return;
  int challenger_index = ctx->clients[sender_index].pending_invite_from;

  // If the challenger is still connected, we put them back to IDLE and we warn
  // them
  if (challenger_index == -1)
    {
      send_server_message (manager, sender_index, S2C_ERROR,
                           "Error: You have no pending invitations.");
      return;
    }
  // Put the player who refused back into IDLE mode
  ctx->clients[sender_index].status = CLIENT_STATUS_IDLE;
  ctx->clients[sender_index].pending_invite_from = -1;

  if (challenger_index >= FIRST_CLIENT_INDEX && challenger_index <= MAX_CLIENTS
      && ctx->fds[challenger_index].fd != INVALID_SOCKET)
    {
      // On le libère de son statut d'attente
      ctx->clients[challenger_index].status = CLIENT_STATUS_IDLE;

      // On le prévient que son invitation a été refusée !
      send_server_message (manager, challenger_index, S2C_DECLINE,
                           "Invitation declined by the opponent.");
    }
}

/** @brief Processes a list packet received from a client.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param client_index Index of the client sending the packet.
 * @param payload The payload of the list packet.
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

/** @brief Handles incoming packets for the server.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param client_index Index of the client sending the packet.
 * @param pkt Pointer to the received packet.
 */
void
handle_server_packet (NetworkManager *manager, ServerContext *ctx,
                      int client_index, Packet *pkt)
{

  switch (pkt->type)
    {
      /** @brief Processes a JOIN packet received from a client. */
    case C2S_JOIN:
      process_join_packet (manager, ctx, client_index, pkt->payload);
      break;
      /** @brief Processes an invitation packet received from a client. */
    case C2S_INVITE:
      process_invite_packet (manager, ctx, client_index, pkt->payload);
      break;
      /** @brief Processes an acceptance packet received from a client. */
    case C2S_ACCEPT:
      process_accept_packet (manager, ctx, client_index, pkt->payload);
      break;
      /** @brief Processes a PING packet received from a client. */
    case C2S_PING:
      {
        send_server_message (manager, client_index, S2C_PONG, pkt->payload);
      }
      break;
      /** @brief Processes a decline packet received from a client. */
    case C2S_DECLINE:
      process_decline_packet (manager, ctx, client_index, pkt->payload);
      break;
      /** @brief Processes an AWAY packet received from a client. */
    case C2S_AWAY:
      if (ctx->clients[client_index].status == CLIENT_STATUS_IDLE)
        {
          ctx->clients[client_index].status = CLIENT_STATUS_AWAY;
          printf ("[SERVER] Player %d is now AWAY.\n", client_index);
          printf ("[LOBBY] >> ");
          fflush (stdout);
        }
      break;
      /** @brief Processes a BACK packet received from a client. */
    case C2S_BACK:
      if (ctx->clients[client_index].status == CLIENT_STATUS_AWAY)
        {
          ctx->clients[client_index].status = CLIENT_STATUS_IDLE;
          printf ("[SERVER] Player %d is BACK.\n", client_index);
          printf ("[LOBBY] >> ");
          fflush (stdout);
        }
      break;
      /** @brief Processes a CANCEL packet received from a client. */
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
      /** @brief Processes a SCOREBOARD packet received from a client. */
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
      /** @brief Processes a GAME_OVER packet received from a client. */
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
      /** @brief Processes a LIST packet received from a client. */
    case C2S_LIST:
      process_list_packet (manager, ctx, client_index, pkt->payload);
      break;
      /** @brief Processes a TIMEOUT packet received from a client. */
    case PKT_TIMEOUT:
      // ROUTING : The opponent is notified that time is up.
      route_packet_to_opponent (manager, ctx, client_index, pkt);
      break;
      /** @brief Processes a MOVE packet received from a client. */
    case C2S_MOVE:
      // ROUTING: The move is relayed to the opponent
      process_move_packet (manager, ctx, client_index, pkt->payload);
      break;

      /** @brief Processes an unknown packet received from a client. */
    default:
      printf ("[SERVER] Unmanaged package (Type: %d)\n", pkt->type);
      break;
    }
}
