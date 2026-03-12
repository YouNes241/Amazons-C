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
  C2S_INVITE = 3,     // Client A wants to invite Client B
  C2S_ACCEPT = 4,     // Client B accepts
  C2S_DECLINE = 5,    // Client B refuses
  C2S_LIST = 6,       // Client -> Server: Requests the list of players
  S2C_GAME_START = 7, // Server -> Client: The game starts
  S2C_ERROR = 8,      // Server -> Client: Error message (e.g., invalid move)
  S2C_WELCOME = 9,    // Server -> Acceptance of nickname
  S2C_REJECT = 10,    // Server ->  Username rejected (already taken, etc.)
  S2C_MOVE = 11,
  S2C_INVITE = 12,  // The server tells Client B, "A invites you."
  S2C_DECLINE = 13, // The server tells Client A "B refused"
  S2C_LIST = 14,    // Server -> Client: Sends the list of players
  PKT_TIMEOUT = 15,
  PKT_TYPE_MAX = 16,
} PacketType;

/* ==========================================================================
 * CONSTANTS
 * ==========================================================================
 */

#define MAX_PAYLOAD_SIZE 256

#define SERIALIZE_ERROR -1 // Return code in case of serialization failure
#define EMPTY_LEN 0        // Zero size for validation

#define PACKET_SEPARATOR '|' // Character delimiting the PAYLOAD and the TYPE
#define PACKET_FORMAT "%d|%s\n" // Output format: "TYPE|PAYLOAD\n"
#define TYPE_PARSE_FORMAT "%d"  // Format for reading the package type
#define PAYLOAD_PTR_OFFSET 1    // Offset to point just after the separator
#define EXPECTED_TYPE_TOKENS 1  // Number of elements that sscanf should find
#define NULL_CHAR_SPACE                                                       \
  1 // Reserved space for the end-of-string character '\0'

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