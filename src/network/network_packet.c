#include "network_packet.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Serializes a packet to the format "TYPE|PAYLOAD\n"
 */
int
SerializePacket (const Packet *pkt, char *buffer_out, size_t max_len)
{
  if (!pkt || !buffer_out || max_len == 0)
    return -1;

  int written
      = snprintf (buffer_out, max_len, "%d|%s\n", pkt->type, pkt->payload);

  // Formatting error or output buffer too small
  if (written < 0 || (size_t)written >= max_len)
    {
      return -1;
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

  // 1. Find the separator '|'
  const char *separator = strchr (buffer_in, '|');
  if (!separator)
    {
      return false; // invalid format
    }

  // 2. Extract the type (the numbers before the '|')
  int type_val = 0;
  if (sscanf (buffer_in, "%d", &type_val) != 1)
    {
      return false; // cant read int
    }
  pkt_out->type = (PacketType)type_val;

  // 3. Extract the payload (everything after the '|')
  const char *payload_start = separator + 1;
  strncpy (pkt_out->payload, payload_start, MAX_PAYLOAD_SIZE - 1);
  pkt_out->payload[MAX_PAYLOAD_SIZE - 1] = '\0';

  return true;
}