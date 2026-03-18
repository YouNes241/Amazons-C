/**
 * @file network_stats.c
 * @brief Gestion de l'enregistrement et du chargement des scores.
 */

#include "network_common.h"
#include "network_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
SaveScoreboard (NetworkManager *manager)
{
  FILE *f = fopen ("scoreboard.txt", "w");
  if (!f)
    return;

  // Ici, on pourrait parcourir une liste globale,
  // mais pour l'instant sauvegardons les clients actifs
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

void
LoadScoreboardForClient (ConnectedClient *client)
{
  FILE *f = fopen ("scoreboard.txt", "r");
  if (!f)
    return;

  char name[MAX_USERNAME_LENGTH];
  int w, l, g;
  while (fscanf (f, "%s %d %d %d", name, &w, &l, &g) != EOF)
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

void
NetworkManager_UpdatePlayerStats (const char *username, bool won)
{
  FILE *f = fopen ("scoreboard.txt", "r");
  // On utilise un fichier temporaire pour réécrire les données
  FILE *temp = fopen ("scoreboard.tmp", "w");
  if (!temp)
    return;

  bool found = false;
  char name[32];
  int w, l, g;

  if (f)
    {
      while (fscanf (f, "%s %d %d %d", name, &w, &l, &g) != EOF)
        {
          if (strcmp (name, username) == 0)
            {
              found = true;
              g++; // +1 partie jouée
              if (won)
                w++;
              else
                l++; // +1 victoire ou défaite
            }
          fprintf (temp, "%s %d %d %d\n", name, w, l, g);
        }
      fclose (f);
    }

  // Si le joueur n'existait pas encore dans le fichier
  if (!found)
    {
      fprintf (temp, "%s %d %d %d\n", username, won ? 1 : 0, won ? 0 : 1, 1);
    }

  fclose (temp);
  rename ("scoreboard.tmp", "scoreboard.txt");
}