/** @file network_handler_client.c
 * @brief Implementation of packet handlers for the Client.
 *
 * This file contains the logic for processing incoming packets on the Client
 * side. It includes functions to handle various message types such as WELCOME,
 * REJECT, LIST, GAME_START, MOVE, etc., and ensures that the client's game
 * state is updated accordingly based on the received data.
 */

#include "network_includes/network_handlers.h"
#include "network_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/**
 * @brief Handles incoming packets for the client.
 * @param manager Pointer to the NetworkManager instance.
 * @param pkt Pointer to the received packet.
 *
 * This function processes packets received by the client from the server and
 * triggers the appropriate callbacks based on the packet type. It handles
 * various message types such as WELCOME, REJECT, LIST, GAME_START, MOVE, etc.,
 * and ensures that the client's game state is updated accordingly based on the
 * received data.
 */
void
handle_client_packet (NetworkManager *manager, Packet *pkt)
{
  switch (pkt->type)
    {
      /** @brief Processes a WELCOME packet received from the server. */
    case S2C_WELCOME:
      printf ("\n[CLIENT] Connection accepted by server: %s\n", pkt->payload);
      if (manager->callbacks.OnPlayerJoined)
        {

          manager->callbacks.OnPlayerJoined ("Server");
        }
      break;
      /** @brief Processes a TIMEOUT packet received from the server. */
    case PKT_TIMEOUT:
      if (manager->callbacks.OnTimeoutReceived)
        manager->callbacks.OnTimeoutReceived ();
      break;
      /** @brief Processes a REJECT packet received from the server. */
    case S2C_REJECT:
      printf (_ ("[CLIENT] Connection rejected: %s\n"), pkt->payload);
      NetworkManager_Cleanup (manager);
      if (manager->callbacks.OnPlayerDisconnected)
        {
          manager->callbacks.OnPlayerDisconnected (pkt->payload);
        }
      NetworkManager_Cleanup (manager);
      break;
      /** @brief Processes a LIST packet received from the server. */
    case S2C_LIST:
      if (manager->callbacks.OnListReceived)
        {
          manager->callbacks.OnListReceived (pkt->payload);
        }
      break;
      /** @brief Processes a SCOREBOARD packet received from the server. */
    case S2C_SCOREBOARD:
      {
        if (strcmp (pkt->payload, "END_OF_SCOREBOARD") == 0)
          {
            // Fin de la liste, on redessine proprement le prompt
            printf (_ ("[LOBBY] >> "));
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
      /** @brief Processes a PONG packet received from the server. */
    case S2C_PONG:
      {
        struct timeval tv;
        gettimeofday (&tv, NULL);
        long long now_ms = (long long)tv.tv_sec * 1000 + (tv.tv_usec / 1000);

        long long sent_time = atoll (pkt->payload); // On lit l'heure de départ
        long long rtt = now_ms - sent_time;         // Round-Trip Time

        printf (_ ("\nServer: PONG TIME=%lldms\n"), rtt);

        printf (_ ("[LOBBY] >> "));
        fflush (stdout);
      }
      break;
      /** @brief Processes an ERROR packet received from the server. */
    case S2C_ERROR:
      if (manager->callbacks.OnGameError)
        {
          manager->callbacks.OnGameError (pkt->payload);
        }
      else
        {

          printf (_ ("\n[CLIENT] Server Error: %s\n"), pkt->payload);
        }
      break;
      /** @brief Processes a DECLINE packet received from the server. */
    case S2C_DECLINE:
      if (manager->callbacks.OnInvitationDeclined)
        {
          manager->callbacks.OnInvitationDeclined (pkt->payload);
        }
      break;
      /** @brief Processes a CANCEL packet received from the server. */
    case S2C_CANCEL:
      if (manager->callbacks.OnInvitationDeclined)
        {

          manager->callbacks.OnInvitationDeclined (pkt->payload);
        }
      break;
      /** @brief Processes an OPPONENT_LEFT packet received from the server. */
    case S2C_OPPONENT_LEFT:

      if (manager->callbacks.OnPlayerDisconnected)
        {
          manager->callbacks.OnPlayerDisconnected (pkt->payload);
        }
      break;
      /** @brief Processes a GAME_START packet received from the server. */
    case S2C_GAME_START:
      printf (_ ("[CLIENT] Game is starting!\n"));
      int color_val = 0, board_size = 10;
      sscanf (pkt->payload, "%d %d", &color_val, &board_size);
      PlayerColor my_color
          = (color_val == ROLE_WHITE_VAL) ? PLAYER_WHITE : PLAYER_BLACK;

      if (manager->callbacks.OnGameStarted)
        {
          manager->callbacks.OnGameStarted (my_color, board_size);
        }
      break;
      /** @brief Processes a MOVE packet received from the server. */
    case S2C_MOVE:
      if (manager->callbacks.OnMoveReceived)
        {
          Move incoming_move;
          // On traduit le texte reçu en structure Move
          if (Protocol_StringToMove (pkt->payload, &incoming_move))
            {
              // Si la traduction réussit, on passe la VRAIE structure au jeu
              manager->callbacks.OnMoveReceived (incoming_move);
            }
          else
            {
              printf (
                  _ ("[CLIENT] Error: Received unreadable move format from "
                     "server: %s\n"),
                  pkt->payload);
            }
        }
      break;
      /** @brief Processes an INVITE packet received from the server. */
    case S2C_INVITE:
      if (manager->callbacks.OnInvitationReceived)
        {
          manager->callbacks.OnInvitationReceived (atoi (pkt->payload));
        }

      break;
      /** @brief Processes an unknown packet received from the server. */
    default:
      printf (_ ("[CLIENT] Unmanaged package (Type: %d)\n"), pkt->type);
      break;
    }
}