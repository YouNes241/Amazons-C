#include <gtest/gtest.h>
#include <unistd.h>

extern "C"
{
#include "../../src/network/network_internal.h"
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

/**
 * @brief Tests the full pipeline: AI decision -> Serialization -> TCP ->
 * Deserialization.
 * @details Creates a local server/client pair, generates a move using the AI,
 * sends it over the network, and verifies that the exact same move is received
 * and decoded by the server's callback.
 */
TEST (NetworkIntegrationTest, AI_Plays_Over_TCP)
{
  // 1. Network Initialization (Server + Client)
  NetworkManager server_mgr, client_mgr;
  NetworkEvents callbacks = {};
  callbacks.OnMoveReceived = mock_on_move_received; // we listen

  NetworkManager_Init (&server_mgr, callbacks);
  NetworkManager_Init (&client_mgr, callbacks);

  int port = 9500;
  ASSERT_EQ (NetworkManager_ServerStart (&server_mgr, port), 0);
  ASSERT_EQ (NetworkManager_ClientJoin (&client_mgr, "127.0.0.1", port), 0);

  NetworkManager_Tick (
      &server_mgr);             // The server accepts the client's connection
  g_move_received_flag = false; // flag reset

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

  GameState state = { .size = 10,
                      .board = cc,
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 0 };

  // 3. AI generates a real hit
  ConfigMap empty_params = { nullptr, 0 };
  AI_Interface randomAI = create_ai ("random", empty_params);
  Move ai_move = randomAI.vtable->get_best_move (&randomAI, state);

  // 4. TRANSLATION: Move ➔ String (Serialization)
  char network_message[64];
  Protocol_MoveToString (ai_move, network_message);
  printf ("[TEST] AI playing, translation sent : %s", network_message);

  // 5. TCP SEND: The client sends the text to the server
  ASSERT_EQ (NetworkManager_Send (&client_mgr, 0, network_message), 0);

  // 6. TCP RECEPTION: The server ticks, reads the buffer, translates, and
  // calls OnMoveReceived
  NetworkManager_Tick (&server_mgr);

  // 7. VERIFICATION: Did the attack arrive intact across the network?
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

/**
 * @brief Tests NULL pointer protection for Protocol_MoveToString.
 * @details Ensures the function returns safely without causing a segmentation
 * fault when provided a NULL output buffer.
 * **Coverage Goal:** Exercises the error branch where buffer_out is NULL.
 */
TEST (NetworkProtocolTest, MoveToString_NullBufferGuard)
{
  Move m = { { 0, 0 }, { 0, 1 }, { 0, 2 } };
  EXPECT_NO_THROW (Protocol_MoveToString (m, nullptr));
}

/**
 * @brief Tests NULL pointer protection for Protocol_StringToMove.
 * @details Ensures the parser safely rejects NULL input strings or NULL
 * output structures, preventing memory access violations.
 * **Coverage Goal:** Exercises the error branches for NULL parameters.
 */
TEST (NetworkProtocolTest, StringToMove_NullPointerGuards)
{
  Move m;
  EXPECT_FALSE (Protocol_StringToMove (nullptr, &m));
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-b2-c3\n", nullptr));
}

/**
 * @brief Tests the protocol parser against malformed and invalid strings.
 * @details Feeds incomplete or completely invalid strings to the parser to
 * verify that it safely rejects them without altering game state.
 * **Coverage Goal:** Exercises the false branch of the sscanf validation
 * (parsed != 6).
 */
TEST (NetworkProtocolTest, StringToMove_InvalidFormats)
{
  Move m;

  // 1. Complete garbage (No coordinates)
  EXPECT_FALSE (Protocol_StringToMove ("PATA_CREPE", &m));

  // 2. Incomplete move (Missing the arrow destination)
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a1-b2", &m));

  // 3. Missing row numbers
  EXPECT_FALSE (Protocol_StringToMove ("MOVE a-b-c", &m));

  // 4. Wrong prefix (Not matching "MOVE ")
  EXPECT_FALSE (Protocol_StringToMove ("JUMP a1-b2-c3", &m));
}