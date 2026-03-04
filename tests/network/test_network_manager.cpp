#include <gtest/gtest.h>
#include <unistd.h>

extern "C"
{
#include "../include/network_common.h"
}

/* ==========================================================================
 * GLOBAL VARIABLES
 * ==========================================================================
 */
static int cb_joined_count = 0;
static int cb_disconnected_count = 0;
static int current_test_port = 9090;

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
 * TESTS F40.3 : SOCKET INITIALIZATION
 * ==========================================================================
 */

/**
 * @brief Tests if the server successfully starts and initializes its context
 * on a given port.
 * @details Verifies that the internal mode switches to NET_MODE_SERVER and the
 * context is allocated.
 */
TEST_F (NetworkManagerTest, ServerStartSuccess)
{
  int result = NetworkManager_ServerStart (&server_mgr, test_port);
  EXPECT_EQ (result, 0);
  EXPECT_EQ (server_mgr.current_mode, NET_MODE_SERVER);
  EXPECT_NE (server_mgr.server_ctx, nullptr);
}

/**
 * @brief Tests if a client can successfully connect to an active server on the
 * local loopback.
 * @details Verifies that the server accepts the initial connection and the
 * client context is properly set.
 */
TEST_F (NetworkManagerTest, ClientJoinSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  int result = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  EXPECT_EQ (result, 0);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_CLIENT);
  EXPECT_NE (client_mgr.client_ctx, nullptr);
}

/**
 * @brief Tests the client's failure handling when attempting to connect to a
 * closed port.
 * @details Ensures the client returns an error code (-1) and remains in
 * NET_MODE_NONE.
 */
TEST_F (NetworkManagerTest, ClientJoinFailureOnWrongPort)
{
  int result = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", 9999);
  EXPECT_EQ (result, -1);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_NONE);
}

/* ==========================================================================
 * TESTS F40.4 : MULTIPLEXING & EVENT LOOP
 * ==========================================================================
 */

/**
 * @brief Tests if the server's multiplexing tick correctly detects and accepts
 * an incoming connection.
 * @details Simulates the asynchronous event loop by calling Tick() multiple
 * times to process the TCP handshake.
 */
TEST_F (NetworkManagerTest, ServerTickAcceptsClient)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);

  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&server_mgr);
      if (cb_joined_count > 0)
        break;
      usleep (10000);
    }

  EXPECT_EQ (cb_joined_count, 1);
}

/**
 * @brief Tests if the server's tick correctly detects when an established
 * client disconnects abruptly.
 * @details The client is forced to cleanup, and the server's Tick() is called
 * to catch the POLLIN event reading 0 bytes.
 */
TEST_F (NetworkManagerTest, ServerTickDetectsDisconnection)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);

  // 1. Establish connection
  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&server_mgr);
      if (cb_joined_count > 0)
        break;
      usleep (10000);
    }
  ASSERT_EQ (cb_joined_count, 1);

  // 2. Client abruptly disconnects
  NetworkManager_Cleanup (&client_mgr);

  // 3. Server processes the disconnection
  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&server_mgr);
      if (cb_disconnected_count > 0)
        break;
      usleep (10000);
    }

  EXPECT_EQ (cb_disconnected_count, 1);
}

/**
 * @brief Tests if the client's tick correctly detects when the server shuts
 * down.
 * @details Evaluates the client-side polling mechanism to ensure it triggers
 * the disconnect callback.
 */
TEST_F (NetworkManagerTest, ClientTickDetectsServerShutdown)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);

  // 1. Establish connection (Server accepts client)
  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&server_mgr);
      if (cb_joined_count > 0)
        break;
      usleep (10000);
    }
  ASSERT_EQ (cb_joined_count, 1);

  // 2. Server shuts down abruptly
  NetworkManager_Cleanup (&server_mgr);

  // 3. Client processes the disconnection via its own Tick
  for (int i = 0; i < 20; i++)
    {
      NetworkManager_Tick (&client_mgr); // tick the CLIENT here
      if (cb_disconnected_count > 0)
        break;
      usleep (10000);
    }

  EXPECT_EQ (cb_disconnected_count, 1);
}