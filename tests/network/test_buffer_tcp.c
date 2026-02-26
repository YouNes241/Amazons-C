#include "../../src/network/tcp_buffer.h"
#include <stdio.h>
#include <string.h>

// Utility function to display the raw contents of the buffer
void
print_buffer_status (tcp_buffer_t *buf)
{
  printf ("   -> Buffer status : %zu bytes pending [%s]\n", buf->length,
          buf->data);
}

int
main ()
{
  tcp_buffer_t buffer;
  char extracted_msg[256];

  printf ("=== TCP BUFFER TESTING STARTED ===\n\n");
  tcp_buffer_init (&buffer);

  // ---------------------------------------------------------
  // TEST 1 : A perfect message arrived all at once
  // ---------------------------------------------------------
  printf ("TEST 1 : Perfect message\n");
  tcp_buffer_append (&buffer, "e2-e4\n", 6);
  print_buffer_status (&buffer);

  if (tcp_buffer_extract_line (&buffer, extracted_msg, sizeof (extracted_msg)))
    {
      printf ("  Excerpt : '%s'\n", extracted_msg);
    }
  print_buffer_status (&buffer);
  printf ("\n");

  // ---------------------------------------------------------
  // TEST 2 : Fragmentation
  // ---------------------------------------------------------
  printf ("TEST 2 : Message split in two (Network lag)\n");
  printf ("   Adding piece 1 : 'INVI'\n");
  tcp_buffer_append (&buffer, "INVI", 4);
  print_buffer_status (&buffer);

  if (!tcp_buffer_extract_line (&buffer, extracted_msg,
                                sizeof (extracted_msg)))
    {
      printf ("   No full message yet, we're waiting...\n");
    }

  printf ("   Adding piece 2 : 'TE 2\\n'\n");
  tcp_buffer_append (&buffer, "TE 2\n", 5);
  print_buffer_status (&buffer);

  if (tcp_buffer_extract_line (&buffer, extracted_msg, sizeof (extracted_msg)))
    {
      printf ("   Excerpt : '%s'\n", extracted_msg);
    }
  print_buffer_status (&buffer);
  printf ("\n");

  // ---------------------------------------------------------
  // TEST 3 : Two messages joined together (TCP Aggregation) + a piece of the
  // next one
  // ---------------------------------------------------------
  printf ("TEST 3 : Messages joined 'MSG1\\nMSG2\\nDEB'\n");
  tcp_buffer_append (&buffer, "MSG1\nMSG2\nDEB", 13);
  print_buffer_status (&buffer);

  // The loop continues as long as there are complete messages to extract.
  while (
      tcp_buffer_extract_line (&buffer, extracted_msg, sizeof (extracted_msg)))
    {
      printf ("   Excerpt : '%s'\n", extracted_msg);
    }

  printf ("   End of the extraction loop.\n");
  print_buffer_status (&buffer); // 'DEB' must remain in the buffer
  printf ("\n");

  printf ("=== ALL TESTS ARE COMPLETE ===\n");
  return 0;
}