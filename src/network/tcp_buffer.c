#include "tcp_buffer.h"
#include <string.h>

void
tcp_buffer_init (tcp_buffer_t *buf)
{
  buf->length = 0;
  memset (buf->data, 0, TCP_BUFFER_SIZE);
}

bool
tcp_buffer_append (tcp_buffer_t *buf, const char *new_data, size_t new_len)
{
  // security: prevent buffer overflow
  if (buf->length + new_len >= TCP_BUFFER_SIZE)
    {
      return false;
    }

  // We copy the new data right after the data that is already waiting
  memcpy (buf->data + buf->length, new_data, new_len);
  buf->length += new_len;

  // For safety, we make sure it's always a valid C string
  buf->data[buf->length] = '\0';

  return true;
}

int
tcp_buffer_extract_line (tcp_buffer_t *buf, char *out_msg, size_t max_out_len)
{
  // 1. We are looking for the end-of-line character '\n'
  char *newline = memchr (buf->data, '\n', buf->length);

  if (newline == NULL)
    {
      return 0; // No '\n' found, the message is not yet complete
    }

  // 2. Calculate the size of the complete message (without the '\n')
  size_t msg_len = newline - buf->data;

  // 3. Copy the extracted message into the output variable
  if (msg_len < max_out_len)
    {
      memcpy (out_msg, buf->data, msg_len);
      out_msg[msg_len] = '\0'; // Terminate the extracted string properly
    }

  // 4. Shift the remaining memory to the left
  size_t total_consumed = msg_len + 1; // The message + the '\n'
  size_t remaining = buf->length - total_consumed;

  if (remaining > 0)
    {
      // We use memmove and not memcpy, because the memory areas overlap.
      memmove (buf->data, buf->data + total_consumed, remaining);
    }

  // 5. Update the new buffer size
  buf->length = remaining;
  buf->data[buf->length] = '\0';

  return 1; // Message extracted successfully
}