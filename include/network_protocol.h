/* ==========================================================================
 * network_protocol.h
 * Header file for the network protocol used to communicate moves between the
 * client and server. This file defines the functions for converting Move
 * structures to and from their string representations as used in network
 * messages.
 * ==========================================================================
 */

#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include "game_types.h"
#include <stddef.h>

/* ==========================================================================
 * CONSTANTS
 * ==========================================================================
 */

#define MIN_COL_CHAR 'a'
#define MAX_COL_CHAR 'z'

#define MIN_ROW_VAL 1
#define MAX_ROW_VAL 26 // Theoretical limit based on the alphabet (a-z)

#define ROW_OFFSET                                                            \
  1 // Offset between the internal index (0) and the display (1)

#define EXPECTED_PARSED_FIELDS                                                \
  6 // Number of elements expected by sscanf for a valid move

/**
 * @brief Transforms a Move structure into a network message (e.g., "MOVE
 * a1-a4-b4\n")
 * @param m The move to translate.
 * @param buffer_out The output text (must be at least 32 characters).
 */
void Protocol_MoveToString (Move m, char *buffer_out, size_t buf_size);

/**
 * @brief Transforms a network message (e.g., "MOVE a1-a4-b4") into a Move
 * structure.
 * @param message The text received from the network.
 * @param move_out Pointer to the structure to be populated.
 * @return true if the message was correctly formatted, false otherwise.
 */
bool Protocol_StringToMove (const char *message, Move *move_out);

#endif // NETWORK_PROTOCOL_H