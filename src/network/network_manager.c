/**
 * @file network_manager.c
 * @brief Implementation of the NetworkManager, handling TCP Server and Client
 * connections.
 */

#include "network_common.h"
#include "network_handlers.h"
#include "network_internal.h"
#include "network_packet.h"
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

/* ==========================================================================
 * Forward Declarations (Private Helpers)
 * ==========================================================================
 */

static int set_nonblocking (int fd);
static int receive_into_buffer (int sock, tcp_buffer_t *buffer);
static void handle_server_new_connection (ServerContext *ctx);
static void disconnect_server_client (NetworkManager *manager,
                                      ServerContext *ctx, int client_index);
static void process_buffer_packets (NetworkManager *manager,
                                    tcp_buffer_t *buffer, int client_index);
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
  manager->server_ctx = malloc (sizeof (ServerContext));
  if (manager->server_ctx == NULL)
    {
      perror ("Server memory allocation error");
      close (sock);
      return -1;
    }
  manager->current_mode = NET_MODE_SERVER;
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
  manager->client_ctx = malloc (sizeof (ClientContext));
  if (manager->client_ctx == NULL)
    {
      perror ("Client memory allocation error");
      close (sock);
      return -1;
    }
  manager->current_mode = NET_MODE_CLIENT;
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
      printf ("\n[NETWORK] Server down.\n");
    }
  else if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      close (manager->client_ctx->server_socket);
      free (manager->client_ctx);
      manager->client_ctx = NULL;
      printf ("\n[NETWORK] Disconnected from the server.\n");
    }
  manager->current_mode = NET_MODE_NONE;
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
 * @brief Construit et envoie un paquet JOIN proprement.
 */
