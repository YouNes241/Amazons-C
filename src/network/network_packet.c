/**
 * @file network_packet.c
 * @brief Implementation of packet serialization and deserialization.
 *
 * This file contains the logic for converting Packet structs to and from their
 * string representation for network transmission. The serialization format is
 * "TYPE|PAYLOAD\n", where TYPE is the integer value of the PacketType enum,
 * and PAYLOAD is the string content of the packet.
 */

#include "network_includes/network_packet.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Serializes a Packet struct into a string format suitable for network
 * transmission.
 *
 * The output format is "TYPE|PAYLOAD\n". The function checks for buffer
 * overflow and returns an error code if the output buffer is too small.
 *
 * @param pkt Pointer to the Packet to serialize.
 * @param buffer_out Output buffer to hold the serialized string.
 * @param max_len Maximum length of the output buffer.
 * @return int Number of bytes written to the output buffer, or a negative
 * value on error (e.g., if the buffer is too small).
 */
int
SerializePacket (const Packet *pkt, char *buffer_out, size_t max_len)
{
  if (!pkt || !buffer_out || max_len == EMPTY_LEN)
    return SERIALIZE_ERROR;

  int written
      = snprintf (buffer_out, max_len, PACKET_FORMAT, pkt->type, pkt->payload);

  // Formatting error or output buffer too small
  if (written < 0 || (size_t)written >= max_len)
    {
      return SERIALIZE_ERROR;
    }

  return written;
}

/**
 * @brief Deserializes a string into a Packet struct.
 *
 * The input string should be in the format "TYPE|PAYLOAD\n". The function
 * extracts the type and payload, validates them, and fills the output Packet
 * struct. It returns true on success and false on failure (e.g., invalid
 * format, unknown type).
 *
 * @param buffer_in Input string to deserialize.
 * @param pkt_out Output Packet struct to fill with the deserialized data.
 * @return bool true if deserialization was successful, false otherwise.
 */
bool
DeserializePacket (const char *buffer_in, Packet *pkt_out)
{
  if (!buffer_in || !pkt_out)
    return false;

  memset (pkt_out, 0, sizeof (Packet));

  // Step 1: Find the separator character
  const char *separator = strchr (buffer_in, PACKET_SEPARATOR);
  if (!separator)
    {
      return false; ///< Invalid format without separator
    }

  // Step 2: Extract the packet type value before the separator
  int type_val = 0;
  if (sscanf (buffer_in, TYPE_PARSE_FORMAT, &type_val) != EXPECTED_TYPE_TOKENS)
    {
      return false; ///< Cannot parse type value
    }
  if (type_val <= PKT_UNKNOWN || type_val >= PKT_TYPE_MAX)
    return false;
  pkt_out->type = (PacketType)type_val;

  // Step 3: Extract the payload after the separator
  const char *payload_start = separator + PAYLOAD_PTR_OFFSET;
  strncpy (pkt_out->payload, payload_start,
           MAX_PAYLOAD_SIZE - NULL_CHAR_SPACE);
  pkt_out->payload[MAX_PAYLOAD_SIZE - NULL_CHAR_SPACE] = '\0';

  return true;
}
