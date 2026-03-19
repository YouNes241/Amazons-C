/**
 * @file network_packet.h
 * @brief Defines the structure and functions for serializing and deserializing
 * network packets used in communication between the Server and Client.
 *
 * This header defines the Packet structure, which encapsulates the type of
 * message being sent and its associated payload. It also provides functions to
 * convert between the Packet structure and a string format suitable for
 * network transmission, as well as parsing received strings back into Packet
 * structures.
 *
 * The packet format is designed to be simple and efficient, using a delimiter
 * to separate the message type from its payload. This allows for easy parsing
 * and construction of messages while maintaining flexibility for different
 * types of communication (e.g., game moves, invitations, status updates).
 */

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
  /** @brief Unknown packet type. */
  PKT_UNKNOWN = 0,
  /** @brief Client -> Server: Connection request with a nickname. */
  C2S_JOIN = 1,
  /** @brief Client -> Server: Sends a played move. */
  C2S_MOVE = 2,
  /** @brief Client A wants to invite Client B. */
  C2S_INVITE = 3,
  /** @brief Client B accepts. */
  C2S_ACCEPT = 4,
  /** @brief Client B refuses. */
  C2S_DECLINE = 5,
  /** @brief Client -> Server: Requests the list of players. */
  C2S_LIST = 6,
  /** @brief Server -> Client: The game starts. */
  S2C_GAME_START = 7,
  /** @brief Server -> Client: Error message (e.g., invalid move). */
  S2C_ERROR = 8,
  /** @brief Server -> Client: Acceptance of nickname. */
  S2C_WELCOME = 9,
  /** @brief Server -> Client: Username rejected (already taken, etc.). */
  S2C_REJECT = 10,
  /** @brief Server -> Client: Sent when a move is received. */
  S2C_MOVE = 11,
  /** @brief Server -> Client: Inform the invitee that someone has invited
     them. */
  S2C_INVITE = 12,
  /** @brief Server -> Client: Inform the challenger that their invite was
     refused. */
  S2C_DECLINE = 13,
  /** @brief Server -> Client: Sends the list of players. */
  S2C_LIST = 14,
  /** @brief Server -> Client: Timeout message. */

  PKT_TIMEOUT = 15,
  /** @brief Server -> Client: Inform the client that their opponent has left.
   */
  S2C_OPPONENT_LEFT = 16,
  /** @brief Client -> Server: Cancel an invite. */
  C2S_CANCEL = 17,
  /** @brief Server -> Client: Inform the invitee that the challenger has
     canceled. */
  S2C_CANCEL = 18,
  /** @brief Client -> Server: Set status to away. */
  C2S_AWAY = 19,
  /** @brief Client -> Server: Set status to back. */
  C2S_BACK = 20,
  /** @brief Client -> Server: Send a ping message. */
  C2S_PING = 21,
  /** @brief Server -> Client: Send a pong message. */
  S2C_PONG = 22,
  /** @brief Client -> Server: Request the current scoreboard. */
  C2S_SCOREBOARD = 23,
  /** @brief Server -> Client: Send the current scoreboard. */
  S2C_SCOREBOARD = 24,
  /** @brief Client -> Server: Inform the server that the game is over. */
  C2S_GAME_OVER = 25,
  /** @brief Maximum value for packet types (used in validation). */
  PKT_TYPE_MAX = 26
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

/** @brief Structure representing a network packet. */
typedef struct
{
  /** @brief The type of the packet. */
  PacketType type;
  /** @brief The payload of the packet. */
  char payload[MAX_PAYLOAD_SIZE];
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