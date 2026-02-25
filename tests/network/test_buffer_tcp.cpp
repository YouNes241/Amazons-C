#include <gtest/gtest.h>
#include <string>

extern "C"
{
#include "../../src/network/tcp_buffer.h"
}

void
print_buffer_debug (const tcp_buffer_t *buf)
{
  printf ("   -> Buffer status : %zu bytes pending [%s]\n", buf->length,
          buf->data);
}

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

// --- TEST 1 : perfect message ---
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

// --- TEST 2 : Fragmentation ---
TEST_F (TcpBufferTest, HandleFragmentation)
{
  printf ("TEST 2 : Message split in two (Network lag)\n");

  // Part 1
  printf ("   Adding 'INVI'...\n");
  tcp_buffer_append (&buffer, "INVI", 4);
  print_buffer_debug (&buffer);

  int result = tcp_buffer_extract_line (&buffer, extracted_msg,
                                        sizeof (extracted_msg));
  EXPECT_EQ (result, 0); // should be an echec
  printf ("    No full message yet...\n");

  // Part 2
  printf ("   Adding 'TE 2\\n'...\n");
  tcp_buffer_append (&buffer, "TE 2\n", 5);
  print_buffer_debug (&buffer);

  result = tcp_buffer_extract_line (&buffer, extracted_msg,
                                    sizeof (extracted_msg));

  EXPECT_EQ (result, 1);
  EXPECT_STREQ (extracted_msg, "INVITE 2");
  printf ("    Extracted: '%s'\n", extracted_msg);
}

// --- TEST 3 : TCP Aggregation  While ---
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

  EXPECT_EQ (count, 2);

  EXPECT_EQ (buffer.length, 3);

  print_buffer_debug (&buffer);
}