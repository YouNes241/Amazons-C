/**
 * @file network_protocol.h
 * @brief Handles the translation between game structures (Move) and network
 * text (Protocol).
 */

#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include "../include/game_types.h"
#include <stdbool.h>
#include <stddef.h>
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