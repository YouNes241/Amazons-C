#include "network_test_fixture.h"

/* ==========================================================================
 * CATEGORY 6: API HELPERS & STATE CHECKS
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, APIHelperFunctionsCoverage)
{
  // 1. HasOpponent Coverage
  EXPECT_EQ (NetworkManager_HasOpponent (nullptr), NO_OPPONENT);
  EXPECT_EQ (NetworkManager_HasOpponent (&server_mgr), NO_OPPONENT);

  NetworkManager_ServerStart (&server_mgr, test_port);
  EXPECT_EQ (NetworkManager_HasOpponent (&server_mgr), NO_OPPONENT);

  NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  EXPECT_EQ (NetworkManager_HasOpponent (&server_mgr), HAS_OPPONENT);
  EXPECT_EQ (NetworkManager_HasOpponent (&client_mgr), HAS_OPPONENT);
}

/* ==========================================================================
 * CATEGORY: NORMAL BEHAVIOR (Happy Path)
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, GetServerStatus_Success)
{
  int clients, games, port;
  NetworkManager_ServerStart (&server_mgr, test_port);

  // initial status check (0 clients, 0 games, correct port)
  EXPECT_EQ (
      NetworkManager_GetServerStatus (&server_mgr, &clients, &games, &port),
      NET_SUCCESS);
  EXPECT_EQ (clients, 0);
  EXPECT_EQ (games, 0);
  EXPECT_EQ (port, test_port);

  // simulation of the status of the server after 2 clients have joined and
  // started a game
  server_mgr.server_ctx->num_clients = 2;
  server_mgr.server_ctx->fds[1].fd = 100; // Simule un socket valide
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->fds[2].fd = 101;
  server_mgr.server_ctx->clients[2].status
      = CLIENT_STATUS_INGAME; // 2 players InGame = 1 game

  EXPECT_EQ (
      NetworkManager_GetServerStatus (&server_mgr, &clients, &games, nullptr),
      NET_SUCCESS);
  EXPECT_EQ (clients, 2);
  EXPECT_EQ (games, 1);
}

TEST_F (NetworkManagerTest, GetServerPlayerList_Success)
{
  NetworkManager_ServerStart (&server_mgr, test_port);
  char buffer[512];

  // Test 1 no players in lobby
  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer),
                                      -1);
  EXPECT_NE (std::string (buffer).find ("No players currently in the lobby"),
             std::string::npos);

  // we simulate 2 clients connected to the server with different statuses
  server_mgr.server_ctx->fds[1].fd = 100;
  server_mgr.server_ctx->clients[1].is_joined = true;
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_IDLE;
  strncpy (server_mgr.server_ctx->clients[1].username, "Alice", 31);

  server_mgr.server_ctx->fds[2].fd = 101;
  server_mgr.server_ctx->clients[2].is_joined = true;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  strncpy (server_mgr.server_ctx->clients[2].username, "Bob", 31);

  // Test 2: Liste complète avec 2 joueurs
  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer),
                                      -1);
  EXPECT_NE (std::string (buffer).find ("[ID 1] Alice - (idle)"),
             std::string::npos);
  EXPECT_NE (std::string (buffer).find ("[ID 2] Bob - (ingame)"),
             std::string::npos);

  // Test 3: Infos détaillées sur le joueur 1 (IDLE)
  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer), 1);
  EXPECT_NE (std::string (buffer).find ("Name: Alice | Status: idle"),
             std::string::npos);

  // Test 4: Différents statuts pour couvrir le switch-case
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_AWAY;
  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer), 1);
  EXPECT_NE (std::string (buffer).find ("Status: away"), std::string::npos);

  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_WAITGAME;
  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer), 1);
  EXPECT_NE (std::string (buffer).find ("Status: waitgame"),
             std::string::npos);

  server_mgr.server_ctx->clients[1].status = (ClientStatus)99; // Cas "default"
  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer), 1);
  EXPECT_NE (std::string (buffer).find ("Status: connected"),
             std::string::npos);
}

TEST_F (NetworkManagerTest, GetServerList_Success)
{
  char buffer[512];

  // we simulate a discovered server in the client's discovery context
  client_mgr.discovery_ctx = (struct ClientDiscoveryContext *)calloc (
      1, sizeof (struct ClientDiscoveryContext));
  client_mgr.discovery_ctx->server_count = 1;
  client_mgr.discovery_ctx->servers[0].port = 1234;
  client_mgr.discovery_ctx->servers[0].last_seen = time (NULL);
  strncpy (client_mgr.discovery_ctx->servers[0].name, "TestServer", 31);
  strncpy (client_mgr.discovery_ctx->servers[0].ip, "192.168.1.50", 15);

  int count
      = NetworkManager_GetServerList (&client_mgr, buffer, sizeof (buffer));
  EXPECT_EQ (count, 1);
  EXPECT_NE (std::string (buffer).find (
                 "[1] TestServer (IP: 192.168.1.50, Port: 1234)"),
             std::string::npos);

  // Test: Un serveur "fantôme" qui a timeout (vu il y a 60 secondes)
  client_mgr.discovery_ctx->servers[0].last_seen = time (NULL) - 60;
  count = NetworkManager_GetServerList (&client_mgr, buffer, sizeof (buffer));
  EXPECT_EQ (count, 0);
  EXPECT_NE (std::string (buffer).find ("No servers found"),
             std::string::npos);
}

/* ==========================================================================
 * CATEGORY: ROBUSTNESS & EDGE CASES (Not made for ways)
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, GetServerStatus_Robustness)
{
  int clients, games, port;
  NetworkManager_ServerStart (&server_mgr, test_port);
  NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);

  // 1. Null pointer for the manager
  EXPECT_EQ (NetworkManager_GetServerStatus (nullptr, &clients, &games, &port),
             NET_ERROR);

  // 2. A client cannot request server status (incorrect mode)
  EXPECT_EQ (
      NetworkManager_GetServerStatus (&client_mgr, &clients, &games, &port),
      NET_ERROR);

  // 3. Manager not initialized / no context
  NetworkManager none_mgr;
  NetworkManager_Init (&none_mgr, test_callbacks);
  EXPECT_EQ (
      NetworkManager_GetServerStatus (&none_mgr, &clients, &games, &port),
      NET_ERROR);

  // 4. Passing NULL pointers as output should not cause a crash.
  EXPECT_EQ (
      NetworkManager_GetServerStatus (&server_mgr, nullptr, nullptr, nullptr),
      NET_SUCCESS);
}

TEST_F (NetworkManagerTest, GetServerPlayerList_Robustness)
{
  char buffer[512];
  NetworkManager_ServerStart (&server_mgr, test_port);
  NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);

  // 1. Basic guard clauses (nullptrs, buffers of size 0)
  EXPECT_NO_THROW (NetworkManager_GetServerPlayerList (nullptr, buffer,
                                                       sizeof (buffer), -1));
  EXPECT_NO_THROW (NetworkManager_GetServerPlayerList (&server_mgr, nullptr,
                                                       sizeof (buffer), -1));
  EXPECT_NO_THROW (
      NetworkManager_GetServerPlayerList (&server_mgr, buffer, 0, -1));

  // 2. A Client (or an inactive manager) cannot use this function
  NetworkManager_GetServerPlayerList (&client_mgr, buffer, sizeof (buffer),
                                      -1);
  EXPECT_EQ (buffer[0], '\0'); // The buffer must remain empty

  // 3. Requesting information for a non-existent player / Out-of-bounds ID
  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer),
                                      999);
  EXPECT_NE (std::string (buffer).find ("Player 999 not found"),
             std::string::npos);

  NetworkManager_GetServerPlayerList (&server_mgr, buffer, sizeof (buffer),
                                      -5);
  EXPECT_NE (std::string (buffer).find ("Player -5 not found"),
             std::string::npos);

  // 4. Test with a very small buffer (must stop cleanly without buffer
  // overflow)We simulate a player
  server_mgr.server_ctx->fds[1].fd = 100;
  server_mgr.server_ctx->clients[1].is_joined = true;
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_IDLE;
  strncpy (server_mgr.server_ctx->clients[1].username, "LongNamePlayer", 31);

  char tiny_buffer[10];
  EXPECT_NO_THROW (NetworkManager_GetServerPlayerList (
      &server_mgr, tiny_buffer, sizeof (tiny_buffer), -1));
  // tiny_buffer couldn't concatenate the entire string, but it won't crash.
  // not.
}

TEST_F (NetworkManagerTest, GetServerList_Robustness)
{
  char buffer[512];

  // 1. Nullptrs
  EXPECT_EQ (NetworkManager_GetServerList (nullptr, buffer, sizeof (buffer)),
             -1);
  EXPECT_EQ (
      NetworkManager_GetServerList (&client_mgr, nullptr, sizeof (buffer)),
      -1);

  // Buffer size 0
  EXPECT_EQ (NetworkManager_GetServerList (&client_mgr, buffer, 0), -1);

  // 2. Manager without a discovery context (simulated)
  NetworkManager no_discovery_mgr;
  NetworkManager_Init (&no_discovery_mgr, test_callbacks);
  free (no_discovery_mgr.discovery_ctx);
  no_discovery_mgr.discovery_ctx = nullptr;
  EXPECT_EQ (NetworkManager_GetServerList (&no_discovery_mgr, buffer,
                                           sizeof (buffer)),
             -1);

  // 3. Buffer Overflow protection with very long server names
  client_mgr.discovery_ctx = (struct ClientDiscoveryContext *)calloc (
      1, sizeof (struct ClientDiscoveryContext));
  client_mgr.discovery_ctx->server_count = 1;
  client_mgr.discovery_ctx->servers[0].last_seen = time (NULL);
  // An abusive name that exceeds the buffer size when formatted
  memset (client_mgr.discovery_ctx->servers[0].name, 'A',
          MAX_SERVER_NAME_LEN - 1);
  client_mgr.discovery_ctx->servers[0].name[MAX_SERVER_NAME_LEN - 1] = '\0';

  char tiny_buffer[20];
  EXPECT_NO_THROW (NetworkManager_GetServerList (&client_mgr, tiny_buffer,
                                                 sizeof (tiny_buffer)));
}