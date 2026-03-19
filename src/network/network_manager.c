/**
 * @file network_manager.c
 * @brief Core implementation of the NetworkManager, handling both Server and
 * Client modes, including connection management, multiplexing with poll(), and
 * integration with the discovery mechanism.
 *
 * This file implements the main logic for managing network connections in both
 * Server and Client modes. It includes functions to start a server, join a
 * server as a client, send messages, and perform regular ticks to handle
 * incoming data and timeouts. The implementation uses non-blocking sockets and
 * the poll() system call for efficient multiplexing of multiple connections.
 */

#include "network_includes/network_handlers.h"
#include "network_protocol.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/* Forward Declarations (Private Helpers) */

static int set_nonblocking (int fd);
static int receive_into_buffer (int sock, tcp_buffer_t *buffer);
static void handle_server_new_connection (ServerContext *ctx);
static void handle_server_client_data (NetworkManager *manager,
                                       ServerContext *ctx, int client_index);
static void handle_server_tick (NetworkManager *manager, ServerContext *ctx);
static void handle_client_tick (NetworkManager *manager, ClientContext *ctx);

/* ==========================================================================
 * Public API Implementation
 * ==========================================================================
 */

/**
 * @brief Initializes the NetworkManager structure.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param callbacks Structure containing event callback functions.
 */
void
NetworkManager_Init (NetworkManager *manager, NetworkEvents callbacks)
{
  if (manager == NULL)
    return;
  manager->current_mode = NET_MODE_NONE;
  manager->server_ctx = NULL;
  manager->client_ctx = NULL;
  manager->callbacks = callbacks;

  manager->discovery_ctx = calloc (1, sizeof (struct ClientDiscoveryContext));
  if (manager->discovery_ctx)
    {
      manager->discovery_ctx->udp_listen_socket
          = socket (AF_INET, SOCK_DGRAM, 0);
      if (manager->discovery_ctx->udp_listen_socket >= 0)
        {
          int opt = 1;
          setsockopt (manager->discovery_ctx->udp_listen_socket, SOL_SOCKET,
                      SO_REUSEADDR, &opt, sizeof (opt));

          struct sockaddr_in local_addr;
          memset (&local_addr, 0, sizeof (local_addr));
          local_addr.sin_family = AF_INET;
          local_addr.sin_port = htons (UDP_DISCOVERY_PORT);
          local_addr.sin_addr.s_addr = INADDR_ANY;

          bind (manager->discovery_ctx->udp_listen_socket,
                (struct sockaddr *)&local_addr, sizeof (local_addr));
        }
    }
}

/**
 * @brief Starts the network manager in Server mode.
 *
 * @details This function creates a TCP socket, binds it to the specified port,
 * and starts listening for incoming connections. It also sets the socket to
 * non-blocking mode.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param port The port number to listen on (e.g., 8080).
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 * @note certain error branches (e.g., socket creation or malloc failure) are
 * defensive checks against catastrophic system exhaustion and are not intended
 * to be triggered during normal unit testing
 */
