#ifndef NETWORK_PACKET_H
#define NETWORK_PACKET_H

#include <stdbool.h>
#include <stddef.h>

/* ==========================================================================
 * ENUM: PACKET TYPES
 * ==========================================================================
 */
typedef enum
{
  PKT_UNKNOWN = 0,
  C2S_JOIN = 1,       // Client -> Server: Connection request with a nickname
  C2S_MOVE = 2,       // Client -> Server: Sends a played move
  S2C_GAME_START = 3, // Server -> Client: The game starts
  S2C_ERROR = 4,      // Server -> Client: Error message (e.g., invalid move)
  S2C_WELCOME = 5,    // Server -> Acceptance of nickname
  S2C_REJECT = 6      // Server ->  Username rejected (already taken, etc.)
} PacketType;

/* ==========================================================================
 * STRUCT: GENERIC PACKET
 * ==========================================================================
 */
#define MAX_PAYLOAD_SIZE 256

typedef struct
{
  PacketType type;                // The message type id
  char payload[MAX_PAYLOAD_SIZE]; // the message content in text
} Packet;

/* ==========================================================================
 * FUNCTIONS
 * ==========================================================================
 */

/**
 * @brief Transforms a Packet structure into a sendable string.
 * @return The number of bytes written, or -1 in case of an error.
 */
int SerializePacket (const Packet *pkt, char *buffer_out, size_t max_len);

/**
 * @brief Transforms a received string into a Packet structure.
 * @return true if parsing was successful, false if the format is invalid.
 */
bool DeserializePacket (const char *buffer_in, Packet *pkt_out);

#endif // NETWORK_PACKET_H