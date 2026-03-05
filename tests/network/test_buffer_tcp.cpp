#include <gtest/gtest.h>
#include <string>
#include <vector>

extern "C"
{
#include "../../src/network/tcp_buffer.h"
}

class TcpBufferTest : public ::testing::Test
{
protected:
  tcp_buffer_t buffer;
  char extracted_msg[256];

  void
  SetUp () override
  {
    tcp_buffer_init (&buffer);
  }
};

/* ==========================================================================
 * CATEGORY 1: CORE FUNCTIONALITY (TCP STREAM LOGIC)
 * ==========================================================================
 */

/**
 * @brief Tests perfect message arrival and fragmented message handling.
 * @details Covers standard TCP scenarios: data arriving all at once and data
 * split into multiple parts (Network Lag).
 */
TEST_F (TcpBufferTest, StreamHandling)
{
  // 1. Perfect message
  tcp_buffer_append (&buffer, "HELLO\n", 6);
  EXPECT_EQ (tcp_buffer_extract_line (&buffer, extracted_msg, 256), 1);
  EXPECT_STREQ (extracted_msg, "HELLO");

  // 2. Fragmented message
  tcp_buffer_append (&buffer, "PA", 2);
  EXPECT_EQ (tcp_buffer_extract_line (&buffer, extracted_msg, 256),
             0); // No \n yet
  tcp_buffer_append (&buffer, "RT2\n", 4);
  EXPECT_EQ (tcp_buffer_extract_line (&buffer, extracted_msg, 256), 1);
  EXPECT_STREQ (extracted_msg, "PART2");
}

/**
 * @brief Tests message aggregation (Multiple lines in one packet).
 * @details Verifies that the extraction logic can be called in a loop to
 * clear multiple complete messages while leaving partial data behind.
 */
TEST_F (TcpBufferTest, StreamAggregation)
{
  tcp_buffer_append (&buffer, "MSG1\nMSG2\nPARTIAL", 17);

  EXPECT_EQ (tcp_buffer_extract_line (&buffer, extracted_msg, 256), 1);
  EXPECT_STREQ (extracted_msg, "MSG1");

  EXPECT_EQ (tcp_buffer_extract_line (&buffer, extracted_msg, 256), 1);
  EXPECT_STREQ (extracted_msg, "MSG2");

  EXPECT_EQ (tcp_buffer_extract_line (&buffer, extracted_msg, 256),
             0); // PARTIAL remains
  EXPECT_EQ (buffer.length, 7);
}

/* ==========================================================================
 * CATEGORY 2: BOUNDARIES & OVERFLOW PROTECTION
 * ==========================================================================
 */

/**
 * @brief Tests buffer capacity and protection against overflows.
 * @details Ensures the internal array does not overflow and that extraction
 * handles output buffers that are too small.
 */
TEST_F (TcpBufferTest, CapacityAndSafety)
{
  // 1. Buffer Overflow (Input too large)
  std::vector<char> huge (TCP_BUFFER_SIZE + 1, 'A');
  EXPECT_FALSE (tcp_buffer_append (&buffer, huge.data (), huge.size ()));

  // 2. Small Output Buffer (Truncation test)
  tcp_buffer_append (&buffer, "TOO_LONG\n", 9);
  char small_out[4];
  EXPECT_EQ (tcp_buffer_extract_line (&buffer, small_out, 4), 1);
  EXPECT_EQ (strlen (small_out), 3); // Must be null-terminated safely
}

/**
 * @brief Tests boundary limits and giant single messages.
 */
TEST_F (TcpBufferTest, BoundaryHandling)
{
  // Fill to exact limit
  std::string limit (TCP_BUFFER_SIZE - 2, 'G');
  limit += "\n";
  EXPECT_TRUE (tcp_buffer_append (&buffer, limit.c_str (), limit.length ()));

  // Malformed stream: No newline and full buffer
  tcp_buffer_init (&buffer);
  std::string garbage (TCP_BUFFER_SIZE - 1, 'X');
  tcp_buffer_append (&buffer, garbage.c_str (), garbage.length ());
  EXPECT_EQ (tcp_buffer_extract_line (&buffer, extracted_msg, 256), 0);
  EXPECT_FALSE (tcp_buffer_append (&buffer, "EXTRA", 5)); // Buffer is full
}

/* ==========================================================================
 * CATEGORY 3: ROBUSTNESS (DEFENSIVE PROGRAMMING)
 * ==========================================================================
 */

/**
 * @brief Tests NULL pointer safety for all public buffer functions.
 * @details Validates that the API does not crash when passed invalid pointers.
 */
TEST_F (TcpBufferTest, NullSafetyGuards)
{
  EXPECT_FALSE (tcp_buffer_append (nullptr, "test", 4));
  EXPECT_FALSE (tcp_buffer_append (&buffer, nullptr, 4));
  EXPECT_EQ (tcp_buffer_extract_line (nullptr, extracted_msg, 256), 0);
  EXPECT_EQ (tcp_buffer_extract_line (&buffer, nullptr, 256), 0);
}