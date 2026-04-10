/**
 * @file network_stats.c
 * @brief Implementation of functions to manage player statistics and the
 * scoreboard for the Amazons game.
 *
 * This file contains functions to save and load player statistics to and from
 * a file, as well as to update the scoreboard after each game. The statistics
 * include wins, losses, and games played for each player, which are stored in
 * a simple text file format.
 */

#include "network_includes/network_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @name Scoreboard and Statistics Management
 * @brief Public API for managing player scores and statistics
 */
///@{

/**
 * @brief Saves the current player statistics to a file named scoreboard.txt
 *
 * The function iterates through the list of connected clients and writes their
 * username, wins, losses, and games played to the file. Each line in the file
 * corresponds to one player and has the format: "username wins losses games".
 *
 * @param manager Pointer to the NetworkManager instance containing the server
 * context with client information.
 */
void
SaveScoreboard (NetworkManager *manager)
{
  if (!manager || !manager->server_ctx)
    return;
  FILE *f = fopen ("scoreboard.txt", "w");
  if (!f)
    return;

  // Could iterate through a global player list here, but for now we save
  // the active connected clients
  for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
    {
      if (manager->server_ctx->clients[i].is_joined)
        {
          fprintf (f, "%s %d %d %d\n",
                   manager->server_ctx->clients[i].username,
                   manager->server_ctx->clients[i].wins,
                   manager->server_ctx->clients[i].losses,
                   manager->server_ctx->clients[i].games_played);
        }
    }
  fclose (f);
}

/**
 * @brief Loads player statistics from the "scoreboard.txt" file for a specific
 * client.
 *
 * The function reads the scoreboard file line by line, looking for a matching
 * username. If found, it populates the client's wins, losses, and games
 * played fields with the corresponding values from the file.
 *
 * @param client Pointer to the ConnectedClient structure for which to load
 * statistics.
 */
void
LoadScoreboardForClient (ConnectedClient *client)
{
  if (!client)
    return;
  FILE *f = fopen ("scoreboard.txt", "r");
  if (!f)
    return;

  char name[MAX_USERNAME_LENGTH];
  int w = 0, l = 0, g = 0;
  while (fscanf (f, "%s %d %d %d", name, &w, &l, &g) == 4)
    {
      if (strcmp (name, client->username) == 0)
        {
          client->wins = w;
          client->losses = l;
          client->games_played = g;
          break;
        }
    }
  fclose (f);
}

/**
 * @brief Updates the player statistics after a game and saves the updated
 * scoreboard.
 *
 * This function should be called at the end of each game to update the wins,
 * losses, and games played for the involved players. It then calls
 * SaveScoreboard to persist the changes to the file.
 *
 * @param username The username of the player whose stats are being updated.
 * @param won True if the player won the game, false if they lost.
 */
void
NetworkManager_UpdatePlayerStats (const char *username, bool won)
{
  if (!username)
    return;
  FILE *f = fopen ("scoreboard.txt", "r");
  // Use a temporary file to rewrite the updated statistics
  FILE *temp = fopen ("scoreboard.tmp", "w");
  if (!temp)
    return;

  bool found = false;
  char name[32] = { 0 };
  int w = 0, l = 0, g = 0;

  if (f)
    {
      while (fscanf (f, "%s %d %d %d", name, &w, &l, &g) == 4)
        {
          if (strcmp (name, username) == 0)
            {
              found = true;
              g++; ///< Increment games played count
              if (won)
                w++;
              else
                l++; ///< Increment wins or losses count
            }
          fprintf (temp, "%s %d %d %d\n", name, w, l, g);
          // we reset w,l,g for the next line to avoid carrying over values if
          // the file is malformed
          w = 0;
          l = 0;
          g = 0;
        }
      fclose (f);
    }

  // If the player did not yet exist in the scoreboard file
  if (!found)
    {
      fprintf (temp, "%s %d %d %d\n", username, won ? 1 : 0, won ? 0 : 1, 1);
    }

  fclose (temp);
  rename ("scoreboard.tmp", "scoreboard.txt");
}
///@}
