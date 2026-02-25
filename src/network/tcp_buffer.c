/**
 * @file tcp_buffer.c
 * @brief Implementation of a TCP buffer for handling fragmented network data.
 */

#include "tcp_buffer.h"
#include <string.h>

/**
 * @brief Initializes a TCP buffer structure.
 *
 * @details Zeros out the data array and resets the length counter.
 * This should be called before using the buffer for the first time.
 *
 * @param buf Pointer to the tcp_buffer_t structure to initialize.
 */
void
tcp_buffer_init (tcp_buffer_t *buf)
{
  buf->length = 0;
  memset (buf->data, 0, TCP_BUFFER_SIZE);
}

/**
 * @brief Appends new data to the end of the buffer.
 *
 * @details Checks for buffer overflow before appending. If the data fits,
 * it is copied to the end of the existing data, and the buffer is
 * null-terminated for safety.
 *
 * @param buf Pointer to the TCP buffer.
 * @param new_data Pointer to the data to append.
 * @param new_len Length of the data to append in bytes.
 * @return bool true if data was successfully appended, false if the buffer is
 * full.
 */
bool
tcp_buffer_append (tcp_buffer_t *buf, const char *new_data, size_t new_len)
{
  if (buf->length + new_len >= TCP_BUFFER_SIZE)
    {
      return false;
    }

  memcpy (buf->data + buf->length, new_data, new_len);
  buf->length += new_len;

  buf->data[buf->length] = '\0';

  return true;
}

/**
 * @brief Extracts a complete line (message) ending with a newline character.
 *
 * @details Scans the buffer for a newline ('\\n'). If found, extracts the
 * message into the output buffer, removes the message and newline from the
 * internal buffer, and shifts the remaining data to the beginning (using
 * memmove).
 *
 * @param buf Pointer to the TCP buffer.
 * @param out_msg Buffer where the extracted message will be stored.
 * @param max_out_len Maximum size of the output buffer.
 * @return int 1 if a message was extracted, 0 if no complete message (newline)
 * was found.
 */
int
tcp_buffer_extract_line (tcp_buffer_t *buf, char *out_msg, size_t max_out_len)
{
  char *newline = memchr (buf->data, '\n', buf->length);

  if (newline == NULL)
    {
      return 0;
    }

  size_t msg_len = newline - buf->data;

  if (msg_len < max_out_len)
    {
      memcpy (out_msg, buf->data, msg_len);
      out_msg[msg_len] = '\0';
    }

  size_t total_consumed = msg_len + 1;
  size_t remaining = buf->length - total_consumed;

  if (remaining > 0)
    {
      memmove (buf->data, buf->data + total_consumed, remaining);
    }

  buf->length = remaining;
  buf->data[buf->length] = '\0';

  return 1;
}