#include <gtest/gtest.h>
#include <string.h>

extern "C"
{
#include "../../src/network/network_packet.h"
}

/* ==========================================================================
 * TESTS: SERIALIZATION (Struct -> String)
 * ==========================================================================
 */

/**
 * @brief Tests the serialization of a valid packet.
 * @details Checks that the "TYPE|PAYLOAD\n" format is correctly followed.
 */
TEST (NetworkPacketTest, Serialize_ValidPacket)
{
  Packet pkt;
  pkt.type = C2S_MOVE;
  strcpy (pkt.payload, "MOVE a1-a2-a3");

  char buffer[256];
  int written = SerializePacket (&pkt, buffer, sizeof (buffer));

  EXPECT_GT (written, 0);
  EXPECT_STREQ (buffer, "2|MOVE a1-a2-a3\n");
}

/**
 * @brief Tests serialization security (null pointers and buffers too small).
 * @details Covers error branches of SerializePacket.
 */
TEST (NetworkPacketTest, Serialize_ErrorCases)
{
  Packet pkt;
  pkt.type = C2S_JOIN;
  strcpy (pkt.payload, "Alice");
  char buffer[256];

  // 1. Null pointers or size 0
  EXPECT_EQ (SerializePacket (nullptr, buffer, sizeof (buffer)), -1);
  EXPECT_EQ (SerializePacket (&pkt, nullptr, sizeof (buffer)), -1);
  EXPECT_EQ (SerializePacket (&pkt, buffer, 0), -1);

  // 2. Buffer too small to contain the result
  char tiny_buffer[3];
  EXPECT_EQ (SerializePacket (&pkt, tiny_buffer, sizeof (tiny_buffer)), -1);
}

/* ==========================================================================
 * TESTS: DESERIALIZATION (String -> Struct)
 * ==========================================================================
 */

/**
 * @brief Tests the deserialization of a valid string.
 * @details Checks that the string (without \n) is correctly split into TYPE
 * and PAYLOAD.
 */
TEST (NetworkPacketTest, Deserialize_ValidString)
{
  Packet pkt;
  bool success = DeserializePacket ("3|GAME_START_DATA", &pkt);

  EXPECT_TRUE (success);
  EXPECT_EQ (pkt.type, S2C_GAME_START);
  EXPECT_STREQ (pkt.payload, "GAME_START_DATA");
}

/**
 * @brief Tests deserialization security against malformed strings.
 * @details Covers the absence of the '|' separator, non-numeric types, and
 * null pointers.
 */
TEST (NetworkPacketTest, Deserialize_ErrorCases)
{
  Packet pkt;

  // 1. Null pointers
  EXPECT_FALSE (DeserializePacket (nullptr, &pkt));
  EXPECT_FALSE (DeserializePacket ("1|DATA", nullptr));

  // 2. Invalid format: No '|' separator
  EXPECT_FALSE (DeserializePacket ("1DATA", &pkt));

  // 3. Invalid format: The type is not a number
  EXPECT_FALSE (DeserializePacket ("ABC|DATA", &pkt));
}