int
NetworkManager_ServerStart (NetworkManager *manager, int port)
{
  if (manager == NULL)
    return NET_ERROR;
  if (manager->current_mode != NET_MODE_NONE || manager->server_ctx != NULL)
    {
      printf ("[NETWORK] Manager is already active!\n");
      return NET_ERROR;
    }
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("Server socket creation error");
      return NET_ERROR;
    }

  // prevents "Address already in use" error.
  int opt = ENABLE_SOCKET_OPTION;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (port);

  if (bind (sock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      close (sock);
      return NET_ERROR;
    }
  // MAX_PENDING_CONNECTIONS = size of the backlog queue for connections in
  // queue.
  if (listen (sock, MAX_PENDING_CONNECTIONS) < 0)
    {
      perror ("Listen error");
      close (sock);
      return NET_ERROR;
    }

  set_nonblocking (sock);

  // initialize Server Context
  manager->server_ctx = malloc (sizeof (ServerContext));
  if (manager->server_ctx == NULL)
    {
      perror ("Server memory allocation error");
      close (sock);
      return NET_ERROR;
    }
  manager->current_mode = NET_MODE_SERVER;
  manager->server_ctx->listen_socket = sock;
  manager->server_ctx->port = port;
  snprintf (manager->server_ctx->server_name, MAX_SERVER_NAME_LEN,
            "Amazons_%d", port);
  manager->server_ctx->num_clients = 0;
  manager->server_ctx->next_game_id = 1;

  // We configure the monitoring of the main door
  manager->server_ctx->fds[LISTEN_SOCKET_INDEX].fd = sock;
  manager->server_ctx->fds[LISTEN_SOCKET_INDEX].events = POLLIN;

  // We put the customer "seats" empty (INVALID_SOCKET means "poll, ignore this
  // box")
  for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
    {
      manager->server_ctx->fds[i].fd = INVALID_SOCKET;
    }

  // =========================================================
  // initialization of the UDP socket for discovery broadcasts (non-blocking,
  // but we won't monitor it with poll() since we only use it for sending)
  // =========================================================
  manager->server_ctx->discovery.udp_socket_fd
      = socket (AF_INET, SOCK_DGRAM, 0);
  if (manager->server_ctx->discovery.udp_socket_fd >= 0)
    {
      int broadcast_enable = 1;
      setsockopt (manager->server_ctx->discovery.udp_socket_fd, SOL_SOCKET,
                  SO_BROADCAST, &broadcast_enable, sizeof (broadcast_enable));

      memset (&manager->server_ctx->discovery.broadcast_addr, 0,
              sizeof (struct sockaddr_in));
      manager->server_ctx->discovery.broadcast_addr.sin_family = AF_INET;
      manager->server_ctx->discovery.broadcast_addr.sin_port
          = htons (UDP_DISCOVERY_PORT);
      manager->server_ctx->discovery.broadcast_addr.sin_addr.s_addr
          = inet_addr ("255.255.255.255");

      manager->server_ctx->discovery.last_broadcast_time = time (NULL);
    }
  else
    {
      printf ("[WARNING] Could not create UDP discovery socket.\n");
    }

  printf ("[NETWORK] Server started on port %d\n", port);
  return NET_SUCCESS;
}

/**
 * @brief Connects the network manager to a server (Client mode).
 *
 * @details Creates a socket, converts the IP address, and attempts to connect.
 * The connection process is blocking, but the socket is set to non-blocking
 * immediately after a successful connection.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param ip The target IP address string (e.g., "127.0.0.1").
 * @param port The target server port.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
int
NetworkManager_ClientJoin (NetworkManager *manager, const char *ip, int port)
{
  if (manager == NULL || ip == NULL)
    return NET_ERROR;
  if (manager->current_mode != NET_MODE_NONE || manager->client_ctx != NULL)
    {
      printf ("[NETWORK] Already connected or active!\n");
      return NET_ERROR;
    }
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("Client socket creation error");
      return NET_ERROR;
    }

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);

  // convert text IP to binary
  if (inet_pton (AF_INET, ip, &addr.sin_addr) <= 0)
    {
      printf ("[NETWORK] Invalid IP : %s\n", ip);
      close (sock);
      return NET_ERROR;
    }

  //  connection to the server
  // this call is blocking until the handshake is complete or fails.
  if (connect (sock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("Server connection error");
      close (sock);
      return NET_ERROR;
    }
  set_nonblocking (sock);

  // initialize Client Context
  manager->client_ctx = malloc (sizeof (ClientContext));
  if (manager->client_ctx == NULL)
    {
      perror ("Client memory allocation error");
      close (sock);
      return NET_ERROR;
    }
  manager->current_mode = NET_MODE_CLIENT;
  manager->client_ctx->server_socket = sock;
  manager->client_ctx->fd_info.fd = sock;
  manager->client_ctx->fd_info.events = POLLIN;
  tcp_buffer_init (&manager->client_ctx->buffer);
  printf ("[NETWORK] Connected to the server %s:%d\n", ip, port);
  return NET_SUCCESS;
}

/**
 * @brief Processes all pending network events without blocking the main
 * thread.
 * * @details Delegates the actual multiplexing logic to internal helper
 * functions depending on the current network mode (SERVER or CLIENT).
 */
