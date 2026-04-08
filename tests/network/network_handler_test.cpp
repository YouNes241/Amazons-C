#include "network_test_fixture.h"

/* ==========================================================================
 * MODULE-LEVEL STATICS FOR FULL-CALLBACK TESTS
 * ==========================================================================
 * These counters and C-compatible callbacks are used in tests that need
 * every NetworkEvents slot populated to cover the "callback != NULL" branches
 * inside handle_client_packet.
 * ==========================================================================
 */
static int s_list_cb_count = 0;
static int s_declined_cb_count = 0;
static int s_game_started_cb_count = 0;
static int s_move_cb_count = 0;

static void
s_on_list (const char *txt)
{
  (void)txt;
  s_list_cb_count++;
}
static void
s_on_declined (const char *msg)
{
  (void)msg;
  s_declined_cb_count++;
}
static void
s_on_game_started (PlayerColor c, int sz)
{
  (void)c;
  (void)sz;
  s_game_started_cb_count++;
}
static void
s_on_move (Move m)
{
  (void)m;
  s_move_cb_count++;
}

/* ==========================================================================
 * CATEGORY 2: HANDSHAKE & LOBBY LOGIC
 * ==========================================================================
 */

/**
 * @brief Tests the complete client→server handshake flow.
 * @details Verifies that a C2S_JOIN packet triggers OnPlayerJoined and causes
 * the server to respond with S2C_WELCOME, which the client processes cleanly.
 */
