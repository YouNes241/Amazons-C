/**
 * @file network_internal.h
 * @brief Définitions internes pour le Network Manager et les Tests Unitaires.
 */

#ifndef NETWORK_INTERNAL_H
#define NETWORK_INTERNAL_H

#include "../../include/network_common.h"
#include <poll.h>

/* ==========================================================================
 * 1. Constants
 * ==========================================================================
 */

#define MAX_CLIENTS 64

/* ==========================================================================
 * 2. Full Structure Definitions
 * ==========================================================================
 */

/**
 * @brief Structure réelle du contexte serveur.
 * Permet aux tests de vérifier num_clients et le tableau fds.
 */
struct ServerContext
{
  int listen_socket;
  // The monitoring table.
  // Index 0 will ALWAYS be the listen_socket (the entry point).
  // Indexes 1 through MAX_CLIENTS will be the connected players.
  struct pollfd fds[MAX_CLIENTS + 1];
  int num_clients;
};

/**
 * @brief Structure réelle du contexte client.
 */
struct ClientContext
{
  int server_socket;
  struct pollfd fd_info; // The client only monitors one thing: the server
};

#endif // NETWORK_INTERNAL_H