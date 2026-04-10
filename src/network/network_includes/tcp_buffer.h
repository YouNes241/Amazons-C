/**
 * @file tcp_buffer.h
 * @brief Header for TCP buffer management in the network module.
 *
 * This header defines the structure and functions for managing a TCP buffer
 * used to accumulate incoming data from a socket. It includes functionality to
 * initialize the buffer, append new data, and extract complete messages based
 * on newline delimiters.
 */

#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

/* ==========================================================================
 * CONSTANTS
 * ==========================================================================
 */

/** @brief Length of an empty buffer.
 * @details This value represents the size of a buffer that contains no data.
 */
#define EMPTY_BUFFER_LENGTH 0

// Constants related to the message end character

/** @brief The newline character used to terminate messages.
 * @details This character is used to delimit the end of a message in the
 * buffer.
 */
#define NEWLINE_CHAR '\n'

/** @brief Length of the newline character.
 * @details This value represents the number of bytes occupied by the newline
 * character.
 */
#define NEWLINE_CHAR_LEN 1 // Space occupied by the character '\n'

// Return codes for line extraction
/** @brief Return code indicating successful line extraction.
 * @details This value is returned by tcp_buffer_extract_line when a complete
 * line is found.
 */
#define EXTRACT_SUCCESS 1
/** @brief Return code indicating no complete message was found.
 * @details This value is returned by tcp_buffer_extract_line when no newline
 * character is found.
 */
#define EXTRACT_NO_MSG 0
/** @brief Return code indicating message overflow.
 * @details This value is returned by tcp_buffer_extract_line when the
 * extracted message exceeds the output buffer size.
 */
#define EXTRACT_MSG_OVERFLOW -1

/**
 * @def TCP_BUFFER_SIZE
 * @brief The maximum size of the TCP buffer in bytes.
 *
 * @details Defined as 4096 bytes (4 KB), which is approximately 2.5 times
 * the standard Ethernet MTU (Maximum Transmission Unit) of 1500 bytes.
 * This ensures enough space to hold multiple packets before processing.
 */
#define TCP_BUFFER_SIZE 4096

/**
 * @struct tcp_buffer_t
 * @brief Structure representing a TCP stream buffer.
 */
typedef struct
{
  char data[TCP_BUFFER_SIZE]; // array to store incoming data.
  size_t length; // Current number of valid bytes stored in the buffer.
} tcp_buffer_t;

/**
 * @brief Initializes the TCP buffer.
 *
 * @details Sets the buffer length to 0 and zeroes out the memory.
 * This must be called before using a new `tcp_buffer_t` instance.
 *
 * @param buf Pointer to the tcp_buffer_t structure to initialize.
 */
void tcp_buffer_init (tcp_buffer_t *buf);

/**
 * @brief Appends new network data to the buffer.
 *
 * @details Copies `new_len` bytes from `new_data` to the end of the buffer.
 * Includes a safety check to prevent buffer overflow.
 *
 * @param buf Pointer to the TCP buffer.
 * @param new_data Pointer to the source data (received from socket).
 * @param new_len Length of the new data in bytes.
 * @return bool true if data was appended successfully, false if the buffer is
 * full.
 */
bool tcp_buffer_append (tcp_buffer_t *buf, const char *new_data,
                        size_t new_len);

/**
 * @brief Extracts a complete line (command) from the buffer.
 *
 * @details Scans for a newline character ('\\n'). If found, the line is copied
 * to `out_msg`, and the remaining data in the buffer is shifted to the
 * beginning.
 *
 * @param buf Pointer to the TCP buffer.
 * @param out_msg Buffer where the extracted message will be copied.
 * @param max_out_len Maximum size of the `out_msg` buffer.
 * @return int 1 if a message was extracted, 0 if no complete message was
 * found.
 */
int tcp_buffer_extract_line (tcp_buffer_t *buf, char *out_msg,
                             size_t max_out_len);

#endif // TCP_BUFFER_H