void
NetworkManager_Cleanup (NetworkManager *manager)
{
  if (manager == NULL)
    return;

  if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          if (manager->server_ctx->fds[i].fd != INVALID_SOCKET)
            {
              close (manager->server_ctx->fds[i].fd);
            }
        }
      if (manager->server_ctx->discovery.udp_socket_fd >= 0)
        {
          close (manager->server_ctx->discovery.udp_socket_fd);
        }
      close (manager->server_ctx->listen_socket);
      free (manager->server_ctx);
      manager->server_ctx = NULL;
      printf ("\n[NETWORK] Server down.\n");
    }
  else if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      close (manager->client_ctx->server_socket);
      free (manager->client_ctx);
      manager->client_ctx = NULL;
      printf ("\n[NETWORK] Disconnected from the server.\n");
    }

  // --- Le nettoyage du carnet d'adresses doit se faire tout le temps ---
  // Car on l'a alloué dans NetworkManager_Init, peu importe le mode !
  if (manager->discovery_ctx)
    {
      if (manager->discovery_ctx->udp_listen_socket != INVALID_SOCKET)
        {
          close (manager->discovery_ctx->udp_listen_socket);
        }
      free (manager->discovery_ctx);
      manager->discovery_ctx = NULL;
    }

  manager->current_mode = NET_MODE_NONE;
}

/**
 * @brief Regular tick function to process network events and timeouts.
 *
 * This function should be called regularly (e.g., in the main loop) to allow
 * the NetworkManager to handle incoming data, new connections, and perform
 * periodic tasks such as sending discovery broadcasts.
 *
 * @param manager Pointer to the NetworkManager instance.
 */
