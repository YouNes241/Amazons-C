#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

// We are allocating 4 KB per client, which is 2.5 times the MTU (Maximum
// Transmission Unit) of 1500 bytes.
#define TCP_BUFFER_SIZE 4096

typedef struct
{
  char data[TCP_BUFFER_SIZE];
  size_t length; // How many bytes are currently stored
} tcp_buffer_t;

// 1. Initializes the buffer to zero
void tcp_buffer_init (tcp_buffer_t *buf);

// 2. Adds the new data received by the network
// Returns false if the buffer is full (anti-crash safety)
bool tcp_buffer_append (tcp_buffer_t *buf, const char *new_data,
                        size_t new_len);

// 3. Looking for a complete command (terminated by \n).
// If found, copy it to out_msg, shift the memory, and return 1.
// Otherwise, it returns 0 (more data is needed).
int tcp_buffer_extract_line (tcp_buffer_t *buf, char *out_msg,
                             size_t max_out_len);

#endif // TCP_BUFFER_H