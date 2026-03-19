/** @file network_queries.c
 * @brief Implementation of network query functions for the Amazons game.
 *
 * This file contains functions that allow the client to query the server for
 * information such as the list of available servers, the status of the current
 * server, and the list of connected players. These functions are used to
 * provide feedback to the user and to facilitate interactions with the server.
 */

#include "network_common.h"
#include "network_includes/network_internal.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/** @brief Checks if an opponent is connected.
 * @param manager Pointer to the NetworkManager instance.
 * @return HAS_OPPONENT if an opponent is connected, NO_OPPONENT otherwise.
 */
int
NetworkManager_HasOpponent (NetworkManager *manager)
{
  if (manager == NULL)
    return NO_OPPONENT;

  if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      return (manager->server_ctx->num_clients > 0) ? HAS_OPPONENT
                                                    : NO_OPPONENT;
    }

  if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      return HAS_OPPONENT;
    }

  return NO_OPPONENT;
}

/** @brief Retrieves a list of available servers on the local network.
 * @param manager Pointer to the NetworkManager instance.
 * @param buffer Pointer to the buffer where the server list will be stored.
 * @param max_len Maximum length of the buffer.
 * @return The number of servers found, or -1 on error.
 */
int
NetworkManager_GetServerList (NetworkManager *manager, char *buffer,
                              size_t max_len)
{
  if (!manager || !manager->discovery_ctx || !buffer || max_len == 0)
    return -1;

  buffer[0] = '\0';
  int count = 0;
  time_t now = time (NULL);

  for (int i = 0; i < manager->discovery_ctx->server_count; i++)
    {
      // On vérifie le timeout de 30 secondes ici !
      if (difftime (now, manager->discovery_ctx->servers[i].last_seen)
          <= SERVER_TIMEOUT_SEC)
        {
          char line[128];
          snprintf (line, sizeof (line), "[%d] %s (IP: %s, Port: %d)\n",
                    count + 1, manager->discovery_ctx->servers[i].name,
                    manager->discovery_ctx->servers[i].ip,
                    manager->discovery_ctx->servers[i].port);

          if (strlen (buffer) + strlen (line) < max_len - 1)
            {
              strcat (buffer, line);
              count++;
            }
        }
    }

  if (count == 0)
    {
      snprintf (buffer, max_len, "No servers found on the local network.\n");
    }

  return count;
}

/**
 * @brief gets the current server status, including number of clients and
 * games.
 * @param manager The network manager
 * @param out_clients Pointer to retrieve the number of clients
 * @param out_games Pointer to retrieve the number of games (1 game = 2 players
 * INGAME)
 * @return NET_SUCCESS if it's an active server, NET_ERROR otherwise.
 */
int
NetworkManager_GetServerStatus (NetworkManager *manager, int *out_clients,
                                int *out_games, int *out_port)
{
  if (manager == NULL || manager->current_mode != NET_MODE_SERVER
      || manager->server_ctx == NULL)
    return NET_ERROR;

  if (out_clients)
    *out_clients = manager->server_ctx->num_clients;

  if (out_port)
    *out_port = manager->server_ctx->port;

  if (out_games)
    {
      int ingame_players = 0;
      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          if (manager->server_ctx->fds[i].fd != INVALID_SOCKET
              && manager->server_ctx->clients[i].status
                     == CLIENT_STATUS_INGAME)
            {
              ingame_players++;
            }
        }
      *out_games = ingame_players / 2;
    }

  return NET_SUCCESS;
}

void
NetworkManager_GetServerPlayerList (NetworkManager *manager, char *buffer,
                                    size_t max_len, int target_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_SERVER
      || manager->server_ctx == NULL || buffer == NULL || max_len == 0)
    {
      if (buffer && max_len > 0)
        buffer[0] = '\0';
      return;
    }

  buffer[0] = '\0'; // we will build the string progressively, starting with an
                    // empty buffer

  // =========================================================================
  // case 1 : "players <id>" -> Detailed info about a specific player
  // =========================================================================
  if (target_id >= 0)
    {
      // verify if the target player exists and is connected
      if (target_id < FIRST_CLIENT_INDEX || target_id > MAX_CLIENTS
          || manager->server_ctx->fds[target_id].fd == INVALID_SOCKET
          || !manager->server_ctx->clients[target_id].is_joined)
        {
          snprintf (buffer, max_len, "Player %d not found or not connected.\n",
                    target_id);
          return;
        }

      // the 4 statuses are : idle, away, waitgame, ingame. We can also add
      // "connected"
      const char *status_str = "Unknown";
      switch (manager->server_ctx->clients[target_id].status)
        {
        case CLIENT_STATUS_IDLE:
          status_str = "idle";
          break;
        case CLIENT_STATUS_AWAY:
          status_str = "away";
          break;
        case CLIENT_STATUS_WAITGAME:
          status_str = "waitgame";
          break;
        case CLIENT_STATUS_INGAME:
          status_str = "ingame";
          break;
        default:
          status_str = "connected";
          break;
        }

      // detailled format for a specific player, as requested in F40. Note that
      // we don't have real score data, so we put placeholders for
      // wins/losses/matches.
      // TODO: Replace placeholders with actual score data when available
      snprintf (
          buffer, max_len,
          "Details -> ID: %d | Name: %s | Status: %s | Score: %dWon %dLost "
          "(%d played)",
          target_id, manager->server_ctx->clients[target_id].username,
          status_str, manager->server_ctx->clients[target_id].wins,
          manager->server_ctx->clients[target_id].losses,
          manager->server_ctx->clients[target_id].games_played);
      return; // we stop here for the specific player case, the general list is
              // only for "players" without ID
    }

  // =========================================================================
  // case 2 : "players" -> List of all connected players with basic status
  //
  // =========================================================================
  for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
    {
      if (manager->server_ctx->fds[i].fd != INVALID_SOCKET
          && manager->server_ctx->clients[i].is_joined)
        {
          char line[PLAYER_INFO_BUFFER_SIZE];

          const char *simple_status = "idle";
          if (manager->server_ctx->clients[i].status == CLIENT_STATUS_INGAME)
            {
              simple_status = "ingame";
            }

          snprintf (line, sizeof (line), "[ID %d] %s - (%s),", i,
                    manager->server_ctx->clients[i].username, simple_status);

          if (strlen (buffer) + strlen (line) < max_len - 1)
            {
              strcat (buffer, line);
            }
        }
    }

  // little safety check in case no players are connected or all names are too
  // long to fit in the buffer
  if (buffer[0] == '\0')
    {
      snprintf (buffer, max_len, "No players currently in the lobby.\n");
    }
}