int
NetworkManager_SendJoin (NetworkManager *manager, const char *username)
{
  if (manager == NULL || username == NULL)
    return -1;

  // 1. On crée le paquet réseau
  Packet pkt;
  pkt.type = C2S_JOIN;
  strncpy (pkt.payload, username, MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  // 2. On le sérialise grâce à ton fichier network_packet.c
  char formatted_buffer[512];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    {
      return -1;
    }

  // 3. On utilise le "tuyau bête" pour envoyer la chaîne finale ("1|Alice\n")
  // Le client envoie toujours au serveur (index 0)
  return NetworkManager_Send (manager, 0, formatted_buffer);
}

/**
 * @brief Convertit un Move en texte, l'encapsule dans un Paquet et l'envoie.
 */
int
NetworkManager_SendMove (NetworkManager *manager, Move m)
{
  if (manager == NULL || manager->current_mode == NET_MODE_NONE)
    return 0; // Si on joue en local, on ne fait rien !

  // 1. Traduire la structure Move en texte (ex: "MOVE e2-e4-e6\n")
  char move_str[128];
  Protocol_MoveToString (m, move_str, sizeof (move_str));

  // Petite astuce : on enlève le '\n' ajouté par Protocol_MoveToString
  // car SerializePacket va déjà en rajouter un !
  move_str[strcspn (move_str, "\n")] = 0;

  // 2. Créer le paquet réseau
  Packet pkt;
  pkt.type = (manager->current_mode == NET_MODE_CLIENT) ? C2S_MOVE : S2C_MOVE;
  strncpy (pkt.payload, move_str, MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  // 3. Sérialiser
  char formatted_buffer[512];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return -1;

  // 4. Envoyer ! (Si on est client, on envoie au serveur (0). Si on est
  // serveur, on envoie au client 1)
  int target = (manager->current_mode == NET_MODE_CLIENT) ? 0 : 1;
  return NetworkManager_Send (manager, target, formatted_buffer);
}

/**
 * @brief Construit et envoie un paquet de TIMEOUT proprement.
 */
int
NetworkManager_SendTimeout (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode == NET_MODE_NONE)
    return -1;

  // 1. Création du paquet réseau
  Packet pkt;
  pkt.type = PKT_TIMEOUT; // On utilise le type 0, le handler cherchera le mot
                          // "TIMEOUT"
  strncpy (pkt.payload, "TIMEOUT", MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  // 2. Sérialisation (ajout du TYPE| et du '\n')
  char formatted_buffer[512];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    {
      return -1;
    }

  // 3. Envoi à l'adversaire (0 pour le serveur si on est client, 1 pour le
  // client 1 si on est serveur)
  int target = (manager->current_mode == NET_MODE_CLIENT) ? 0 : 1;
  return NetworkManager_Send (manager, target, formatted_buffer);
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

  // 1. Determine the target socket according to the mode
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

  // 2. Check if a valid recipient has been found
  if (target_socket == -1)
    {
      printf ("%s Error: Invalid target.\n", log_prefix);
      return -1;
    }

  // 3. One single sending call for everyone!
  if (send (target_socket, message, strlen (message), 0) < 0)
    {
      perror (log_prefix);
      return -1;
    }

  return 0;
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

/**
 * @brief Handles a new incoming connection for the server.
 * @details Accepts the socket, finds a free slot, configures the pollfd array,
 * and initializes the dedicated TCP buffer for the new client.
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

      for (int i = 1; i <= MAX_CLIENTS; i++)
        {
          if (ctx->fds[i].fd == -1)
            {
              ctx->fds[i].fd = client_fd;
              ctx->fds[i].events = POLLIN;
              ctx->clients[i].fd = client_fd;
              tcp_buffer_init (&ctx->clients[i].buffer);
              ctx->clients[i].is_joined = false;
              memset (ctx->clients[i].username, 0, 32);
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

/**
 * @brief Cleans up a specific client slot on the server.
 */
static void
disconnect_server_client (NetworkManager *manager, ServerContext *ctx,
                          int client_index)
{
  printf ("\n[SERVER] Client %d has disconnected.\n", client_index);
  close (ctx->fds[client_index].fd);
  ctx->fds[client_index].fd = -1;
  ctx->num_clients--;
  if (manager->callbacks.OnPlayerDisconnected)
    manager->callbacks.OnPlayerDisconnected ("Network disconnection");
}

/**
 * @brief Common helper to extract and process all complete packets from a
 * buffer.
 */
static void
process_buffer_packets (NetworkManager *manager, tcp_buffer_t *buffer,
                        int client_index)
{
  char extracted_msg[256];
  int res;
  const char *log_prefix
      = (manager->current_mode == NET_MODE_SERVER) ? "[SERVER]" : "[CLIENT]";

  while ((res = tcp_buffer_extract_line (buffer, extracted_msg,
                                         sizeof (extracted_msg)))
         != 0)
    {
      if (res == -1)
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
    return;
  if (status == -1)
    {
      disconnect_server_client (manager, ctx, client_index);
      return;
    }
  if (status == -2)
    {
      printf ("\n[SERVER] Error: Buffer full for client %d.\n", client_index);
      return;
    }

  // Use the unified packet processing loop!
  process_buffer_packets (manager, &ctx->clients[client_index].buffer,
                          client_index);
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

  if (ctx->fds[0].revents & POLLIN)
    {
      handle_server_new_connection (ctx);
    }

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
      printf ("\n[CLIENT] The server has cut the connection.\n");
      NetworkManager_Cleanup (manager);
      if (manager->callbacks.OnPlayerDisconnected)
        manager->callbacks.OnPlayerDisconnected ("Server closed");

      return;
    }

  if (status == -2)
    {
      printf ("\n[CLIENT] Error: Buffer full.\n");
      return;
    }

  // Use the unified packet processing loop! (client_index is 0)
  process_buffer_packets (manager, &ctx->buffer, 0);
}

/**
 * @brief Vérifie si un adversaire est connecté (utilisé pour le timer)
 * @return 1 si oui, 0 si non.
 */
int
NetworkManager_HasOpponent (NetworkManager *manager)
{
  if (manager == NULL)
    return 0;

  if (manager->current_mode == NET_MODE_SERVER && manager->server_ctx)
    {
      return (manager->server_ctx->num_clients > 0);
    }

  if (manager->current_mode == NET_MODE_CLIENT && manager->client_ctx)
    {
      // TODO changer plus tard En mode client, si on est connecté,
      // l'adversaire c'est le serveur
      return 1;
    }

  return 0;
}