void
NetworkManager_Tick (NetworkManager *manager)
{
  if (manager == NULL)
    {
      return;
      // ATTENTION: On a enlevé le "|| manager->current_mode == NET_MODE_NONE"
      // Car on VEUT que le tick s'exécute dans le menu pour l'écoute UDP !
    }

  if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      handle_server_tick (manager, manager->server_ctx);
      if (manager->server_ctx->discovery.udp_socket_fd >= 0)
        {
          time_t now = time (NULL);
          if (difftime (now,
                        manager->server_ctx->discovery.last_broadcast_time)
              >= BROADCAST_INTERVAL_SEC)
            {
              char msg[UDP_DISCOVERY_BUFFER_SIZE];
              snprintf (msg, sizeof (msg), "%s;%d",
                        manager->server_ctx->server_name,
                        manager->server_ctx->port);

              sendto (manager->server_ctx->discovery.udp_socket_fd, msg,
                      strlen (msg), 0,
                      (struct sockaddr *)&manager->server_ctx->discovery
                          .broadcast_addr,
                      sizeof (manager->server_ctx->discovery.broadcast_addr));

              manager->server_ctx->discovery.last_broadcast_time = now;
            }
        }
    }
  else if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      handle_client_tick (manager, manager->client_ctx);
    }

  if (manager->discovery_ctx
      && manager->discovery_ctx->udp_listen_socket != INVALID_SOCKET)
    {
      struct pollfd pfd;
      pfd.fd = manager->discovery_ctx->udp_listen_socket;
      pfd.events = POLLIN;

      if (poll (&pfd, 1, 0) > 0)
        {
          char buffer[256];
          struct sockaddr_in sender_addr;
          socklen_t addr_len = sizeof (sender_addr);

          int bytes = recvfrom (manager->discovery_ctx->udp_listen_socket,
                                buffer, sizeof (buffer) - 1, 0,
                                (struct sockaddr *)&sender_addr, &addr_len);

          if (bytes > 0)
            {
              buffer[bytes] = '\0';
              char *port_start = strchr (buffer, ';');

              if (port_start)
                {
                  *port_start = '\0'; // Coupe la string
                  port_start++;       // Avance sur le port

                  int port = atoi (port_start);
                  char *name_start = buffer;
                  char *ip = inet_ntoa (sender_addr.sin_addr);
                  time_t now = time (NULL);

                  bool found = false;

                  for (int i = 0; i < manager->discovery_ctx->server_count;
                       i++)
                    {
                      if (strcmp (manager->discovery_ctx->servers[i].ip, ip)
                              == 0
                          && manager->discovery_ctx->servers[i].port == port)
                        {
                          manager->discovery_ctx->servers[i].last_seen = now;
                          // Petit fix de sécurité : strncpy ne met pas
                          // toujours le \0 à la fin si ça dépasse
                          strncpy (manager->discovery_ctx->servers[i].name,
                                   name_start, MAX_SERVER_NAME_LEN - 1);
                          manager->discovery_ctx->servers[i]
                              .name[MAX_SERVER_NAME_LEN - 1]
                              = '\0';
                          found = true;
                          break;
                        }
                    }

                  if (!found
                      && manager->discovery_ctx->server_count
                             < MAX_DISCOVERED_SERVERS)
                    {
                      int idx = manager->discovery_ctx->server_count;
                      strncpy (manager->discovery_ctx->servers[idx].ip, ip,
                               INET_ADDRSTRLEN - 1);
                      manager->discovery_ctx->servers[idx]
                          .ip[INET_ADDRSTRLEN - 1]
                          = '\0';

                      strncpy (manager->discovery_ctx->servers[idx].name,
                               name_start, MAX_SERVER_NAME_LEN - 1);
                      manager->discovery_ctx->servers[idx]
                          .name[MAX_SERVER_NAME_LEN - 1]
                          = '\0';

                      manager->discovery_ctx->servers[idx].port = port;
                      manager->discovery_ctx->servers[idx].last_seen = now;

                      manager->discovery_ctx->server_count++;
                    }
                }
            }
        }
    }
}

/**
* @brief Sends a text message over the network.
* @param manager Pointer to the manager.
* @param target_client The client index (1 to MAX_CLIENTS) if you are the
server. Ignored (set to 0) if you are the client.
* @param message The message to send (ideally should end with '\n').
* @return Returns 0 for success, -1 for failure.
*/
int
NetworkManager_Send (NetworkManager *manager, int target_client,
                     const char *message)
{
  if (manager == NULL || message == NULL
      || manager->current_mode == NET_MODE_NONE)
    return NET_ERROR;

  int target_socket = INVALID_SOCKET;
  const char *log_prefix = "";

  // 1. Determine the target socket according to the mode
  if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      target_socket = manager->client_ctx->server_socket;
      log_prefix = "[CLIENT]";
    }
  else if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      if (target_client < FIRST_CLIENT_INDEX || target_client > MAX_CLIENTS)
        return NET_ERROR;

      target_socket = manager->server_ctx->fds[target_client].fd;
      log_prefix = "[SERVER]";
    }

  // 2. Check if a valid recipient has been found
  if (target_socket == INVALID_SOCKET)
    {
      printf ("%s Error: Invalid target.\n", log_prefix);
      return NET_ERROR;
    }

  // 3. One single sending call for everyone!
  if (send (target_socket, message, strlen (message), 0) < 0)
    {
      perror (log_prefix);
      return NET_ERROR;
    }

  return NET_SUCCESS;
}

/* ==========================================================================
 * Private Helper Implementations
 * ==========================================================================
 */

