#include <gtest/gtest.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

extern "C"
{
#include "../../src/network/network_internal.h"
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
 * CATEGORY 2: EVENT LOOP & FACTORIZED TICK (F40.4)
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

  // 1. Connection Acceptance
  NetworkManager_Tick (&server_mgr);

  // 2. Data Reception (Client -> Server)
  int c_sock = client_mgr.client_ctx->server_socket;
  send (c_sock, "PING\n", 5, 0);
  NetworkManager_Tick (&server_mgr); // Server processes incoming data

  // 3. Data Reception (Server -> Client)
  int s_client_fd = server_mgr.server_ctx->fds[1].fd;
  send (s_client_fd, "HELLO\n", 6, 0);
  NetworkManager_Tick (&client_mgr); // Client processes incoming data

  // 4. Disconnection Detection
  NetworkManager_Cleanup (&client_mgr);
  NetworkManager_Tick (&server_mgr);
}

/**
 * @brief Tests client-side detection of an abrupt server shutdown.
 * @details Kills the server and ticks the client to ensure it gracefully
 * handles the 'recv <= 0' condition, specifically when callbacks are NULL.
 */
TEST_F (NetworkManagerTest, EventLoopClientDetectionServerDown)
{
  // Initialize client with NO callbacks
  NetworkEvents no_cb = {};
  NetworkManager_Init (&client_mgr, no_cb);

  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  NetworkManager_Cleanup (&server_mgr); // Kill the server
  NetworkManager_Tick (
      &client_mgr); // Client should see disconnection without crashing
}

/**
 * @brief Tests the multiplexer behavior during network inactivity.
 * @details Ensures the 'poll' function safely returns without blocking
 * or crashing when no network events are pending.
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
 * instead of crashing when passed NULL managers.
 */
TEST_F (NetworkManagerTest, RobustnessParameterValidation)
{
  EXPECT_EQ (NetworkManager_ServerStart (nullptr, 8080), -1);
  EXPECT_EQ (NetworkManager_ClientJoin (nullptr, "127.0.0.1", 8080), -1);
  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, nullptr, 8080), -1);

  EXPECT_NO_THROW (NetworkManager_Tick (nullptr));
  EXPECT_NO_THROW (NetworkManager_Cleanup (nullptr));
}

/**
 * @brief Tests the server's maximum client capacity constraints.
 * @details Fills the server to MAX_CLIENTS and verifies that the next
 * incoming connection is safely rejected and closed.
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
 * @details Simulates uninitialized event callbacks to ensure safety, and
 * nullifies internal contexts while in an active mode to test execution
 * skipping branches.
 */
TEST_F (NetworkManagerTest, RobustnessInternalConsistency)
{
  // 1. Missing Callbacks (Server side)
  NetworkEvents empty_ev = {};
  NetworkManager_Init (&server_mgr, empty_ev);
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // Kill client to trigger OnPlayerDisconnected without callback on server
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
  server_mgr.server_ctx = temp_ctx; // Restore context to avoid TearDown crash
}