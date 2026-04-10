#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>

extern "C"
{
#include "../../src/network/network_includes/network_internal.h"
#include "../../src/network/network_includes/network_packet.h"
#include "ai_factory.h"
#include "network_protocol.h"
}

/* ==========================================================================
 * GLOBAL VARIABLES FOR INTERCEPTING THE NETWORK MOVE
 * ==========================================================================
 */
static Move g_last_received_move;
static bool g_move_received_flag = false;

extern "C" void
mock_on_move_received (Move m)
{
  g_last_received_move = m;
  g_move_received_flag = true;
}

/* ==========================================================================
 * CATEGORY 1: INTEGRATION TESTS (NETWORK + AI)
 * ==========================================================================
 */

TEST (NetworkIntegrationTest, AI_Plays_Over_TCP)
{
  // 1. Network Initialization (Server + Client)
  NetworkManager server_mgr, client_mgr;
  NetworkEvents callbacks = {};
  callbacks.OnMoveReceived = mock_on_move_received;

  NetworkManager_Init (&server_mgr, callbacks);
  NetworkManager_Init (&client_mgr, callbacks);

  int port = 9500;
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", port), 0);
  NetworkManager_Tick (&server_mgr);

  // --- : LE HANDSHAKE ---
  // teh clietn has to identify before playing
  Packet join_pkt = { C2S_JOIN, "AI_Player" };
  char join_buf[256];
  SerializePacket (&join_pkt, join_buf, sizeof (join_buf));
  NetworkManager_Send (&client_mgr, 0, join_buf);

  NetworkManager_Tick (&server_mgr); // the server get  JOIN and send WELCOME
  NetworkManager_Tick (&client_mgr); // the client get WELCOME

  g_move_received_flag = false;

  // 2. Creating a fake game board for the AI
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * 10);
  for (int i = 0; i < 10; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * 10);
      for (int j = 0; j < 10; j++)
        cc[i][j] = CONTENT_EMPTY;
    }
  cc[0][3] = CONTENT_BLACK_QUEEN;
  cc[9][6] = CONTENT_WHITE_QUEEN;

  GameState state
      = { 10,  false, 50, PLAYER_WHITE, GAME_IN_PROGRESS, cc, 0.0, 0.0,
          0.0, {},    {} };

  // 3. AI generates a real hit
  ConfigMap empty_params = { 0, nullptr };
  AI_Interface randomAI = create_ai ("random", empty_params);
  Move ai_move = randomAI.vtable->get_best_move (&randomAI, state);

  // 4. TRANSLATION & PACKAGING
  char raw_move[64];
  Protocol_MoveToString (ai_move, raw_move, sizeof (raw_move));

  raw_move[strcspn (raw_move, "\n")] = 0;

  Packet move_pkt;

  move_pkt.type = S2C_MOVE;
  strncpy (move_pkt.payload, raw_move, MAX_PAYLOAD_SIZE);

  char packet_buffer[256];
  SerializePacket (&move_pkt, packet_buffer, sizeof (packet_buffer));
  printf ("[TEST] AI playing, packet sent : %s\n", packet_buffer);

  // 5. TCP SEND: The server sends the packet to Client 1
  // (Index 1 corresponds to the first client connected to the server)
  ASSERT_EQ (NetworkManager_Send (&server_mgr, 1, packet_buffer), 0);

  // 6. TCP RECEIVE: The client ticks, deserializes, and calls the callback
  NetworkManager_Tick (&client_mgr);

  // 7. VERIFICATION
  ASSERT_TRUE (g_move_received_flag) << "OnMoveReceived callback not called";

  EXPECT_EQ (g_last_received_move.queen_src.row, ai_move.queen_src.row);
  EXPECT_EQ (g_last_received_move.queen_src.col, ai_move.queen_src.col);
  EXPECT_EQ (g_last_received_move.queen_dst.row, ai_move.queen_dst.row);
  EXPECT_EQ (g_last_received_move.queen_dst.col, ai_move.queen_dst.col);
  EXPECT_EQ (g_last_received_move.arrow_dst.row, ai_move.arrow_dst.row);
  EXPECT_EQ (g_last_received_move.arrow_dst.col, ai_move.arrow_dst.col);

  printf ("[TEST] SUCCESS: The AI attack passed through the local network "
          "intact\n");

  // 8. Cleanup
  randomAI.vtable->clean (&randomAI);
  for (int i = 0; i < 10; i++)
    free (cc[i]);
  free (cc);
  NetworkManager_Cleanup (&client_mgr);
  NetworkManager_Cleanup (&server_mgr);
}

/* ==========================================================================
 * CATEGORY 2: PROTOCOL UNIT TESTS (EDGE CASES & ROBUSTNESS)
 * ==========================================================================
 */

TEST (NetworkProtocolTest, MoveToString_NullBufferGuard)
{
  Move m = { { 0, 0 }, { 0, 1 }, { 0, 2 } };
  EXPECT_NO_THROW (Protocol_MoveToString (m, nullptr, 0));
}

TEST (NetworkProtocolTest, StringToMove_NullPointerGuards)
{
  Move m;
  EXPECT_FALSE (Protocol_StringToMove (nullptr, &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-b2-c3\n", nullptr));
}

TEST (NetworkProtocolTest, StringToMove_InvalidFormats)
{
  Move m;
  EXPECT_FALSE (Protocol_StringToMove ("PATA_CREPE", &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-b2", &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a-b-c", &m));
  EXPECT_FALSE (Protocol_StringToMove ("JUMP a1-b2-c3", &m));
}

TEST (NetworkProtocolTest, StringToMove_OutOfBoundsCoordinates)
{
  Move m;

  // --- SOURCE QUEEN ---
  EXPECT_FALSE (
      Protocol_StringToMove ("MOVE A1-a2-a3\n", &m)); // Uppercase (Invalid)
  EXPECT_FALSE (
      Protocol_StringToMove ("MOVE !1-a2-a3\n", &m)); // Special character
  EXPECT_FALSE (
      Protocol_StringToMove ("MOVE a0-a2-a3\n", &m)); // Zero (Invalid)
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a27-a2-a3\n", &m)); // Beyond 26

  // --- DESTINATION QUEEN ---
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-A2-a3\n", &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-?2-a3\n", &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-a0-a3\n", &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-a27-a3\n", &m));

  // --- ARROW DESTINATION ---
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-a2-A3\n", &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-a2-+3\n", &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-a2-a0\n", &m));
  EXPECT_FALSE (
      Protocol_StringToMove ("MOVE a1-a2-a99\n", &m)); // Nombre absurde
}