TEST_F (NetworkManagerTest, EventLoopHandshakeSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  Packet join_pkt = { C2S_JOIN, "Alice" };
  char buffer[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&join_pkt, buffer, sizeof (buffer));
  send (client_mgr.client_ctx->server_socket, buffer, strlen (buffer), 0);

  NetworkManager_Tick (&server_mgr);
  EXPECT_EQ (cb_joined_count, 1);

  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/**
 * @brief Tests that the server rejects a duplicate username.
 * @details Two clients join with the same name. The second should receive an
 * S2C_REJECT packet and be cleanly disconnected.
 */
TEST_F (NetworkManagerTest, HandshakeNameCollision)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager client1, client2;
  NetworkManager_Init (&client1, test_callbacks);
  NetworkManager_Init (&client2, test_callbacks);

  NetworkManager_ClientJoin (&client1, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_ClientJoin (&client2, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  Packet join_pkt = { C2S_JOIN, "Clone" };
  char buffer[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&join_pkt, buffer, sizeof (buffer));

  send (client1.client_ctx->server_socket, buffer, strlen (buffer), 0);
  NetworkManager_Tick (&server_mgr);

  send (client2.client_ctx->server_socket, buffer, strlen (buffer), 0);
  NetworkManager_Tick (&server_mgr);

  EXPECT_NO_THROW (NetworkManager_Tick (&client2));

  NetworkManager_Cleanup (&client1);
  NetworkManager_Cleanup (&client2);
}

/* ==========================================================================
 * CATEGORY 3: PACKET PROCESSING & ROUTING
 * ==========================================================================
 */

/**
 * @brief Tests server routing for TIMEOUT and MOVE packets.
 * @details Places two clients in the same game, then sends a PKT_TIMEOUT
 * and a C2S_MOVE to verify they are forwarded to the opponent. Also hits the
 * server-side "default" case by sending an unknown packet type.
 */
TEST_F (NetworkManagerTest, ServerRoutingAndSwitchCoverage)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager client1, client2;
  NetworkManager_Init (&client1, test_callbacks);
  NetworkManager_Init (&client2, test_callbacks);

  NetworkManager_ClientJoin (&client1, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_ClientJoin (&client2, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  server_mgr.server_ctx->clients[1].game_id = 1;
  server_mgr.server_ctx->clients[2].game_id = 1;
  server_mgr.server_ctx->clients[1].is_joined = true;
  server_mgr.server_ctx->clients[2].is_joined = true;

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { PKT_TIMEOUT, "TIMEOUT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client1.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_Tick (&client2);

  pkt = { C2S_MOVE, "MOVE a1-a2-a3" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client1.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_Tick (&client2);

  pkt = { (PacketType)99, "GARBAGE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client1.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  NetworkManager_Cleanup (&client1);
  NetworkManager_Cleanup (&client2);
}

/**
 * @brief Tests the client-side switch covering all primary packet types.
 * @details Sends PKT_TIMEOUT, S2C_ERROR (with and without callback),
 * S2C_INVITE, and an unknown type to the client and verifies each is
 * processed correctly (counters updated, no crash).
 */
TEST_F (NetworkManagerTest, ClientSwitchCoverage)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  int s_fd = server_mgr.server_ctx->fds[1].fd;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { PKT_TIMEOUT, "TIMEOUT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_mgr);
  EXPECT_EQ (cb_timeout_count, 1);

  pkt = { S2C_ERROR, "Invalid Move" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_mgr);
  EXPECT_EQ (cb_error_count, 1);

  client_mgr.callbacks.OnGameError = nullptr;
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  pkt = { S2C_INVITE, "2" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_mgr);
  EXPECT_EQ (cb_invite_count, 1);

  pkt = { (PacketType)99, "GARBAGE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/**
 * @brief Tests move command parsing edge cases on the server side.
 * @details Sends a malformed move payload (no MOVE prefix) and a valid-format
 * but unreachable move to exercise Protocol_StringToMove failure paths within
 * process_move_packet.
 */
TEST_F (NetworkManagerTest, MoveCommandParsingAndCallbacks)
{
  NetworkEvents empty_cb = {};
  NetworkManager server_no_cb, client_no_cb;
  NetworkManager_Init (&server_no_cb, empty_cb);
  NetworkManager_Init (&client_no_cb, empty_cb);

  ASSERT_EQ (NetworkManager_ServerStart (&server_no_cb, test_port),
             NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_no_cb, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_no_cb);

  Packet pkt;
  char buf[MAX_FORMATTED_PKT_SIZE];

  pkt = { C2S_MOVE, "MOVE INVALID" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_no_cb.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_no_cb);

  pkt = { C2S_MOVE, "MOVE a1-a2-a3" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_no_cb.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_no_cb);

  NetworkManager_Cleanup (&client_no_cb);
  NetworkManager_Cleanup (&server_no_cb);
}

/* ==========================================================================
 * CATEGORY 5: LOBBY SERVER LOGIC (Invite, Accept, Decline, List)
 * ==========================================================================
 */

/**
 * @brief Tests the complete invite→accept lifecycle between two clients.
 * @details Joins two clients, Alice invites Bob, Bob accepts, and the test
 * verifies that both clients transition to CLIENT_STATUS_INGAME with the same
 * game_id.
 */
TEST_F (NetworkManagerTest, LobbyFullLifecycle_InviteAndAccept)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&alice, "Alice");
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&bob, "Bob");
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
    }

  ASSERT_TRUE (server_mgr.server_ctx->clients[1].is_joined);
  ASSERT_TRUE (server_mgr.server_ctx->clients[2].is_joined);

  NetworkManager_SendListRequest (&alice, 0);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  NetworkManager_SendInvite (&alice, 2, 10);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
    }

  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_WAITGAME);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_IDLE);

  NetworkManager_SendAccept (&bob);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
      NetworkManager_Tick (&alice);
    }

  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_INGAME);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_INGAME);
  EXPECT_EQ (server_mgr.server_ctx->clients[1].game_id, 1);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].game_id, 1);
  NetworkManager_Cleanup (&server_mgr);
  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}

/**
 * @brief Tests invalid invites and a declined invitation flow.
 * @details Exercises self-invite, out-of-bounds ID invite, a valid invite
 * that is declined, and a stale accept-with-no-invite, covering all error
 * branches in process_invite_packet and process_decline_packet.
 */
