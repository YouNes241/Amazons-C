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
#include <sys/time.h>
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
  if (manager == NULL)
    return;
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
      perror ("Bind error");
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
      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          if (manager->server_ctx->fds[i].fd != INVALID_SOCKET)
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
 * @brief Constructs and sends a JOIN packet cleanly.
 */
int
NetworkManager_SendJoin (NetworkManager *manager, const char *username)
{
  if (manager == NULL || username == NULL)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_JOIN;
  strncpy (pkt.payload, username, MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    {
      return NET_ERROR;
    }

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Converts a Move to text, encapsulates it in a Packet and sends it.
 */
int
NetworkManager_SendMove (NetworkManager *manager, Move m)
{
  if (manager == NULL || manager->current_mode == NET_MODE_NONE)
    return NET_SUCCESS;

  char move_str[MAX_MOVE_STR_SIZE];
  Protocol_MoveToString (m, move_str, sizeof (move_str));
  move_str[strcspn (move_str, "\n")] = 0;

  Packet pkt;
  pkt.type = (manager->current_mode == NET_MODE_CLIENT) ? C2S_MOVE : S2C_MOVE;
  strncpy (pkt.payload, move_str, MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  int target = (manager->current_mode == NET_MODE_CLIENT)
                   ? TARGET_SERVER
                   : TARGET_CLIENT_1; // TODO
  return NetworkManager_Send (manager, target, formatted_buffer);
}

/**
 * @brief Constructs and sends a TIMEOUT packet cleanly.
 */
int
NetworkManager_SendTimeout (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode == NET_MODE_NONE)
    return NET_ERROR;

  Packet pkt;
  pkt.type = PKT_TIMEOUT;
  strncpy (pkt.payload, "TIMEOUT", MAX_PAYLOAD_SIZE - 1);
  pkt.payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    {
      return NET_ERROR;
    }

  int target = (manager->current_mode == NET_MODE_CLIENT)
                   ? TARGET_SERVER
                   : TARGET_CLIENT_1; // TODO
  return NetworkManager_Send (manager, target, formatted_buffer);
}

/**
 * @brief Sends an invitation to a specific player ID.
 */
int
NetworkManager_SendInvite (NetworkManager *manager, int target_id,
                           int board_size)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_INVITE;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d %d", target_id, board_size);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Accepts an invitation from a specific player ID.
 */
int
NetworkManager_SendAccept (NetworkManager *manager, int challenger_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_ACCEPT;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", challenger_id);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Declines an invitation from a specific player ID.
 */
int
NetworkManager_SendDecline (NetworkManager *manager, int challenger_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_DECLINE;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", challenger_id);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Requests the list of connected players from the server.
 */
int
NetworkManager_SendListRequest (NetworkManager *manager, int target_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_LIST;

  if (target_id >= 0)
    snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", target_id);
  else
    pkt.payload[0] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

int
NetworkManager_SendScoreboardRequest (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_SCOREBOARD;
  pkt.payload[0] = '\0';

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

/**
 * @brief Sends a simple network packet (without a payload) to the server.
 * Used for AWAY, BACK, CANCEL, PING, etc.
 */
int
NetworkManager_SendSimplePacket (NetworkManager *manager, PacketType type)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = type;
  pkt.payload[0] = '\0'; // No payload for simple packets

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

int
NetworkManager_SendAway (NetworkManager *manager)
{
  return NetworkManager_SendSimplePacket (manager, C2S_AWAY);
}

int
NetworkManager_SendBack (NetworkManager *manager)
{
  return NetworkManager_SendSimplePacket (manager, C2S_BACK);
}

int
NetworkManager_SendCancel (NetworkManager *manager, int target_id)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  Packet pkt;
  pkt.type = C2S_CANCEL;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%d", target_id);

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

int
NetworkManager_SendPing (NetworkManager *manager)
{
  if (manager == NULL || manager->current_mode != NET_MODE_CLIENT)
    return NET_ERROR;

  // On récupère le temps actuel en millisecondes
  struct timeval tv;
  gettimeofday (&tv, NULL);
  long long now_ms = (long long)tv.tv_sec * 1000 + (tv.tv_usec / 1000);

  Packet pkt;
  pkt.type = C2S_PING;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, "%lld",
            now_ms); // On glisse l'heure dans le message

  char formatted_buffer[MAX_FORMATTED_PKT_SIZE];
  if (SerializePacket (&pkt, formatted_buffer, sizeof (formatted_buffer)) < 0)
    return NET_ERROR;

  return NetworkManager_Send (manager, TARGET_SERVER, formatted_buffer);
}

int
NetworkManager_SendGameOver (NetworkManager *manager, bool won)
{
  Packet pkt;
  pkt.type = C2S_GAME_OVER;
  snprintf (pkt.payload, MAX_PAYLOAD_SIZE, won ? "WIN" : "LOSE");

  char out_buffer[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&pkt, out_buffer, sizeof (out_buffer));
  return NetworkManager_Send (manager, TARGET_SERVER, out_buffer);
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

  int game_id = ctx->clients[client_index].game_id;
  if (game_id != NO_GAME_ID)
    {
      for (int i = FIRST_CLIENT_INDEX; i <= MAX_CLIENTS; i++)
        {
          // if there is still another player in the game
          if (i != client_index && ctx->fds[i].fd != INVALID_SOCKET
              && ctx->clients[i].game_id == game_id)
            {
              // we send him back to the lobby
              ctx->clients[i].status = CLIENT_STATUS_IDLE;
              ctx->clients[i].game_id = NO_GAME_ID;

              Packet pkt;
              pkt.type = S2C_OPPONENT_LEFT;
              strncpy (pkt.payload, "Opponent disconnected.",
                       MAX_PAYLOAD_SIZE - 1);
              char out_buf[MAX_FORMATTED_PKT_SIZE];
              if (SerializePacket (&pkt, out_buf, sizeof (out_buf)) >= 0)
                {
                  NetworkManager_Send (manager, i, out_buf);
                }
            }
        }
    }

  close (ctx->fds[client_index].fd);
  ctx->fds[client_index].fd = INVALID_SOCKET;
  ctx->clients[client_index].is_joined = false;
  ctx->clients[client_index].status = CLIENT_STATUS_CONNECTED;
  ctx->clients[client_index].game_id = NO_GAME_ID;
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
  char extracted_msg[RECV_TEMP_BUFFER_SIZE];
  int res;
  const char *log_prefix
      = (manager->current_mode == NET_MODE_SERVER) ? "[SERVER]" : "[CLIENT]";

  while ((res = tcp_buffer_extract_line (buffer, extracted_msg,
                                         sizeof (extracted_msg)))
         != EXTRACT_NO_MSG)
    {
      if (res == EXTRACT_MSG_OVERFLOW)
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

  if (status == NET_NO_DATA)
    return;
  if (status == NET_ERROR)
    {
      disconnect_server_client (manager, ctx, client_index);
      return;
    }
  if (status == NET_BUFFER_FULL)
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
  int poll_count = poll (ctx->fds, MAX_CLIENTS + 1, POLL_TIMEOUT_IMMEDIATE);
  if (poll_count <= 0)
    return;

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

/**
 * @brief Handles the client's multiplexing logic.
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
        manager->callbacks.OnPlayerDisconnected ("Server closed");

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
 * @brief Vérifie si un adversaire est connecté (utilisé pour le timer)
 * @return 1 si oui, 0 si non.
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
  // case 1 : "players <id>" -> Detailed info about a specific player (F40)
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
      snprintf (buffer, max_len,
                "--- Player Details ---\n"
                "ID     : %d\n"
                "Name   : %s\n"
                "Status : %s\n"
                "Score  : 0 Wins, 0 Losses\n"
                "Matches: 0 Played\n"
                "----------------------\n",
                target_id, manager->server_ctx->clients[target_id].username,
                status_str);
      return; // we stop here for the specific player case, the general list is
              // only for "players" without ID
    }

  // =========================================================================
  // case 2 : "players" -> List of all connected players with basic status
  // (F39)
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

          snprintf (line, sizeof (line), "[ID %d] %s - %s\n", i,
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