/**
 * @brief Sets a file descriptor (socket) to non-blocking mode.
 *
 * @details This is done so that read/write operations on the socket do not
 * halt the program execution if no data is immediately available.
 *
 * @param fd The file descriptor to modify.
 * @return int NET_SUCCESS on success, NET_ERROR on failure.
 */
static int
set_nonblocking (int fd)
{
  int flags = fcntl (fd, F_GETFL, 0);
  if (flags == -1)
    return NET_ERROR;
  if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) == -1)
    return NET_ERROR;

  return NET_SUCCESS;
}

/**
 * @brief Reads data from a socket and injects it into the target TCP buffer.
 * @return NET_DATA_READ (success), NET_NO_DATA (nothing to read/EAGAIN),
 * NET_ERROR (disconnection), NET_BUFFER_FULL (buffer full)
 */
static int
receive_into_buffer (int sock, tcp_buffer_t *buffer)
{
  char temp[RECV_TEMP_BUFFER_SIZE];
  int bytes_received = recv (sock, temp, sizeof (temp) - 1, 0);

  if (bytes_received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    return NET_NO_DATA; // nothing new

  if (bytes_received <= 0)
    return NET_ERROR; // Disconnect

  if (!tcp_buffer_append (buffer, temp, bytes_received))
    return NET_BUFFER_FULL; // critical error : full buffer

  return NET_DATA_READ; // Succes
}
/**
 * @brief Handles a new incoming connection on the server's listening socket.
 *
 * This function accepts the new connection, sets it to non-blocking mode, and
 * adds it to the server context's monitoring table. If the server is at full
 * capacity, the connection is refused and closed immediately.
 *
 * @param ctx Pointer to the ServerContext structure.
 */
static void
handle_server_new_connection (ServerContext *ctx)
{
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof (client_addr);
  int client_fd = accept (ctx->listen_socket, (struct sockaddr *)&client_addr,
                          &addr_len);

  if (client_fd >= 0)
    {
      set_nonblocking (client_fd);
      bool accepted = false;

      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          if (ctx->fds[i].fd == INVALID_SOCKET)
            {
              ctx->fds[i].fd = client_fd;
              ctx->fds[i].events = POLLIN;
              ctx->clients[i].fd = client_fd;
              tcp_buffer_init (&ctx->clients[i].buffer);
              ctx->clients[i].is_joined = false;
              memset (ctx->clients[i].username, 0, MAX_USERNAME_LENGTH);
              ctx->clients[i].game_id = NO_GAME_ID;
              ctx->clients[i].status = CLIENT_STATUS_CONNECTED;
              ctx->clients[i].last_activity = time (NULL);
              ctx->num_clients++;
              accepted = true;

              printf ("\n[SERVER] TCP Connection established (Slot %d) - "
                      "Waiting for JOIN packet...\n",
                      i);

              break;
            }
        }

      if (!accepted)
        {
          printf ("\n[SERVER] Full! Connection refused.\n");
          close (client_fd);
        }
    }
}

/** @brief Handles incoming data or disconnection from a specific client.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 * @param client_index Index of the client associated with the data.
 */
static void
handle_server_client_data (NetworkManager *manager, ServerContext *ctx,
                           int client_index)
{
  int status = receive_into_buffer (ctx->fds[client_index].fd,
                                    &ctx->clients[client_index].buffer);

  if (status == NET_NO_DATA)
    return;
  if (status == NET_ERROR)
    {
      disconnect_server_client (manager, ctx, client_index);
      return;
    }
  ctx->clients[client_index].last_activity = time (NULL);
  if (status == NET_BUFFER_FULL)
    {
      printf ("\n[SERVER] Error: Buffer full for client %d.\n", client_index);
      return;
    }

  // Use the unified packet processing loop!
  process_buffer_packets (manager, &ctx->clients[client_index].buffer,
                          client_index);
}

/** @brief Handles the server's periodic tick events.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ServerContext structure.
 */
