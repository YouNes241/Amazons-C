#include "network_includes/network_packet.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Serializes a packet to the format "TYPE|PAYLOAD\n"
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
 * @brief Deserializes a string like "TYPE|PAYLOAD" into a Packet struct.
 * @note The input string should NOT contain the '\n' wich is removed by
 * tcp_buffer_extract_line.
 */
bool
DeserializePacket (const char *buffer_in, Packet *pkt_out)
{
  if (!buffer_in || !pkt_out)
    return false;

  memset (pkt_out, 0, sizeof (Packet));

  // 1. Find the separator PACKET_SEPARATOR
  const char *separator = strchr (buffer_in, PACKET_SEPARATOR);
  if (!separator)
    {
      return false; // invalid format
    }

  // 2. Extract the type (the numbers before the PACKET_SEPARATOR)
  int type_val = 0;
  if (sscanf (buffer_in, TYPE_PARSE_FORMAT, &type_val) != EXPECTED_TYPE_TOKENS)
    {
      return false; // cant read int
    }
  if (type_val <= PKT_UNKNOWN || type_val >= PKT_TYPE_MAX)
    return false;
  pkt_out->type = (PacketType)type_val;

  // 3. Extract the payload (everything after the PACKET_SEPARATOR)
  const char *payload_start = separator + PAYLOAD_PTR_OFFSET;
  strncpy (pkt_out->payload, payload_start,
           MAX_PAYLOAD_SIZE - NULL_CHAR_SPACE);
  pkt_out->payload[MAX_PAYLOAD_SIZE - NULL_CHAR_SPACE] = '\0';

  return true;
}