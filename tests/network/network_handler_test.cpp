#include "network_test_fixture.h"

/* ==========================================================================
 * CATEGORY 2: HANDSHAKE & LOBBY LOGIC
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, EventLoopHandshakeSuccess)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  // Client -> Server : C2S_JOIN
  Packet join_pkt = { C2S_JOIN, "Alice" };
  char buffer[MAX_FORMATTED_PKT_SIZE];
  SerializePacket (&join_pkt, buffer, sizeof (buffer));
  send (client_mgr.client_ctx->server_socket, buffer, strlen (buffer), 0);

  // Server processes JOIN, fires OnPlayerJoined, and sends WELCOME
  NetworkManager_Tick (&server_mgr);
  EXPECT_EQ (cb_joined_count, 1);

  // Client receives WELCOME
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

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

/* ==========================================================================
 * CATEGORY 3: PACKET PROCESSING & ROUTING
 * ==========================================================================
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

  // Hack: Place both clients in the same game "1" to test the router
  server_mgr.server_ctx->clients[1].game_id = 1;
  server_mgr.server_ctx->clients[2].game_id = 1;
  server_mgr.server_ctx->clients[1].is_joined = true;
  server_mgr.server_ctx->clients[2].is_joined = true;

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  // Test TIMEOUT routing
  pkt = { PKT_TIMEOUT, "TIMEOUT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client1.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_mgr); // Server routes
  NetworkManager_Tick (&client2);    // Client 2 receives

  // Test MOVE routing
  pkt = { C2S_MOVE, "MOVE a1-a2-a3" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client1.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_mgr); // Server converts to S2C_MOVE and routes
  NetworkManager_Tick (&client2);

  // Test unknown packet (hits "default" case)
  pkt = { (PacketType)99, "GARBAGE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client1.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (
      NetworkManager_Tick (&server_mgr)); // Should cleanly hit default print

  NetworkManager_Cleanup (&client1);
  NetworkManager_Cleanup (&client2);
}

TEST_F (NetworkManagerTest, ClientSwitchCoverage)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  int s_fd = server_mgr.server_ctx->fds[1].fd;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  // 1. Timeout Received
  pkt = { PKT_TIMEOUT, "TIMEOUT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_mgr);
  EXPECT_EQ (cb_timeout_count, 1);

  // 2. Error Received (With callback)
  pkt = { S2C_ERROR, "Invalid Move" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_mgr);
  EXPECT_EQ (cb_error_count, 1);

  // 3. Error Received (Without callback to hit fallback print)
  client_mgr.callbacks.OnGameError = nullptr;
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  // 4. Invite Received
  pkt = { S2C_INVITE, "2" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  NetworkManager_Tick (&client_mgr);
  EXPECT_EQ (cb_invite_count, 1);

  // 5. Unknown packet
  pkt = { (PacketType)99, "GARBAGE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

TEST_F (NetworkManagerTest, MoveCommandParsingAndCallbacks)
{
  NetworkEvents empty_cb = {}; // No callbacks registered
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

  // Malformed payload
  pkt = { C2S_MOVE, "MOVE INVALID" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_no_cb.client_ctx->server_socket, buf, strlen (buf), 0);
  NetworkManager_Tick (&server_no_cb);

  // Valid MOVE, but Server has NO callback
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

TEST_F (NetworkManagerTest, LobbyFullLifecycle_InviteAndAccept)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  // Connect Alice
  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&alice, "Alice");
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  // Connect Bob
  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&bob, "Bob");
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
    }

  // Verify they are actually joined before proceeding
  ASSERT_TRUE (server_mgr.server_ctx->clients[1].is_joined);
  ASSERT_TRUE (server_mgr.server_ctx->clients[2].is_joined);

  // 1. Alice requests the player list
  NetworkManager_SendListRequest (&alice, 0); // Target 0 for full list
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  // 2. Alice invites Bob
  NetworkManager_SendInvite (&alice, 2, 10);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
    }

  // Verify states: Alice is WAITGAME, Bob is still IDLE
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_WAITGAME);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_IDLE);

  // 3. Bob accepts Alice's invite
  NetworkManager_SendAccept (&bob);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&bob);
      NetworkManager_Tick (&alice);
    }

  // Verify states: Both are INGAME and share the same game_id
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_INGAME);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_INGAME);
  EXPECT_EQ (server_mgr.server_ctx->clients[1].game_id, 1);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].game_id, 1);

  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}

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

  // 1. Invalid Invite: Alice invites herself (Target 1)
  NetworkManager_SendInvite (&alice, 1, 10);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);

  // 2. Invalid Invite: Alice invites an out-of-bounds ID (99)
  NetworkManager_SendInvite (&alice, 99, 10);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
      NetworkManager_Tick (&alice);
    }

  // 3. Alice invites Bob, but Bob declines
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

  // Both should be back to IDLE
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_IDLE);

  // 4. Bob tries to accept an expired/non-existent invite from Alice
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
 * @brief Tests the server parsing empty payloads for the new Lobby packets.
 * @details Verifies that receiving empty strings via the network does not
 * crash the server.
 */
TEST_F (NetworkManagerTest, LobbyNullPayloads)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt_empty;

  // 1. Test empty C2S_INVITE
  pkt_empty = { C2S_INVITE, "" };
  SerializePacket (&pkt_empty, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 2. Test empty C2S_ACCEPT
  pkt_empty = { C2S_ACCEPT, "" };
  SerializePacket (&pkt_empty, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 3. Test empty C2S_DECLINE
  pkt_empty = { C2S_DECLINE, "" };
  SerializePacket (&pkt_empty, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
}

/**
 * @brief Targets the payload validation in handlers and buffer initialization.
 * @details Ensures that passing null pointers to buffer utils or sending empty
 * payloads over the network does not cause Segmentation Faults.
 */
TEST_F (NetworkManagerTest, RobustnessBufferAndEmptyPayloads)
{
  // 1. Test tcp_buffer_init with nullptr (guard clause check)
  EXPECT_NO_THROW (tcp_buffer_init (nullptr));

  // Initialize server and client for payload tests
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", test_port),
             NET_SUCCESS);
  NetworkManager_Tick (&server_mgr);

  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  // 2. Test empty JOIN payload
  // The server should process it cleanly (username will just be empty)
  pkt = { C2S_JOIN, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 3. Test empty MOVE payload
  // Protocol_StringToMove should gracefully return false and ignore it
  pkt = { C2S_MOVE, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 4. Test extreme edge case: packet with no separator (bypassing
  // SerializePacket) This directly tests DeserializePacket's strchr fallback
  const char *malformed_raw_packet = "0NO_SEPARATOR_HERE\n";
  send (client_mgr.client_ctx->server_socket, malformed_raw_packet,
        strlen (malformed_raw_packet), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
}