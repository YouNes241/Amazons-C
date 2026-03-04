/**
 * @file network_manager.c
 * @brief Implementation of the NetworkManager, handling TCP Server and Client
 * connections.
 */

#include "network_common.h"
#include "network_internal.h"
#include "network_protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Private Utility Functions

/**
 * @brief Sets a file descriptor (socket) to non-blocking mode.
 *
 * @details This is done so that read/write operations on the socket do not
 * halt the program execution if no data is immediately available.
 *
 * @param fd The file descriptor to modify.
 * @return int 0 on success, -1 on failure.
 */
static int
set_nonblocking (int fd)
{
  int flags = fcntl (fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl (fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * @brief Reads data from a socket and injects it into the target TCP buffer.
 * @return 1 (success), 0 (nothing to read/EAGAIN), -1 (disconnection), -2
 * (buffer full)
 */
static int
receive_into_buffer (int sock, tcp_buffer_t *buffer)
{
  char temp[256];
  int bytes_received = recv (sock, temp, sizeof (temp) - 1, 0);

  if (bytes_received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    return 0; // nothing new

  if (bytes_received <= 0)
    return -1; // Disconnect

  if (!tcp_buffer_append (buffer, temp, bytes_received))
    return -2; // critical error : full buffer

  return 1; // Succes
}

// Public Implementation

/**
 * @brief Initializes the NetworkManager structure.
 *
 * @param manager Pointer to the NetworkManager instance.
 * @param callbacks Structure containing event callback functions.
 */
void
NetworkManager_Init (NetworkManager *manager, NetworkEvents callbacks)
{
  manager->current_mode = NET_MODE_NONE;
  manager->server_ctx = NULL;
  manager->client_ctx = NULL;
  manager->callbacks = callbacks;
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
 * @return int 0 on success, -1 on failure.
 * @note certain error branches (e.g., socket creation or malloc failure) are
 * defensive checks against catastrophic system exhaustion and are not intended
 * to be triggered during normal unit testing
 */
int
NetworkManager_ServerStart (NetworkManager *manager, int port)
{
  if (manager == NULL)
    return -1;
  if (manager->current_mode != NET_MODE_NONE || manager->server_ctx != NULL)
    {
      printf ("[NETWORK] Manager is already active!\n");
      return -1;
    }
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("Server socket creation error");
      return -1;
    }

  // prevents "Address already in use" error.
  int opt = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (port);

  if (bind (sock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("Bind error");
      close (sock);
      return -1;
    }
  // 10 = size of the backlog queue for connections in queue.
  if (listen (sock, 10) < 0)
    {
      perror ("Listen error");
      close (sock);
      return -1;
    }

  set_nonblocking (sock);

  // initialize Server Context
  manager->current_mode = NET_MODE_SERVER;
  manager->server_ctx = malloc (sizeof (ServerContext));
  if (manager->server_ctx == NULL)
    {
      perror ("Server memory allocation error");
      close (sock);
      return -1;
    }
  manager->server_ctx->listen_socket = sock;
  manager->server_ctx->num_clients = 0;

  // We configure the monitoring of the main door
  manager->server_ctx->fds[0].fd = sock;
  manager->server_ctx->fds[0].events = POLLIN;

  // We put the customer "seats" empty (-1 means "poll, ignore this box")
  for (int i = 1; i <= MAX_CLIENTS; i++)
    {
      manager->server_ctx->fds[i].fd = -1;
    }

  printf ("[NETWORK] Server started on port %d\n", port);
  return 0;
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
 * @return int 0 on success, -1 on failure.
 */
int
NetworkManager_ClientJoin (NetworkManager *manager, const char *ip, int port)
{
  if (manager == NULL || ip == NULL)
    return -1;
  if (manager->current_mode != NET_MODE_NONE || manager->client_ctx != NULL)
    {
      printf ("[NETWORK] Already connected or active!\n");
      return -1;
    }
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("Client socket creation error");
      return -1;
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
      return -1;
    }

  //  connection to the server
  // this call is blocking until the handshake is complete or fails.
  if (connect (sock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("Server connection error");
      close (sock);
      return -1;
    }
  set_nonblocking (sock);

  // initialize Client Context
  manager->current_mode = NET_MODE_CLIENT;
  manager->client_ctx = malloc (sizeof (ClientContext));
  if (manager->client_ctx == NULL)
    {
      perror ("Client memory allocation error");
      close (sock);
      return -1;
    }
  manager->client_ctx->server_socket = sock;

  manager->client_ctx->fd_info.fd = sock;
  manager->client_ctx->fd_info.events = POLLIN;
  tcp_buffer_init (&manager->client_ctx->buffer);
  printf ("[NETWORK] Connected to the server %s:%d\n", ip, port);
  return 0;
}

/**
 * @brief Cleans up network resources and frees memory.
 *
 * @details Closes the listening socket (if server) or the connection socket
 * (if client), frees the allocated context structures, and resets the mode to
 * NET_MODE_NONE.
 *
 * @param manager Pointer to the NetworkManager instance.
 */
void
NetworkManager_Cleanup (NetworkManager *manager)
{
  if (manager == NULL)
    return;
  if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      for (int i = 1; i <= MAX_CLIENTS; i++)
        {
          if (manager->server_ctx->fds[i].fd != -1)
            {
              close (manager->server_ctx->fds[i].fd);
            }
        }
      close (manager->server_ctx->listen_socket);
      free (manager->server_ctx);
      manager->server_ctx = NULL;
      printf ("[NETWORK] Server down.\n");
    }
  else if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      close (manager->client_ctx->server_socket);
      free (manager->client_ctx);
      manager->client_ctx = NULL;
      printf ("[NETWORK] Disconnected from the server.\n");
    }
  manager->current_mode = NET_MODE_NONE;
}
// ==========================================================================
// Private Helper Functions for Tick
// ==========================================================================

/**
 * @brief Handles a new incoming connection for the server.
 * @details Accepts the socket, finds a free slot, configures the pollfd array,
 * and initializes the dedicated TCP buffer for the new client.
 */
static void
handle_server_new_connection (NetworkManager *manager, ServerContext *ctx)
{
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof (client_addr);
  int client_fd = accept (ctx->listen_socket, (struct sockaddr *)&client_addr,
                          &addr_len);

  if (client_fd >= 0)
    {
      set_nonblocking (client_fd);
      bool accepted = false;

      for (int i = 1; i <= MAX_CLIENTS; i++)
        {
          if (ctx->fds[i].fd == -1)
            {
              ctx->fds[i].fd = client_fd;
              ctx->fds[i].events = POLLIN;
              ctx->clients[i].fd = client_fd;
              tcp_buffer_init (&ctx->clients[i].buffer);
              ctx->num_clients++;
              accepted = true;

              printf ("[SERVER] New client connected (Slot %d)\n", i);
              if (manager->callbacks.OnPlayerJoined)
                {
                  manager->callbacks.OnPlayerJoined (
                      "Unknown"); // TODO change later
                }
              break;
            }
        }

      if (!accepted)
        {
          printf ("[SERVER] Full! Connection refused.\n");
          close (client_fd);
        }
    }
}

/**
 * @brief Handles incoming data or disconnection from a specific client.
 */
static void
handle_server_client_data (NetworkManager *manager, ServerContext *ctx,
                           int client_index)
{
  int status = receive_into_buffer (ctx->fds[client_index].fd,
                                    &ctx->clients[client_index].buffer);

  if (status == 0)
    return; // nothing to read

  if (status == -1)
    {
      printf ("[SERVER] Client %d has disconnected.\n", client_index);
      close (ctx->fds[client_index].fd);
      ctx->fds[client_index].fd = -1;
      ctx->num_clients--;

      if (manager->callbacks.OnPlayerDisconnected)
        manager->callbacks.OnPlayerDisconnected ("Network disconnection");
      return;
    }

  if (status == -2)
    {
      printf ("[SERVER] Error: Buffer full for client %d.\n", client_index);
      return;
    }

  char extracted_msg[256];
  while (tcp_buffer_extract_line (&ctx->clients[client_index].buffer,
                                  extracted_msg, sizeof (extracted_msg)))
    {
      printf ("[SERVER] Client command received %d : '%s'\n", client_index,
              extracted_msg);
      Move parsed_move;
      if (strncmp (extracted_msg, "MOVE", 4) == 0)
        {
          if (Protocol_StringToMove (extracted_msg, &parsed_move))
            {
              if (manager->callbacks.OnMoveReceived)
                {
                  manager->callbacks.OnMoveReceived (parsed_move);
                }
            }
        }
    }
}

/**
 * @brief Handles the server's multiplexing logic.
 */
static void
handle_server_tick (NetworkManager *manager, ServerContext *ctx)
{
  int poll_count = poll (ctx->fds, MAX_CLIENTS + 1, 0);
  if (poll_count <= 0)
    return;

  // 1. Is there a new connection at the door?
  if (ctx->fds[0].revents & POLLIN)
    {
      handle_server_new_connection (manager, ctx);
    }

  // 2. Is there data from existing clients?
  for (int i = 1; i <= MAX_CLIENTS; i++)
    {
      if (ctx->fds[i].fd != -1
          && (ctx->fds[i].revents & (POLLIN | POLLHUP | POLLERR)))
        {
          handle_server_client_data (manager, ctx, i);
        }
    }
}

/**
 * @brief Handles the client's multiplexing logic.
 */
static void
handle_client_tick (NetworkManager *manager, ClientContext *ctx)
{
  int poll_count = poll (&ctx->fd_info, 1, 0);
  if (poll_count <= 0
      || !(ctx->fd_info.revents & (POLLIN | POLLHUP | POLLERR)))
    return;

  int status = receive_into_buffer (ctx->server_socket, &ctx->buffer);

  if (status == -1)
    {
      printf ("[CLIENT] The server has cut the connection.\n");
      if (manager->callbacks.OnPlayerDisconnected)
        manager->callbacks.OnPlayerDisconnected ("Server closed");
      return;
    }

  if (status == -2)
    {
      printf ("[CLIENT] Error: Buffer full.\n");
      return;
    }

  char extracted_msg[256];
  while (tcp_buffer_extract_line (&ctx->buffer, extracted_msg,
                                  sizeof (extracted_msg)))
    {
      printf ("[CLIENT] Server command received : '%s'\n", extracted_msg);
      Move parsed_move;
      if (strncmp (extracted_msg, "MOVE", 4) == 0)
        {
          if (Protocol_StringToMove (extracted_msg, &parsed_move))
            {
              if (manager->callbacks.OnMoveReceived)
                {
                  manager->callbacks.OnMoveReceived (parsed_move);
                }
            }
        }
    }
}

/**
 * @brief Processes all pending network events without blocking the main
 * thread.
 * * @details Delegates the actual multiplexing logic to internal helper
 * functions depending on the current network mode (SERVER or CLIENT).
 */
void
NetworkManager_Tick (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode == NET_MODE_NONE)
    {
      return;
    }

  if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      handle_server_tick (manager, manager->server_ctx);
    }
  else if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      handle_client_tick (manager, manager->client_ctx);
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
    return -1;

  int target_socket = -1;
  const char *log_prefix = "";

  // 1. Déterminer le socket cible selon le mode
  if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      target_socket = manager->client_ctx->server_socket;
      log_prefix = "[CLIENT]";
    }
  else if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      if (target_client < 1 || target_client > MAX_CLIENTS)
        return -1;

      target_socket = manager->server_ctx->fds[target_client].fd;
      log_prefix = "[SERVER]";
    }

  // 2. Vérifier si on a trouvé un destinataire valide
  if (target_socket == -1)
    {
      printf ("%s Error: Invalid target.\n", log_prefix);
      return -1;
    }

  // 3. Un seul appel d'envoi pour tout le monde !
  if (send (target_socket, message, strlen (message), 0) < 0)
    {
      perror (log_prefix);
      return -1;
    }

  return 0;
}