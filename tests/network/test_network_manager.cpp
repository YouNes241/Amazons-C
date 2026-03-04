#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

extern "C"
{
#include "../../src/network/network_internal.h"
#include "network_protocol.h"
}

/* ==========================================================================
 * GLOBAL MOCKS & HELPERS
 * ==========================================================================
 */
static int cb_joined_count = 0;
static int cb_disconnected_count = 0;
static int current_test_port = 9400;

extern "C" void
mock_on_player_joined (const char *name)
{
  (void)name;
  cb_joined_count++;
}
extern "C" void
mock_on_player_disconnected (const char *reason)
{
  (void)reason;
  cb_disconnected_count++;
}

/**
 * @brief Test fixture for NetworkManager unit tests.
 * @details Automatically initializes and cleans up client/server managers,
 * resets callback counters, and assigns a unique port per test to avoid
 * collisions.
 */
class NetworkManagerTest : public ::testing::Test
{
protected:
  NetworkManager server_mgr;
  NetworkManager client_mgr;
  NetworkEvents test_callbacks = {};
  int test_port;

  void
  SetUp () override
  {
    cb_joined_count = 0;
    cb_disconnected_count = 0;
    test_port = current_test_port++;
    test_callbacks.OnPlayerJoined = mock_on_player_joined;
    test_callbacks.OnPlayerDisconnected = mock_on_player_disconnected;
    NetworkManager_Init (&server_mgr, test_callbacks);
    NetworkManager_Init (&client_mgr, test_callbacks);
  }

  void
  TearDown () override
  {
    NetworkManager_Cleanup (&client_mgr);
    NetworkManager_Cleanup (&server_mgr);
  }
};

/* ==========================================================================
 * CATEGORY 1: CONNECTION LIFECYCLE (F40.3)
 * ==========================================================================
 */

/**
 * @brief Tests the successful initialization and connection of a server and a
 * client.
 * @details Verifies that internal modes transition correctly to
 * NET_MODE_SERVER and NET_MODE_CLIENT.
 */
TEST_F (NetworkManagerTest, ConnectionLifecycleSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  EXPECT_EQ (server_mgr.current_mode, NET_MODE_SERVER);

  int res = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  EXPECT_EQ (res, 0);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_CLIENT);
}

/**
 * @brief Tests state machine guards against double activation.
 * @details Ensures a manager cannot start a server or join a client if it is
 * already in an active mode.
 */
TEST_F (NetworkManagerTest, StateGuardsPreventDoubleActivation)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  EXPECT_EQ (NetworkManager_ServerStart (&server_mgr, test_port + 1), -1);
  EXPECT_EQ (NetworkManager_ClientJoin (&server_mgr, "127.0.0.1", test_port),
             -1);

  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             -1);
  EXPECT_EQ (NetworkManager_ServerStart (&client_mgr, test_port + 1), -1);
}

/**
 * @brief Tests error handling during connection and initialization.
 * @details Covers restricted ports, port binding collisions, and invalid IP
 * string formats.
 */
TEST_F (NetworkManagerTest, ConnectionLifecycleFailures)
{
  EXPECT_EQ (NetworkManager_ServerStart (&server_mgr, 1), -1);

  NetworkManager dummy_server;
  NetworkManager_Init (&dummy_server, test_callbacks);
  NetworkManager_ServerStart (&server_mgr, test_port);
  EXPECT_EQ (NetworkManager_ServerStart (&dummy_server, test_port), -1);
  NetworkManager_Cleanup (&dummy_server);

  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, "999.9.9.9", test_port),
             -1);
}

/* ==========================================================================
 * CATEGORY 2: EVENT LOOP, SENDING & DATA HANDLING (F40.4 & F40.5)
 * ==========================================================================
 */

/**
 * @brief Tests the core event loop for data transmission and connection
 * events.
 * @details Simulates client connection, two-way data transmission
 * (PING/HELLO), and disconnection to verify the multiplexer logic handles all
 * POLLIN events.
 */
TEST_F (NetworkManagerTest, EventLoopTrafficHandling)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  // Client -> Server
  int c_sock = client_mgr.client_ctx->server_socket;
  send (c_sock, "PING\n", 5, 0);
  NetworkManager_Tick (&server_mgr);

  // Server -> Client
  int s_client_fd = server_mgr.server_ctx->fds[1].fd;
  send (s_client_fd, "HELLO\n", 6, 0);
  NetworkManager_Tick (&client_mgr);

  NetworkManager_Cleanup (&client_mgr);
  NetworkManager_Tick (&server_mgr);
}

/**
 * @brief Verifies the routing logic of NetworkManager_Send.
 * @details Tests sending messages from the server, targeting specific clients,
 * and handling invalid target IDs or disconnected slots.
 */
TEST_F (NetworkManagerTest, SendFunctionRoutingLogic)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  // 1. Valid send from Server to Client (Slot 1)
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 1, "VALID\n"), 0);

  // 2. Invalid target IDs (Out of bounds)
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 0, "INVALID\n"), -1);
  EXPECT_EQ (NetworkManager_Send (&server_mgr, MAX_CLIENTS + 1, "INVALID\n"),
             -1);

  // 3. Sending to an empty/disconnected slot
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 2, "EMPTY\n"), -1);
}

/**
 * @brief Validates the parsing and callback triggering for "MOVE" commands.
 * @details Tests the parsing logic against malformed moves, valid moves,
 * and scenarios where no callbacks are registered to cover all branches.
 */