static void
handle_server_tick (NetworkManager *manager, ServerContext *ctx)
{
  int poll_count = poll (ctx->fds, MAX_CLIENTS + 1, POLL_TIMEOUT_IMMEDIATE);
  time_t now = time (NULL);

  if (poll_count > 0)
    {
      if (ctx->fds[LISTEN_SOCKET_INDEX].revents & POLLIN)
        {
          handle_server_new_connection (ctx);
        }

      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          if (ctx->fds[i].fd != INVALID_SOCKET
              && (ctx->fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
            {
              handle_server_client_data (manager, ctx, i);
            }
        }
    }
  for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
    {
      if (ctx->fds[i].fd != INVALID_SOCKET)
        {
          if (difftime (now, ctx->clients[i].last_activity)
              >= CLIENT_TIMEOUT_SEC)
            {
              printf (
                  "\n[SERVER] Client %d timed out (no activity for %d sec).\n",
                  i, CLIENT_TIMEOUT_SEC);

              Packet pkt;
              pkt.type = S2C_ERROR;
              snprintf (pkt.payload, MAX_PAYLOAD_SIZE,
                        "Kicked for inactivity (%d sec).", CLIENT_TIMEOUT_SEC);

              char out_buf[MAX_FORMATTED_PKT_SIZE];
              if (SerializePacket (&pkt, out_buf, sizeof (out_buf)) >= 0)
                {
                  NetworkManager_Send (manager, i, out_buf);
                }
              disconnect_server_client (manager, ctx, i);
            }
        }
    }
}

/** @brief Handles the client's periodic tick events.
 * @param manager Pointer to the NetworkManager instance.
 * @param ctx Pointer to the ClientContext structure.
 */
static void
handle_client_tick (NetworkManager *manager, ClientContext *ctx)
{
  int poll_count
      = poll (&ctx->fd_info, SINGLE_FD_COUNT, POLL_TIMEOUT_IMMEDIATE);
  if (poll_count <= 0
      || !(ctx->fd_info.revents & (POLLIN | POLLHUP | POLLERR)))
    return;

  int status = receive_into_buffer (ctx->server_socket, &ctx->buffer);

  if (status == NET_ERROR)
    {
      printf ("\n[CLIENT] The server has cut the connection.\n");
      NetworkManager_Cleanup (manager);
      if (manager->callbacks.OnPlayerDisconnected)
        manager->callbacks.OnPlayerDisconnected ("");

      return;
    }

  if (status == NET_BUFFER_FULL)
    {
      printf ("\n[CLIENT] Error: Buffer full.\n");
      return;
    }

  // Use the unified packet processing loop! (client_index is 0)
  process_buffer_packets (manager, &ctx->buffer, 0);
}

/**
 * @brief Shuts down the server gracefully by notifying all connected clients
 * before cleaning up resources.
 * @details This function sends a final message to all clients informing them
 * of the shutdown, then proceeds to close all client connections and the
 * listening socket, and finally frees the server context. Clients should
 * receive the shutdown message and handle it appropriately (e.g., by showing a
 * notification and returning to the main menu) before the connection is
 * terminated.
 * @param manager The network manager instance to shut down.
 */
void
NetworkManager_ShutdownServer (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode != NET_MODE_SERVER
      || manager->server_ctx == NULL)
    return;

  // 1. we create a shutdown packet to inform clients about the server shutdown
  Packet pkt;
  pkt.type = S2C_ERROR;
  strncpy (pkt.payload, "Server is shutting down.", MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char out_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, out_buffer, sizeof (out_buffer)) >= 0)
    {
      // 2. we send the shutdown message to all connected clients before
      // cutting the
      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          if (manager->server_ctx->fds[i].fd != INVALID_SOCKET)
            {
              NetworkManager_Send (manager, i, out_buffer);
            }
        }
    }

  // 3. now we can safely clean up all resources, knowing that clients have
  // been informed and have had a chance to react to the shutdown message.
  NetworkManager_Cleanup (manager);
}