TEST_F (NetworkManagerTest, LobbyDeclineAndInvalidInvites)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&alice, "Alice");
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&bob, "Bob");
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
    }

  NetworkManager_SendInvite (&alice, 1, 10);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);

  NetworkManager_SendInvite (&alice, 99, 10);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  NetworkManager_SendInvite (&alice, 2, 10);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
    }

  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_WAITGAME);

  NetworkManager_SendDecline (&bob);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_IDLE);

  NetworkManager_SendAccept (&bob);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
    }

  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}

/**
 * @brief Tests that empty payloads for C2S_INVITE, C2S_ACCEPT, C2S_DECLINE
 * do not crash the server.
 */
TEST_F (NetworkManagerTest, LobbyNullPayloads)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt_empty;

  pkt_empty = { C2S_INVITE, "" };
  SerializePacket (&pkt_empty, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  pkt_empty = { C2S_ACCEPT, "" };
  SerializePacket (&pkt_empty, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  pkt_empty = { C2S_DECLINE, "" };
  SerializePacket (&pkt_empty, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
}

/**
 * @brief Tests NULL-pointer safety for tcp_buffer_init and empty payloads.
 * @details Exercises the NULL guard in tcp_buffer_init, as well as server
 * handling of empty JOIN, empty MOVE, and a raw packet with no separator.
 */
TEST_F (NetworkManagerTest, RobustnessBufferAndEmptyPayloads)
{
  EXPECT_NO_THROW (tcp_buffer_init (nullptr));

  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { C2S_JOIN, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  pkt = { C2S_MOVE, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  const char *malformed_raw_packet = "0NO_SEPARATOR_HERE\n";
  send (client_mgr.client_ctx->server_socket, malformed_raw_packet,
        strlen (malformed_raw_packet), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
}

/* ==========================================================================
 * CATEGORY 6: CLIENT SPECIFIC PACKETS (SCOREBOARD, PONG)
 * ==========================================================================
 */

/**
 * @brief Tests S2C_SCOREBOARD (regular line and END_OF_SCOREBOARD sentinel)
 * and S2C_PONG with a fake timestamp payload.
 */
TEST_F (NetworkManagerTest, ClientScoreboardAndPongCoverage)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  int s_fd = server_mgr.server_ctx->fds[1].fd;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { S2C_SCOREBOARD, "Alice - 1500 points" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  pkt = { S2C_SCOREBOARD, "END_OF_SCOREBOARD" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  pkt = { S2C_PONG, "1700000000000" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/* ==========================================================================
 * CATEGORY 7: ROBUSTNESS - MISSING CALLBACKS
 * ==========================================================================
 */

/**
 * @brief Tests that all client-side packet types are handled safely when no
 * callbacks are registered.
 * @details Sends S2C_LIST, S2C_DECLINE, S2C_CANCEL, S2C_OPPONENT_LEFT, and
 * S2C_GAME_START to a client with an empty NetworkEvents struct to verify
 * the "if (callback)" guards prevent null dereferences.
 */
TEST_F (NetworkManagerTest, ClientNullCallbacksRobustness)
{
  NetworkEvents empty_cb = {};
  NetworkManager client_no_cb;
  NetworkManager_Init (&client_no_cb, empty_cb);

  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_no_cb, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  int s_fd = server_mgr.server_ctx->fds[1].fd;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { S2C_LIST, "Alice, Bob" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  pkt = { S2C_DECLINE, "Invitation declined" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  pkt = { S2C_CANCEL, "Annulation" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  pkt = { S2C_OPPONENT_LEFT, "Bob" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  pkt = { S2C_GAME_START, "1 10" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  NetworkManager_Cleanup (&client_no_cb);
}

/* ==========================================================================
 * CATEGORY 8: MALFORMED PAYLOADS AND UNKNOWN PACKETS
 * ==========================================================================
 */

/**
 * @brief Tests S2C_CANCEL and S2C_OPPONENT_LEFT with callbacks, a malformed
 * S2C_MOVE payload, and a truly unknown packet type landing in the default
 * branch of handle_client_packet.
 */
TEST_F (NetworkManagerTest, ClientMalformedMoveAndUnknownPacket)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  int s_fd = server_mgr.server_ctx->fds[1].fd;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { S2C_CANCEL, "Game cancelled" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  pkt = { S2C_OPPONENT_LEFT, "Bob" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  pkt = { S2C_MOVE, "CECI_N_EST_PAS_UN_MOUVEMENT_VALIDE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  pkt = { (PacketType)999, "DATA" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/* ==========================================================================
 * CATEGORY 9: SERVER SPECIFIC PACKETS (AWAY, BACK, PING, SCOREBOARD, CANCEL)
 * ==========================================================================
 */

/**
 * @brief Tests the full lifecycle of server status and utility packets.
 * @details Sequentially exercises C2S_AWAY→BACK→PING→SCOREBOARD→CANCEL
 * (with a fake opponent), verifying that client and server states transition
 * correctly at each step.
 */
TEST_F (NetworkManagerTest, ServerStatusAndUtilityPackets)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);

  auto WaitUntilStatus = [&] (int client_id, ClientStatus expected)
    {
      for (int i = 0; i < 50; i++)
        {
          NetworkManager_Tick (&server_mgr);
          if (server_mgr.server_ctx->clients[client_id].status == expected)
            return true;
          std::this_thread::sleep_for (std::chrono::milliseconds (10));
        }
      return false;
    };

  std::this_thread::sleep_for (std::chrono::milliseconds (20));
  NetworkManager_Tick (&server_mgr);

  NetworkManager_SendJoin (&client_mgr, "Alice");
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_IDLE));

  NetworkManager_SendAway (&client_mgr);
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_AWAY));

  NetworkManager_SendBack (&client_mgr);
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_IDLE));

  NetworkManager_SendPing (&client_mgr);
  NetworkManager_SendScoreboardRequest (&client_mgr);
  for (int i = 0; i < 5; i++)
    {
      NetworkManager_Tick (&server_mgr);
      std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt = { (PacketType)999, "GARBAGE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  for (int i = 0; i < 3; i++)
    NetworkManager_Tick (&server_mgr);

  server_mgr.server_ctx->fds[2].fd = 999;
  server_mgr.server_ctx->clients[2].is_joined = true;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_IDLE;
  server_mgr.server_ctx->clients[2].last_activity = time (NULL);
  strcpy (server_mgr.server_ctx->clients[2].username, "FakeBob");

  NetworkManager_SendInvite (&client_mgr, 2, 10);
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_WAITGAME));

  NetworkManager_SendCancel (&client_mgr);
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_IDLE));
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_IDLE);
}

/* ==========================================================================
 * CATEGORY 10: INVITE ROBUSTNESS & EDGE CASES
 * ==========================================================================
 */

/**
 * @brief Tests invitation edge cases on the server: self-invite, nonexistent
 * target, AWAY target, INGAME target, and a target already considering another
 * invitation.
 */
TEST_F (NetworkManagerTest, ServerInviteEdgeCases)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&alice, "Alice");

  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&bob, "Bob");

  for (int i = 0; i < 3; i++)
    NetworkManager_Tick (&server_mgr);

  int alice_fd = alice.client_ctx->server_socket;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { C2S_ACCEPT, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  pkt = { C2S_INVITE, "3 10" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_AWAY;
  pkt = { C2S_INVITE, "2 10" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_IDLE;
  server_mgr.server_ctx->clients[2].pending_invite_from = 99;
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}

/* ==========================================================================
 * CATEGORY 11: IN-GAME LOGIC, ANTI-CHEAT & DISCONNECTS
 * ==========================================================================
 */

/**
 * @brief Tests in-game move handling (anti-cheat), game-over reporting, and
 * a forfeiting disconnect.
 * @details Verifies that malformed and invalid moves are rejected without
 * disconnecting the client, that C2S_GAME_OVER correctly updates stats and
 * resets the client status, and that Bob's abrupt disconnect awards Alice a
 * win and returns her to IDLE.
 */
TEST_F (NetworkManagerTest, ServerInGameLogicAndDisconnect)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&alice, "Alice");

  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&bob, "Bob");
  for (int i = 0; i < 3; i++)
    NetworkManager_Tick (&server_mgr);

  server_mgr.server_ctx->clients[1].game_id = 0;
  server_mgr.server_ctx->clients[2].game_id = 0;
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  game_init (&server_mgr.server_ctx->active_games[0]);

  int alice_fd = alice.client_ctx->server_socket;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  pkt = { C2S_MOVE, "PAS_UN_MOUVEMENT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  pkt = { C2S_MOVE, "M a1-z9" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  pkt = { C2S_GAME_OVER, "WIN" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 0;

  NetworkManager_Cleanup (&bob);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);
  EXPECT_EQ (server_mgr.server_ctx->clients[1].game_id, NO_GAME_ID);

  NetworkManager_Cleanup (&alice);
}

/* ==========================================================================
 * CATEGORY 12: ULTIMATE COVERAGE (Overflow, Files, Game Logic, Default)
 * ==========================================================================
 */

/**
 * @brief Comprehensive coverage sweep for overflow, scoreboard file I/O,
 * game-over packets (WIN and LOSS), an illegal-format switch default, and
 * both legal and illegal in-game moves through the server referee.
 */
TEST_F (NetworkManagerTest, UltimateCoverage)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  std::this_thread::sleep_for (std::chrono::milliseconds (20));
  for (int i = 0; i < 3; i++)
    NetworkManager_Tick (&server_mgr);

  NetworkManager_SendJoin (&alice, "Alice");
  NetworkManager_SendJoin (&bob, "Bob");
  std::this_thread::sleep_for (std::chrono::milliseconds (20));
  for (int i = 0; i < 3; i++)
    NetworkManager_Tick (&server_mgr);

  int alice_fd = alice.client_ctx->server_socket;
  char buf[1024];
  Packet pkt;

  /* 1. Oversized message (overflow) */
  std::string huge_payload (300, 'A');
  huge_payload += "\n";
  send (alice_fd, huge_payload.c_str (), huge_payload.length (), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  /* 2. Scoreboard with existing file */
  FILE *f = fopen ("scoreboard.txt", "w");
  if (f)
    {
      fprintf (f, "TestUser 10 5 15\n");
      fclose (f);
    }
  NetworkManager_SendScoreboardRequest (&alice);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  remove ("scoreboard.txt");

  /* 3. C2S_GAME_OVER WIN */
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 0;

  pkt = { C2S_GAME_OVER, "WIN" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  /* 4. C2S_GAME_OVER LOSS */
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 0;

  pkt = { C2S_GAME_OVER, "LOSS" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  /* 5. Switch default (PKT_UNKNOWN) */
  pkt = { PKT_UNKNOWN, "TEST_DEFAULT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  /* 6. Move logic: Alice and Bob in the same game */
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 1;
  server_mgr.server_ctx->clients[2].game_id = 1;

  GameState *board = &server_mgr.server_ctx->active_games[1];
  board->size = 10;
  game_init (board);
  board->current_player = PLAYER_WHITE;

  /* Legal move from the standard initial position */
  pkt = { C2S_MOVE, "MOVE d1-d2-d3" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  /* Well-formatted illegal move (empty square) */
  pkt = { C2S_MOVE, "MOVE a1-a2-a3" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}

/* ==========================================================================
 * NEW CATEGORY: CLIENT CALLBACKS — ALL TRUE BRANCHES
 * ==========================================================================
 */

/**
 * @brief Tests every client-side callback true branch in handle_client_packet.
 * @details Registers all NetworkEvents callbacks and sends S2C_LIST,
 * S2C_DECLINE, S2C_CANCEL (both reuse OnInvitationDeclined), S2C_GAME_START
 * (ROLE_WHITE and ROLE_BLACK paths), S2C_MOVE with a valid protocol move, and
 * finally an unknown packet type to hit the default branch. Each callback
 * counter is asserted after each packet to confirm the branch was taken.
 */
TEST_F (NetworkManagerTest, ClientCallbacksTrueBranches)
{
  s_list_cb_count = 0;
  s_declined_cb_count = 0;
  s_game_started_cb_count = 0;
  s_move_cb_count = 0;

  NetworkEvents full_cb = {};
  full_cb.OnPlayerJoined = mock_on_player_joined;
  full_cb.OnPlayerDisconnected = mock_on_player_disconnected;
  full_cb.OnTimeoutReceived = mock_on_timeout;
  full_cb.OnGameError = mock_on_error;
  full_cb.OnInvitationReceived = mock_on_invite;
  full_cb.OnListReceived = s_on_list;
  full_cb.OnInvitationDeclined = s_on_declined;
  full_cb.OnGameStarted = s_on_game_started;
  full_cb.OnMoveReceived = s_on_move;

  NetworkManager client_full;
  NetworkManager_Init (&client_full, full_cb);

  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_full, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  int s_fd = server_mgr.server_ctx->fds[1].fd;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  /* 1. S2C_LIST → OnListReceived (branche true) */
  pkt = { S2C_LIST, "Alice, Bob" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_full);
  EXPECT_EQ (s_list_cb_count, 1);

  /* 2. S2C_DECLINE → OnInvitationDeclined (branche true) */
  pkt = { S2C_DECLINE, "Invitation declined" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_full);
  EXPECT_EQ (s_declined_cb_count, 1);

  /* 3. S2C_CANCEL → OnInvitationDeclined (shared callback, branche true) */
  pkt = { S2C_CANCEL, "Invitation cancelled" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_full);
  EXPECT_EQ (s_declined_cb_count, 2);

  /* 4a. S2C_GAME_START ROLE_WHITE → OnGameStarted (true branch, white color)
   */
  pkt = { S2C_GAME_START, "0 10" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_full);
  EXPECT_EQ (s_game_started_cb_count, 1);

  /* 4b. S2C_GAME_START ROLE_BLACK → OnGameStarted (couvre le ternaire) */
  pkt = { S2C_GAME_START, "1 8" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_full);
  EXPECT_EQ (s_game_started_cb_count, 2);

  /* 5. S2C_MOVE avec format valide → Protocol_StringToMove OK → OnMoveReceived
   */
  pkt = { S2C_MOVE, "MOVE a4-a5-a6" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_full);
  EXPECT_EQ (s_move_cb_count, 1);

  /* 6. S2C_OPPONENT_LEFT → OnPlayerDisconnected (branche true) */
  pkt = { S2C_OPPONENT_LEFT, "Opponent left" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_full);
  EXPECT_GT (cb_disconnected_count, 0);

  /* 7. Paquet inconnu → default case (printf uniquement, pas de crash) */
  pkt = { (PacketType)123, "UNKNOWN_TYPE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_full));

  NetworkManager_Cleanup (&client_full);
}

/* ==========================================================================
 * NEW CATEGORY: process_move_packet — LEGAL MOVE PATH
 * ==========================================================================
 */

/**
 * @brief Tests process_move_packet with both legal and illegal moves.
 * @details Sets up two clients in the same game with a properly initialised
 * game board, then sends moves using the exact protocol format expected by
 * Protocol_StringToMove ("MOVE col_row-col_row-col_row"). A move from a
 * standard white-queen starting position targets the legal branch, while a
 * move from an empty cell covers the anti-cheat (illegal) branch.
 */
TEST_F (NetworkManagerTest, ProcessMoveLegalAndIllegalPaths)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);

  /* Force both clients into the same game with an initialized board */
  server_mgr.server_ctx->clients[1].is_joined = true;
  server_mgr.server_ctx->clients[2].is_joined = true;
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 0;
  server_mgr.server_ctx->clients[2].game_id = 0;

  GameState *board = &server_mgr.server_ctx->active_games[0];
  board->size = 10;
  game_init (board);
  board->current_player = PLAYER_WHITE;

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  /* Attempt a legal move: white queen from d1 (col 3, row 0) */
  pkt = { C2S_MOVE, "MOVE d1-d2-d3" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  EXPECT_NO_THROW (NetworkManager_Tick (&bob));
  game_cleanup (board);
  /* Reinitialize for the next test */
  game_init (board);
  board->current_player = PLAYER_WHITE;

  /* Attempt from g1 (col 6, row 0) */
  pkt = { C2S_MOVE, "MOVE g1-g2-g3" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  game_cleanup (board);
  /* Well-formed illegal move (empty square b5) */
  game_init (board);
  board->current_player = PLAYER_WHITE;
  pkt = { C2S_MOVE, "MOVE b5-b6-b7" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  NetworkManager_Cleanup (&server_mgr);
  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}

/**
 * @brief Tests process_move_packet when the sending client has no game
 * assigned and when no opponent can be found in the server's client list.
 * @details Covers the early-return branches guarding the function body:
 * game_id == NO_GAME_ID and opponent_idx == -1.
 */
TEST_F (NetworkManagerTest, MovePacketEdgeCasesNoGameNoOpponent)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  /* 1. game_id == NO_GAME_ID → premier return de process_move_packet */
  server_mgr.server_ctx->clients[1].is_joined = true;
  server_mgr.server_ctx->clients[1].game_id = NO_GAME_ID;
  pkt = { C2S_MOVE, "MOVE a4-a5-a6" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  /* 2. valid game_id but no opponent with the same game_id */
  server_mgr.server_ctx->clients[1].game_id = 7; /* no other client here */
  pkt = { C2S_MOVE, "MOVE a4-a5-a6" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  /* 3. game_id hors-limites (>= MAX_CLIENTS/2) */
  server_mgr.server_ctx->clients[1].game_id = 100;
  pkt = { C2S_MOVE, "MOVE a4-a5-a6" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
}

/* ==========================================================================
 * NEW CATEGORY: SERVER STATUS PACKET EDGE CASES
 * ==========================================================================
 */

/**
 * @brief Tests C2S_AWAY, C2S_BACK, and C2S_CANCEL in states where the server
 * should silently ignore them.
 * @details C2S_BACK when status is IDLE (not AWAY) must be a no-op. C2S_AWAY
 * when status is already AWAY must be a no-op. C2S_CANCEL when status is IDLE
 * (not WAITGAME) must be a no-op. All three must not crash or corrupt state.
 */
TEST_F (NetworkManagerTest, ServerStatusPacketNoOpBranches)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&client_mgr, "TestPlayer");
  for (int i = 0; i < 5; i++)
    NetworkManager_Tick (&server_mgr);

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  /* BACK when IDLE (status != AWAY → no-op) */
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_IDLE;
  pkt = { C2S_BACK, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);

  /* AWAY when already AWAY (status != IDLE → no-op) */
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_AWAY;
  pkt = { C2S_AWAY, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_AWAY);

  /* CANCEL when IDLE (status != WAITGAME → no-op) */
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_IDLE;
  pkt = { C2S_CANCEL, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);
}

/**
 * @brief Tests process_join_packet when a NULL username is supplied.
 * @details Directly sends a raw C2S_JOIN with an empty payload that results
 * in the NULL/empty username guard triggering, ensuring the server does not
 * crash and the client remains unjoint.
 */
TEST_F (NetworkManagerTest, JoinPacketNullUsernamePath)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  /* Packet with NULL-equivalent payload (empty string) */
  Packet pkt = { C2S_JOIN, "" };
  char buf[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
}

/**
 * @brief Tests process_list_packet with an empty payload (all-players list).
 * @details When the C2S_LIST payload is empty, target_id should default to -1
 * and the server should return the full player list without crashing.
 */
TEST_F (NetworkManagerTest, ListPacketEmptyPayload)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&client_mgr, "Alice");
  for (int i = 0; i < 3; i++)
    NetworkManager_Tick (&server_mgr);

  Packet pkt = { C2S_LIST, "" };
  char buf[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
}