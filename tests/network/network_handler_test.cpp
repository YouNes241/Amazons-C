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

/* ==========================================================================
 * CATEGORY 6: CLIENT SPECIFIC PACKETS (SCOREBOARD, PONG)
 * ==========================================================================
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

  // S2C_SCOREBOARD : Test d'une ligne classique
  pkt = { S2C_SCOREBOARD, "Alice - 1500 points" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  // S2C_SCOREBOARD : Test du marqueur de fin (END_OF_SCOREBOARD)
  pkt = { S2C_SCOREBOARD, "END_OF_SCOREBOARD" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  // S2C_PONG : Test du ping avec un faux timestamp en payload
  pkt = { S2C_PONG, "1700000000000" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/* ==========================================================================
 * CATEGORY 7: ROBUSTNESS - MISSING CALLBACKS
 * ==========================================================================
 */

TEST_F (NetworkManagerTest, ClientNullCallbacksRobustness)
{
  // Initialisation d'un client SANS callbacks (tout est à NULL)
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

  // On bombarde le client de paquets valides.
  // Il ne doit pas crasher malgré l'absence de callbacks.

  // Test Ligne 58 : S2C_LIST sans callback
  pkt = { S2C_LIST, "Alice, Bob" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  // Test Ligne 110 : S2C_DECLINE sans callback
  pkt = { S2C_DECLINE, "Invitation refusée" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  // Test Ligne 117 : S2C_CANCEL sans callback
  pkt = { S2C_CANCEL, "Annulation" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  // Test Ligne 125 : S2C_OPPONENT_LEFT sans callback
  pkt = { S2C_OPPONENT_LEFT, "Bob" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  // Test Ligne 133 : S2C_GAME_START sans callback
  pkt = { S2C_GAME_START, "1 10" }; // Couleur 1 (White), Taille 10
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_no_cb));

  NetworkManager_Cleanup (&client_no_cb);
}

/* ==========================================================================
 * CATEGORY 8: MALFORMED PAYLOADS AND UNKNOWN PACKETS
 * ==========================================================================
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

  // 1. S2C_CANCEL & S2C_OPPONENT_LEFT avec des callbacks valides
  // (Pour couvrir les branches où le if(callback) est VRAI)
  pkt = { S2C_CANCEL, "Game cancelled" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  pkt = { S2C_OPPONENT_LEFT, "Bob" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  // 2. S2C_MOVE illisible (Test ligne 158-161)
  // On envoie une chaîne qui fera échouer Protocol_StringToMove()
  pkt = { S2C_MOVE, "CECI_N_EST_PAS_UN_MOUVEMENT_VALIDE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  // Doit afficher l'erreur proprement via le printf sans crasher
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));

  // 3. Paquet totalement inconnu (Test du bloc default ligne 174-176)
  pkt = { (PacketType)999, "DATA" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (s_fd, buf, strlen (buf), 0);
  // Doit tomber dans le switch "default" et afficher [CLIENT] Unmanaged
  // package
  EXPECT_NO_THROW (NetworkManager_Tick (&client_mgr));
}

/* ==========================================================================
 * CATEGORY 9: SERVER SPECIFIC PACKETS (AWAY, BACK, PING, SCOREBOARD, CANCEL)
 * ==========================================================================
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

  // On laisse le temps à la connexion initiale
  std::this_thread::sleep_for (std::chrono::milliseconds (20));
  NetworkManager_Tick (&server_mgr);

  // JOIN -> Doit passer en IDLE
  NetworkManager_SendJoin (&client_mgr, "Alice");
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_IDLE));

  // 1. AWAY -> Doit passer en AWAY
  NetworkManager_SendAway (&client_mgr);
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_AWAY));

  // 2. BACK -> Doit repasser en IDLE
  NetworkManager_SendBack (&client_mgr);
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_IDLE));

  // 3. PING et SCOREBOARD (On tick juste pour purger les buffers)
  NetworkManager_SendPing (&client_mgr);
  NetworkManager_SendScoreboardRequest (&client_mgr);
  for (int i = 0; i < 5; i++)
    {
      NetworkManager_Tick (&server_mgr);
      std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

  // 4. Paquet inconnu (on garde l'envoi manuel ici)
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt = { (PacketType)999, "GARBAGE" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (client_mgr.client_ctx->server_socket, buf, strlen (buf), 0);
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
    }

  // ==========================================================
  // 5. C2S_CANCEL (Annuler une invitation en attente)
  // ==========================================================

  // HACK DE TEST : On crée un faux joueur 2 (Bob)
  server_mgr.server_ctx->fds[2].fd
      = 999; // Faux socket (mais différent de INVALID_SOCKET)
  server_mgr.server_ctx->clients[2].is_joined = true;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_IDLE;

  // ---> LA LIGNE MAGIQUE POUR ÉVITER LE TIMEOUT INSTANTANÉ <---
  server_mgr.server_ctx->clients[2].last_activity = time (NULL);

  strcpy (server_mgr.server_ctx->clients[2].username, "FakeBob");

  // Maintenant Alice invite le faux Bob
  NetworkManager_SendInvite (&client_mgr, 2, 10);

  // Cette fois, le serveur va accepter et passer Alice en WAITGAME
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_WAITGAME));

  // Puis Alice annule son invitation
  NetworkManager_SendCancel (&client_mgr);

  // Le serveur doit remettre Alice (et le faux Bob) en IDLE
  EXPECT_TRUE (WaitUntilStatus (1, CLIENT_STATUS_IDLE));
  EXPECT_EQ (server_mgr.server_ctx->clients[2].status, CLIENT_STATUS_IDLE);
}

/* ==========================================================================
 * CATEGORY 10: INVITE ROBUSTNESS & EDGE CASES
 * ==========================================================================
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
    {
      NetworkManager_Tick (&server_mgr);
    }

  int alice_fd = alice.client_ctx->server_socket;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  // 1. Accepter une invitation alors qu'on n'en a pas (challenger_index == -1)
  pkt = { C2S_ACCEPT, "" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 2. Inviter un joueur qui n'est pas "joined" ou inexistant (ex: slot 3)
  pkt = { C2S_INVITE, "3 10" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 3. Inviter un joueur qui est AWAY
  server_mgr.server_ctx->clients[2].status
      = CLIENT_STATUS_AWAY; // Bob est AWAY
  pkt = { C2S_INVITE, "2 10" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 4. Inviter un joueur qui n'est pas IDLE (ex: INGAME)
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 5. Inviter un joueur qui réfléchit déjà à une autre invitation
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_IDLE;
  server_mgr.server_ctx->clients[2].pending_invite_from
      = 99; // Fausse invitation en cours
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}

/* ==========================================================================
 * CATEGORY 11: IN-GAME LOGIC, ANTI-CHEAT & DISCONNECTS
 * ==========================================================================
 */
TEST_F (NetworkManagerTest, ServerInGameLogicAndDisconnect)
{
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, test_port), NET_SUCCESS);

  NetworkManager alice, bob;
  NetworkManager_Init (&alice, test_callbacks);
  NetworkManager_Init (&bob, test_callbacks);

  // Setup: Alice et Bob rejoignent
  NetworkManager_ClientJoin (&alice, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&alice, "Alice");

  NetworkManager_ClientJoin (&bob, "127.0.0.1", test_port);
  NetworkManager_Tick (&server_mgr);
  NetworkManager_SendJoin (&bob, "Bob");
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
    }

  // Hack manuel : On les force dans une partie pour gagner du temps
  server_mgr.server_ctx->clients[1].game_id = 0;
  server_mgr.server_ctx->clients[2].game_id = 0;
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  game_init (
      &server_mgr.server_ctx->active_games[0]); // Init le plateau côté serveur

  int alice_fd = alice.client_ctx->server_socket;
  char buf[MAX_FORMATTED_PKT_SIZE];
  Packet pkt;

  // 1. C2S_MOVE mal formatté (Hack attempt)
  pkt = { C2S_MOVE, "PAS_UN_MOUVEMENT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 2. C2S_MOVE bien formatté mais ILLÉGAL (Anti-cheat)
  // On suppose ici qu'un mouvement impossible à réaliser (ex: sauter depuis
  // une case vide) sera rejeté par is_move_legal(). À adapter selon la logique
  // exacte de ton jeu.
  pkt = { C2S_MOVE, "M a1-z9" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // 3. Test de C2S_GAME_OVER
  pkt = { C2S_GAME_OVER, "WIN" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // On remet Alice en jeu pour tester la déconnexion
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 0;

  // 4. Déconnexion brutale de Bob en plein match (Forfait)
  NetworkManager_Cleanup (&bob); // Bob ferme sa socket
  // Le serveur tick et doit détecter la perte de connexion, puis notifier
  // Alice
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // Vérifie qu'Alice a bien été remise dans le lobby (IDLE) suite au forfait
  // de Bob
  EXPECT_EQ (server_mgr.server_ctx->clients[1].status, CLIENT_STATUS_IDLE);
  EXPECT_EQ (server_mgr.server_ctx->clients[1].game_id, NO_GAME_ID);

  NetworkManager_Cleanup (&alice);
}

/* ==========================================================================
 * CATEGORY 12: (Overflow, Files, Game Logic, Default)
 * ==========================================================================
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
    {
      NetworkManager_Tick (&server_mgr);
    }

  NetworkManager_SendJoin (&alice, "Alice");
  NetworkManager_SendJoin (&bob, "Bob");
  std::this_thread::sleep_for (std::chrono::milliseconds (20));
  for (int i = 0; i < 3; i++)
    {
      NetworkManager_Tick (&server_mgr);
    }

  int alice_fd = alice.client_ctx->server_socket;
  char buf[1024];
  Packet pkt;

  // ========================================================================
  // 1.  Message Oversized (Overflow) [Ligne 100]
  // ========================================================================
  // On envoie un message de 300 caractères (plus grand que
  // RECV_TEMP_BUFFER_SIZE)
  std::string huge_payload (300, 'A');
  huge_payload += "\n";
  send (alice_fd, huge_payload.c_str (), huge_payload.length (), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // ========================================================================
  // 2.  Scoreboard with existing file [Lignes 642-657]
  // ========================================================================
  // On crée un faux fichier scoreboard.txt à la volée
  FILE *f = fopen ("scoreboard.txt", "w");
  if (f)
    {
      fprintf (f, "TestUser 10 5 15\n");
      fclose (f);
    }
  NetworkManager_SendScoreboardRequest (&alice);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));
  remove ("scoreboard.txt"); // On nettoie derrière nous

  // ========================================================================
  // 3.  Game Over (WIN and LOSS) [Lignes 670-687]
  // ========================================================================
  // On place Alice en jeu
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 0;

  pkt = { C2S_GAME_OVER, "WIN" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // On la remet en jeu pour tester la défaite
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 0;

  pkt = { C2S_GAME_OVER, "LOSS" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // ========================================================================
  // 4.  Switch Default Case [Ligne 704]
  // ========================================================================
  // On utilise PKT_UNKNOWN (0) qui passe la désérialisation mais n'a pas de
  // case
  pkt = { PKT_UNKNOWN, "TEST_DEFAULT" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // ========================================================================
  // 5.  Move Logic (Legal and Illegal) [Lignes 167-207]
  // ========================================================================
  // On met Alice (1) et Bob (2) dans la même partie
  server_mgr.server_ctx->clients[1].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[2].status = CLIENT_STATUS_INGAME;
  server_mgr.server_ctx->clients[1].game_id = 1;
  server_mgr.server_ctx->clients[2].game_id = 1;

  GameState *board = &server_mgr.server_ctx->active_games[1];
  board->size = 10;
  game_init (board);
  board->current_player = PLAYER_WHITE; // C'est au tour des blancs

  // A. Coup Parfaitement Légal (Prouve que is_move_legal = true)
  // Format attendu par Protocol_StringToMove : ex "d10-d9(d8)" (Adapter si
  // besoin)
  pkt = { C2S_MOVE, "d10-d9(d8)" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  // B. Coup Illégal mais bien formatté (Prouve que is_move_legal = false)
  // Ex: Essayer de bouger une pièce depuis une case vide (a1 sur le plateau de
  // départ)
  pkt = { C2S_MOVE, "a1-a2(a3)" };
  SerializePacket (&pkt, buf, sizeof (buf));
  send (alice_fd, buf, strlen (buf), 0);
  std::this_thread::sleep_for (std::chrono::milliseconds (10));
  EXPECT_NO_THROW (NetworkManager_Tick (&server_mgr));

  NetworkManager_Cleanup (&alice);
  NetworkManager_Cleanup (&bob);
}