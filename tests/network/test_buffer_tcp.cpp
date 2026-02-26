#include <gtest/gtest.h>
#include <string>
#include <vector>

extern "C"
{
#include "../../src/network/tcp_buffer.h"
}

/* ==========================================================================
 * UTILITY FUNCTIONS
 * ==========================================================================
 */
void
print_buffer_debug (const tcp_buffer_t *buf)
{
  printf ("   -> Buffer status : %zu bytes pending [%s]\n", buf->length,
          buf->data);
}

/* ==========================================================================
 * TEST FIXTURE CLASS
 * ==========================================================================
 */
class TcpBufferTest : public ::testing::Test
{
protected:
  tcp_buffer_t buffer;
  char extracted_msg[256];

  void
  SetUp () override
  {
    printf ("\n--- START TEST ---\n");
    tcp_buffer_init (&buffer);
  }

  void
  TearDown () override
  {
    printf ("--- END TEST ---\n");
  }
};

/* ==========================================================================
 * TESTS
 * ==========================================================================
 */

/**
 * @brief Tests the extraction of a perfectly formatted, complete message.
 * @details Verifies that a message appended with its newline character is
 * successfully extracted in one go, leaving the buffer empty.
 */
TEST_F (TcpBufferTest, HandlePerfectMessage)
{
  printf ("TEST 1 : Perfect message arriving at once\n");

  tcp_buffer_append (&buffer, "e2-e4\n", 6);
  print_buffer_debug (&buffer);

  int result = tcp_buffer_extract_line (&buffer, extracted_msg,
                                        sizeof (extracted_msg));

  EXPECT_EQ (result, 1);
  EXPECT_STREQ (extracted_msg, "e2-e4");

  if (result)
    printf ("    Extracted: '%s'\n", extracted_msg);

  print_buffer_debug (&buffer);
}

/**
 * @brief Tests the handling of a fragmented message (Network Lag).
 * @details Verifies that the buffer waits for the newline character. The first
 * extraction attempt should safely fail, and the second should succeed after
 * the rest of the message arrives.
 */
TEST_F (TcpBufferTest, HandleFragmentation)
{
  printf ("TEST 2 : Message split in two (Network lag)\n");

  // Part 1: First half of the message
  printf ("   Adding 'INVI'...\n");
  tcp_buffer_append (&buffer, "INVI", 4);
  print_buffer_debug (&buffer);

  int result = tcp_buffer_extract_line (&buffer, extracted_msg,
                                        sizeof (extracted_msg));
  EXPECT_EQ (result, 0); // Should fail (no \n yet)
  printf ("    No full message yet...\n");

  // Part 2: Second half of the message
  printf ("   Adding 'TE 2\\n'...\n");
  tcp_buffer_append (&buffer, "TE 2\n", 5);
  print_buffer_debug (&buffer);

  result = tcp_buffer_extract_line (&buffer, extracted_msg,
                                    sizeof (extracted_msg));

  EXPECT_EQ (result, 1);
  EXPECT_STREQ (extracted_msg, "INVITE 2");
  printf ("    Extracted: '%s'\n", extracted_msg);
}

/**
 * @brief Tests the extraction of multiple messages glued together (TCP
 * Aggregation).
 * @details Simulates TCP stream aggregation where multiple complete messages
 * and a partial message arrive at once. Verifies that the while loop logic
 * extracts exactly the complete ones and leaves the partial one in the buffer.
 */
TEST_F (TcpBufferTest, HandleAggregation)
{
  printf ("TEST 3 : Messages joined 'MSG1\\nMSG2\\nDEB'\n");

  tcp_buffer_append (&buffer, "MSG1\nMSG2\nDEB", 13);
  print_buffer_debug (&buffer);

  int count = 0;

  while (
      tcp_buffer_extract_line (&buffer, extracted_msg, sizeof (extracted_msg)))
    {
      count++;
      printf ("    Extracted #%d: '%s'\n", count, extracted_msg);

      if (count == 1)
        {
          EXPECT_STREQ (extracted_msg, "MSG1");
        }
      else if (count == 2)
        {
          EXPECT_STREQ (extracted_msg, "MSG2");
        }
      else
        {
          FAIL () << "Error : too much messages extracted !";
        }
    }

  printf ("   End of the extraction loop.\n");

  // We must have extracted exactly 2 messages
  EXPECT_EQ (count, 2);

  // The remaining string "DEB" is 3 bytes long
  EXPECT_EQ (buffer.length, 3);

  print_buffer_debug (&buffer);
}

/**
 * @brief Tests the buffer's protection against memory overflow.
 * @details Attempts to append data that exceeds the maximum buffer size
 * (TCP_BUFFER_SIZE) to ensure the append function safely rejects it without
 * causing a segmentation fault.
 */
TEST_F (TcpBufferTest, HandleBufferOverflow)
{
  printf ("TEST 4 : Buffer Overflow Protection\n");

  // Create a dummy string that is larger than the buffer's capacity
  std::vector<char> huge_data (TCP_BUFFER_SIZE + 100, 'A');

  // Try to append this huge data
  bool success
      = tcp_buffer_append (&buffer, huge_data.data (), huge_data.size ());

  // It should safely reject the append and return false
  EXPECT_FALSE (success);
  EXPECT_EQ (buffer.length, 0); // Buffer should remain empty

  printf ("   Buffer successfully rejected the huge payload.\n");
}