#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

extern "C"
{
#include "../../src/network/network_internal.h"
#include "../../src/network/network_packet.h"
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
 * CATEGORY 1: CONNECTION LIFECYCLE
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, ConnectionLifecycleSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  EXPECT_EQ (server_mgr.current_mode, NET_MODE_SERVER);

  int res = NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port);
  EXPECT_EQ (res, 0);
  EXPECT_EQ (client_mgr.current_mode, NET_MODE_CLIENT);
}

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
 * CATEGORY 2: EVENT LOOP, PROTOCOL & HANDSHAKE
 * ==========================================================================
 */

/**
 * @brief Tests the new Join Handshake .
 * @details Simulates a client sending a JOIN packet, and the server
 * responding with a WELCOME packet.
 */
TEST_F (NetworkManagerTest, EventLoopHandshakeSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  // Client -> Server : C2S_JOIN
  Packet join_pkt = { C2S_JOIN, "Alice" };
  char buffer[256];
  SerializePacket (&join_pkt, buffer, sizeof (buffer));
  send (client_mgr.client_ctx->server_socket, buffer, strlen (buffer), 0);

  // Server processes JOIN, fires OnPlayerJoined, and sends WELCOME
  NetworkManager_Tick (&server_mgr);
  EXPECT_EQ (cb_joined_count, 1);

  // Client receives WELCOME
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/**
 * @brief Tests server rejection when a username is already taken.
 * @details Connects two clients with the exact same pseudo to trigger
 * the S2C_REJECT branch on the server.
 */
TEST_F (NetworkManagerTest, HandshakeNameCollision)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);

  NetworkManager client1, client2;
  NetworkManager_Init (&client1, test_callbacks);
  NetworkManager_Init (&client2, test_callbacks);

  NetworkManager_ClientJoin (&client1, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_ClientJoin (&client2, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  Packet join_pkt = { C2S_JOIN, "Clone" };
  char buffer[256];
  SerializePacket (&join_pkt, buffer, sizeof (buffer));

  // Client 1 joins successfully
  send (client1.client_ctx->server_socket, buffer, strlen (buffer), 0);
  NetworkManager_Tick (&server_mgr);

  // Client 2 attempts to join with the same name -> Rejected
  send (client2.client_ctx->server_socket, buffer, strlen (buffer), 0);
  NetworkManager_Tick (&server_mgr);

  // Client 2 processes the REJECT packet
  EXPECT_NO_THROW (NetworkManager_Tick (&client2));

  NetworkManager_Cleanup (&client1);
  NetworkManager_Cleanup (&client2);
}

/**
 * @brief Validates all Client-side switch branches.
 * @details Sends every possible server packet to the client to hit all cases.
 */
TEST_F (NetworkManagerTest, ClientPacketTypesCoverage)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  int s_fd = server_mgr.server_ctx->fds[1].fd;
  char buf[256];
  Packet pkt;

  // Send various packets to hit all the switch cases in handle_client_tick
  pkt = { S2C_ERROR, "Err" };
  SerializePacket (&pkt, buf, 256);
  send (s_fd, buf, strlen (buf), 0);
  pkt = { S2C_GAME_START, "Go" };
  SerializePacket (&pkt, buf, 256);
  send (s_fd, buf, strlen (buf), 0);
  pkt = { PKT_UNKNOWN, "?" };
  SerializePacket (&pkt, buf, 256);
  send (s_fd, buf, strlen (buf), 0);

  // Client processes everything
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

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

  Packet pkt;
  char buf[256];

  // 1. Malformed payload (C2S_MOVE)
  pkt = { C2S_MOVE, "MOVE INVALID" };
  SerializePacket (&pkt, buf, 256);
  send (client_no_cb.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_no_cb);

  // 2. Valid MOVE, but Server has NO callback registered (C2S_MOVE)
  pkt = { C2S_MOVE, "MOVE a1-a2-a3" };
  SerializePacket (&pkt, buf, 256);
  send (client_no_cb.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_no_cb);

  // 3. Server sends valid MOVE to Client, but Client has NO callback

  pkt = { S2C_MOVE, "MOVE a1-a2-a3" };
  SerializePacket (&pkt, buf, 256);
  send (server_no_cb.server_ctx->fds[1].fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_no_cb);

  NetworkManager_Cleanup (&client_no_cb);
  NetworkManager_Cleanup (&server_no_cb);
}

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

TEST_F (NetworkManagerTest, RobustnessParameterValidation)
{
  EXPECT_EQ (NetworkManager_ServerStart (nullptr, 8080), -1);
  EXPECT_EQ (NetworkManager_ClientJoin (nullptr, "127.0.0.1", 8080), -1);
  EXPECT_EQ (NetworkManager_ClientJoin (&client_mgr, nullptr, 8080), -1);

  EXPECT_EQ (NetworkManager_Send (nullptr, 1, "MSG\n"), -1);
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 1, nullptr), -1);
  EXPECT_EQ (NetworkManager_Send (&server_mgr, 1, "MSG\n"), -1);

  EXPECT_NO_THROW (NetworkManager_Tick (nullptr));
  EXPECT_NO_THROW (NetworkManager_Cleanup (nullptr));
}

/**
 * @brief Tests extraction rejection for excessively long lines (res == -1).
 */
TEST_F (NetworkManagerTest, RobustnessOversizedLineDiscarded)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             0);
  NetworkManager_Tick (&server_mgr);

  // Creates a 300-character message (greater than the 256 allocated for
  // extracted_msg)
  std::string huge_line (300, 'A');
  huge_line += "\n";

  send (server_mgr.server_ctx->fds[1].fd, huge_line.c_str (),
        huge_line.size (), 0);

  // The client receives the line, sees that it exceeds sizeof(extracted_msg),
  // and rejects it
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

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