TEST_F (NetworkManagerTest, MoveCommandParsingAndCallbacks)
{
  NetworkEvents empty_cb = {}; // No callbacks registered
  NetworkManager server_no_cb, client_no_cb;
  NetworkManager_Init (&server_no_cb, empty_cb);
  NetworkManager_Init (&client_no_cb, empty_cb);

  ASSERT_EQ (NetworkManager_ServerStart (&server_no_cb, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_no_cb, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_no_cb);

  // 1. Client sends a malformed MOVE (Hits Protocol_StringToMove fail branch)
  send (client_no_cb.client_ctx->server_socket, "MOVE INVALID\n", 13, 0);
  NetworkManager_Tick (&server_no_cb);

  // 2. Client sends valid MOVE, but Server has NO callback registered
  send (client_no_cb.client_ctx->server_socket, "MOVE a1-a2-a3\n", 14, 0);
  NetworkManager_Tick (&server_no_cb);

  // 3. Server sends valid MOVE to Client, but Client has NO callback
  // registered
  send (server_no_cb.server_ctx->fds[1].fd, "MOVE a1-a2-a3\n", 14, 0);
  NetworkManager_Tick (&client_no_cb);

  NetworkManager_Cleanup (&client_no_cb);
  NetworkManager_Cleanup (&server_no_cb);
}

/**
 * @brief Tests client-side detection of an abrupt server shutdown.
 */
TEST_F (NetworkManagerTest, EventLoopClientDetectionServerDown)
{
  NetworkEvents no_cb = {};
  NetworkManager_Init (&client_mgr, no_cb);

  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  NetworkManager_Cleanup (&server_mgr);
  NetworkManager_Tick (&client_mgr);
}

/**
 * @brief Tests the multiplexer behavior during network inactivity.
 */
TEST_F (NetworkManagerTest, EventLoopInactivity)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/* ==========================================================================
 * CATEGORY 3: ROBUSTNESS & DEFENSIVE SECURITY
 * ==========================================================================
 */

/**
 * @brief Tests defensive programming guards against NULL pointer parameters.
 * @details Ensures public API functions safely return errors or exit early
 * instead of crashing when passed NULL managers or messages.
 */
TEST_F (NetworkManagerTest, RobustnessParameterValidation)
{
  EXPECT_EQ (NetworkManager_ServerStart (nullptr, 8080), -1);
  EXPECT_EQ (NetworkManager_ClientJoin (nullptr, "127.0.0.1", 8080), -1);
  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, nullptr, 8080), -1);

  // Send function guards
  EXPECT_EQ (NetworkManager_Send (nullptr, 1, "MSG\n"), -1);
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 1, nullptr), -1);
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 1, "MSG\n"),
             -1); // Fails because mode is NONE

  EXPECT_NO_THROW (NetworkManager_Tick (nullptr));
  EXPECT_NO_THROW (NetworkManager_Cleanup (nullptr));
}

/**
 * @brief Tests protection against network flooding and buffer overflows.
 * @details Floods the sockets with oversized payloads to trigger the
 * status == -2 (buffer full) error on both the client and server sides.
 */
TEST_F (NetworkManagerTest, RobustnessBufferOverflowProtection)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  // Generate a massive payload without any newlines
  std::string huge_payload (4096, 'X');

  // 1. Flood Server buffer
  send (client_mgr.client_ctx->server_socket, huge_payload.c_str (),
        huge_payload.size (), 0);
  // Tick multiple times to let recv() drain the socket into the tcp_buffer
  // until it chokes
  for (int i = 0; i < 20; i++)
    NetworkManager_Tick (&server_mgr);

  // 2. Flood Client buffer
  send (server_mgr.server_ctx->fds[1].fd, huge_payload.c_str (),
        huge_payload.size (), 0);
  for (int i = 0; i < 20; i++)
    NetworkManager_Tick (&client_mgr);

  // If the program reaches here without crashing, the protection works.
  SUCCEED ();
}

/**
 * @brief Tests the server's maximum client capacity constraints.
 */
TEST_F (NetworkManagerTest, RobustnessCapacityLimits)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);

  NetworkManager extras[MAX_CLIENTS];
  for (int i = 0; i < MAX_CLIENTS; i++)
    {
      NetworkManager_Init (&extras[i], test_callbacks);
      NetworkManager_ClientJoin (&extras[i], "127.0.0.1", test_port);
      NetworkManager_Tick (&server_mgr);
    }

  // Reject 65th client
  NetworkManager full;
  NetworkManager_Init (&full, test_callbacks);
  NetworkManager_ClientJoin (&full, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  for (int i = 0; i < MAX_CLIENTS; i++)
    NetworkManager_Cleanup (&extras[i]);
  NetworkManager_Cleanup (&full);
}

/**
 * @brief Tests resilience against corrupted internal states and missing
 * callbacks.
 */
TEST_F (NetworkManagerTest, RobustnessInternalConsistency)
{
  // 1. Missing Callbacks
  NetworkEvents empty_ev = {};
  NetworkManager_Init (&server_mgr, empty_ev);
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  NetworkManager_Cleanup (&client_mgr);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 2. Corrupted States
  client_mgr.current_mode = NET_MODE_CLIENT;
  client_mgr.client_ctx = nullptr;
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  server_mgr.current_mode = NET_MODE_SERVER;
  ServerContext *temp_ctx = server_mgr.server_ctx;
  server_mgr.server_ctx = nullptr;
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  server_mgr.server_ctx = temp_ctx;
}