/**
 * @file network_manager_test.cpp
 * @brief Unit tests for the Network Manager, covering lifecycle, routing, and
 * robustness.
 */

#include "network_test_fixture.h"
#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

/* ==========================================================================
 * CATEGORY 1: LIFECYCLE & CONNECTION MANAGEMENT
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, ConnectionLifecycleSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  EXPECT_EQ (server_mgr.current_mode, NET_MODE_SERVER);
  sleep (1); // Give the server a moment to start up
  int res = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  EXPECT_EQ (res, NET_SUCCESS);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_CLIENT);
}

TEST_F (NetworkManagerTest, StateGuardsPreventDoubleActivation)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  EXPECT_EQ (NetworkManager_ServerStart (&server_mgr, test_port + 1),
             NET_ERROR);
  EXPECT_EQ (NetworkManager_ClientJoin (&server_mgr, "127.0.0.1", test_port),
             NET_ERROR);

  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_ERROR);
  EXPECT_EQ (NetworkManager_ServerStart (&client_mgr, test_port + 1),
             NET_ERROR);
}

TEST_F (NetworkManagerTest, ConnectionLifecycleFailures)
{

  NetworkManager dummy_server;
  NetworkManager_Init (&dummy_server, test_callbacks);
  NetworkManager_ServerStart (&server_mgr, test_port);
  EXPECT_EQ (NetworkManager_ServerStart (&dummy_server, test_port),
             NET_ERROR); // Port already in use
  NetworkManager_Cleanup (&dummy_server);

  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, "999.9.9.9", test_port),
             NET_ERROR); // Invalid IP
}

/* ==========================================================================
 * CATEGORY 7: ROBUSTNESS & SECURITY
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, RobustnessParameterValidation)
{
  EXPECT_EQ (NetworkManager_ServerStart (nullptr, 8080), NET_ERROR);
  EXPECT_EQ (NetworkManager_ClientJoin (nullptr, "127.0.0.1", 8080),
             NET_ERROR);
  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, nullptr, 8080),
             NET_ERROR);

  EXPECT_EQ (NetworkManager_Send (nullptr, 1, "MSG\n"), NET_ERROR);
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 1, nullptr), NET_ERROR);
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 1, "MSG\n"), NET_ERROR);

  EXPECT_NO_THROW (NetworkManager_Tick (nullptr));
  EXPECT_NO_THROW (NetworkManager_Cleanup (nullptr));
}

TEST_F (NetworkManagerTest, RobustnessOversizedLineDiscarded)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  std::string huge_line (300, 'A');
  huge_line += "\n";

  send (server_mgr.server_ctx->fds[1].fd, huge_line.c_str (),
        huge_line.size (), 0);

  // The client rejects the line exceeding extraction buffer size
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

TEST_F (NetworkManagerTest, RobustnessBufferOverflowProtection)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  std::string huge_payload (4096, 'X');

  // 1. Flood Server buffer
  send (client_mgr.client_ctx->server_socket, huge_payload.c_str (),
        huge_payload.size (), 0);
  for (int i = 0; i < 20; i++)
    NetworkManager_Tick (&server_mgr);

  // 2. Flood Client buffer
  send (server_mgr.server_ctx->fds[1].fd, huge_payload.c_str (),
        huge_payload.size (), 0);
  for (int i = 0; i < 20; i++)
    NetworkManager_Tick (&client_mgr);

  SUCCEED ();
}

TEST_F (NetworkManagerTest, RobustnessCapacityLimits)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager extras[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; i++)
    {
      NetworkManager_Init (&extras[i], test_callbacks);
      NetworkManager_ClientJoin (&extras[i], "127.0.0.1", test_port);
      NetworkManager_Tick (&server_mgr);
    }

  // Reject the 65th client
  NetworkManager full;
  NetworkManager_Init (&full, test_callbacks);
  NetworkManager_ClientJoin (&full, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  for (int i = 0; i < MAX_CLIENTS; i++)
    NetworkManager_Cleanup (&extras[i]);
  NetworkManager_Cleanup (&full);
}

TEST_F (NetworkManagerTest, RobustnessInternalConsistency)
{
  NetworkManager_Cleanup (&server_mgr);
  // Missing Callbacks
  NetworkEvents empty_ev = {};
  NetworkManager_Init (&server_mgr, empty_ev);
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  NetworkManager_Cleanup (&client_mgr);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // Corrupted States
  client_mgr.current_mode = NET_MODE_CLIENT;
  client_mgr.client_ctx = nullptr;
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  server_mgr.current_mode = NET_MODE_SERVER;
  ServerContext *temp_ctx = server_mgr.server_ctx;
  server_mgr.server_ctx = nullptr;
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  server_mgr.server_ctx = temp_ctx;
  NetworkManager_Cleanup (&server_mgr);
  NetworkManager_Cleanup (&client_mgr);
}

/* ==========================================================================
 * CATEGORY 8: SERVER SHUTDOWN ROBUSTNESS
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, ShutdownServerRobustness)
{
  // 1. Guard clauses (Not made for ways)
  EXPECT_NO_THROW (NetworkManager_ShutdownServer (nullptr));

  NetworkManager none_mgr;
  NetworkManager_Init (&none_mgr, test_callbacks);
  EXPECT_NO_THROW (NetworkManager_ShutdownServer (&none_mgr));

  // A client cannot shut down the server
  NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  EXPECT_NO_THROW (NetworkManager_ShutdownServer (&client_mgr));

  // 2. Happy path: shutdown a real server with a client connected
  NetworkManager_ServerStart (&server_mgr, test_port);
  NetworkManager_Tick (&server_mgr);

  // Verify that memory is cleaned up without crashing
  EXPECT_NO_THROW (NetworkManager_ShutdownServer (&server_mgr));
  EXPECT_EQ (server_mgr.current_mode, NET_MODE_NONE);
  EXPECT_EQ (server_mgr.server_ctx, nullptr);
  NetworkManager_Cleanup (&none_mgr);
}

/* ==========================================================================
 * CATEGORY 9: TIMEOUTS & DISCONNECTIONS
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, ServerDropsInactiveClient)
{
  NetworkManager_ServerStart (&server_mgr, test_port);
  NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  // We're cheating: manually modify the last activity time of the
  // client. We simulate no activity for 100 seconds (which exceeds
  // CLIENT_TIMEOUT_SEC)
  server_mgr.server_ctx->clients[1].last_activity = time (NULL) - 100;

  // On the next Tick, the server should detect and kick them!
  NetworkManager_Tick (&server_mgr);

  // Slot 1 should be free again
  EXPECT_EQ (server_mgr.server_ctx->fds[1].fd, INVALID_SOCKET);
}

TEST_F (NetworkManagerTest, ClientDetectsServerCrash)
{
  NetworkManager_ServerStart (&server_mgr, test_port);
  NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_Tick (&client_mgr); // Le client lit que tout va bien

  // On simule un crash du serveur : on ferme violemment son socket
  close (server_mgr.server_ctx->fds[1].fd);
  server_mgr.server_ctx->fds[1].fd = INVALID_SOCKET;

  // The client does a Tick and should receive NET_ERROR because the pipe is
  // broken
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  // The client manager should have cleaned itself up
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_NONE);
}

/* ==========================================================================
 * CATEGORY 10: UDP DISCOVERY PARSING & INJECTION
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, UDPDiscoveryParsingRobustness)
{
  // The client initializes its UDP listener

  int udp_sock = socket (AF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in dest;
  dest.sin_family = AF_INET;
  dest.sin_port = htons (UDP_DISCOVERY_PORT);
  inet_pton (AF_INET, "127.0.0.1", &dest.sin_addr);

  // 1. Valid format ("ServerName;Port")
  const char *valid_msg = "TestUDP;9999";
  sendto (udp_sock, valid_msg, strlen (valid_msg), 0, (struct sockaddr *)&dest,
          sizeof (dest));
  usleep (10000); // Laisse le temps au paquet UDP d'arriver (10ms)
  NetworkManager_Tick (&client_mgr);

  ASSERT_NE (client_mgr.discovery_ctx, nullptr);
  EXPECT_EQ (client_mgr.discovery_ctx->server_count, 1);
  EXPECT_EQ (client_mgr.discovery_ctx->servers[0].port, 9999);

  // 2. Format invalide (pas de point-virgule)
  const char *no_semi = "NoSemicolonHere";
  sendto (udp_sock, no_semi, strlen (no_semi), 0, (struct sockaddr *)&dest,
          sizeof (dest));
  usleep (10000);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr)); // Ne doit pas crasher
  EXPECT_EQ (client_mgr.discovery_ctx->server_count,
             1); // Le compteur ne doit pas monter

  // 3. Buffer Overflow Attack (Nom beaucoup trop long)
  const char *huge_name = "SuperLongServerNameThatExceedsTheThirtyTwoCharacter"
                          "LimitByAWholeLot;8080";
  sendto (udp_sock, huge_name, strlen (huge_name), 0, (struct sockaddr *)&dest,
          sizeof (dest));
  usleep (10000);
  EXPECT_NO_THROW (NetworkManager_Tick (
      &client_mgr)); // Must truncate properly thanks to strncpy

  close (udp_sock);
}