/**
 * @file network_manager.c
 * @brief Implementation of the NetworkManager, handling TCP Server and Client
 * connections.
 */

#include "network_common.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Internal Structures

/**
 * @brief Internal context structure for Server mode.
 * @details Has data specific to the server, like the listening socket.
 */
struct ServerContext
{
  int listen_socket; // listen for incoming connections.

  // TODO: Add the table of connected clients (e.g., ClientList) here.
};

/**
 * @brief Internal context structure for Client mode.
 * @details Holds data specific to the client, such as the connection to the
 * server.
 */
struct ClientContext
{
  int server_socket; // connected to the remote server.
};

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
 */
int
NetworkManager_ServerStart (NetworkManager *manager, int port)
{
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
  if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
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