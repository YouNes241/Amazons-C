#include "network_common.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* ==========================================================================
 * Definition of internal structures (Hidden from the rest of the program)
 * ==========================================================================
 */

struct ServerContext
{
  int listen_socket; // The socket that listens to newcomers
  // TODO add the table of connected clients in F40.4
};

struct ClientContext
{
  int server_socket; // The socket connected to the server
};

/* ==========================================================================
 * Private utility functions
 * ==========================================================================
 */

// Function to make a socket non-blocking
static int
set_nonblocking (int fd)
{
  int flags = fcntl (fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl (fd, F_SETFL, flags | O_NONBLOCK);
}

/* ==========================================================================
 * Implementation
 * ==========================================================================
 */

void
NetworkManager_Init (NetworkManager *manager, NetworkEvents callbacks)
{
  manager->current_mode = NET_MODE_NONE;
  manager->server_ctx = NULL;
  manager->client_ctx = NULL;
  manager->callbacks = callbacks;
}

int
NetworkManager_ServerStart (NetworkManager *manager, int port)
{
  // 1. Creating the TCP socket (IPv4, Stream)
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("Server socket creation error");
      return -1;
    }

  // 2. Option SO_REUSEADDR (Prevents the "Address already in use" error if the
  // server is restarted quickly)

  int opt = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

  // 3. Address configuration (Listening on all local interfaces)
  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (port);

  // 4. Bind the socket to the port
  if (bind (sock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("bind error");
      close (sock);
      return -1;
    }

  // 5. Put the socket in listening mode.
  if (listen (sock, 10) < 0)
    { // 10 is the queue size
      perror ("listen error");
      close (sock);
      return -1;
    }

  // 6. Make the socket non-blocking
  set_nonblocking (sock);

  // 7. Initialize the Server Context
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

int
NetworkManager_ClientJoin (NetworkManager *manager, const char *ip, int port)
{
  // 1. Creating the TCP socket
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("Client socket creation error");
      return -1;
    }

  // 2. Target address configuration
  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);

  // Converting the text IP address ("127.0.0.1") into binary format
  if (inet_pton (AF_INET, ip, &addr.sin_addr) <= 0)
    {
      printf ("[NETWORK] Invalid IP : %s\n", ip);
      close (sock);
      return -1;
    }

  // 3. Connecting to the server (blocking only during the connection process)
  if (connect (sock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("Server connection error");
      close (sock);
      return -1;
    }

  // 4. Make the socket non-blocking for the remainder of the game
  set_nonblocking (sock);

  // 5. Initialize the Client Context
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

void
NetworkManager_Cleanup (NetworkManager *manager)
{
  // A small piece of F43.1 for clean testing without memory leaks
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