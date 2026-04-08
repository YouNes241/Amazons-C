/**
 * @file network_protocol.c
 * @brief Implementation of the network protocol for the Amazons game.
 *
 * This file contains functions to convert between Move structures and their
 * string representations according to the defined network protocol. It allows
 * for easy serialization and deserialization of moves when communicating over
 * the network.
 */

#include "network_protocol.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Converts a column index (0-9) into its letter representation (a-j)
 * @param col Column index (0 to 9)
 * @return Character representing the column (a-j)
 */
static char
ColToChar (int col)
{
  return (char)(MIN_COL_CHAR + col);
}

/**
 * @brief Converts a letter (a-j) into its column index (0-9)
 * @param c Character representing the column (a-j)
 * @return Column index (0 to 9)
 */
static int
CharToCol (char c)
{
  return (int)(c - MIN_COL_CHAR);
}

/**
 * @brief Converts a Move struct into its string representation according to
 * the protocol.
 *
 * The output format is "MOVE a1-a4-b4\n", where a1 is the source position of
 * the queen, a4 is the destination position of the queen, and b4 is the
 * destination position of the arrow. The function ensures that the output
 * buffer is not overflowed and is null-terminated.
 *
 * @param m The Move struct to convert.
 * @param buffer_out Output buffer to hold the resulting string.
 * @param buf_size Size of the output buffer in bytes.
 */
void
Protocol_MoveToString (Move m, char *buffer_out, size_t buf_size)
{
  if (!buffer_out)
    return;

  // Final format: "MOVE a1-a4-b4\n"
  // Note: In board games, row 0 is conventionally displayed as '1'
  snprintf (buffer_out, buf_size, "MOVE %c%d-%c%d-%c%d\n",
            ColToChar (m.queen_src.col), m.queen_src.row + 1,
            ColToChar (m.queen_dst.col), m.queen_dst.row + 1,
            ColToChar (m.arrow_dst.col), m.arrow_dst.row + 1);
}

/**
 * @brief Parses a move string in the format "MOVE a1-a4-b4" into a Move
 * struct.
 *
 * The function validates the input format and checks that the coordinates are
 * within the expected bounds. It returns true if parsing was successful and
 * false otherwise.
 *
 * @param message Input string to parse.
 * @param move_out Output Move struct to fill with the parsed data.
 * @return bool true if parsing was successful, false otherwise.
 */
bool
Protocol_StringToMove (const char *message, Move *move_out)
{
  if (!message || !move_out)
    return false;

  char c_src, c_dst, c_arr;
  int r_src, r_dst, r_arr;

  int parsed = sscanf (message, "MOVE %c%d-%c%d-%c%d", &c_src, &r_src, &c_dst,
                       &r_dst, &c_arr, &r_arr);

  if (parsed != EXPECTED_PARSED_FIELDS)
    return false;

  if (c_src < MIN_COL_CHAR || c_src > MAX_COL_CHAR)
    return false;
  if (c_dst < MIN_COL_CHAR || c_dst > MAX_COL_CHAR)
    return false;
  if (c_arr < MIN_COL_CHAR || c_arr > MAX_COL_CHAR)
    return false;

  if (r_src < MIN_ROW_VAL || r_src > MAX_ROW_VAL)
    return false;
  if (r_dst < MIN_ROW_VAL || r_dst > MAX_ROW_VAL)
    return false;
  if (r_arr < MIN_ROW_VAL || r_arr > MAX_ROW_VAL)
    return false;

  move_out->queen_src.col = CharToCol (c_src);
  move_out->queen_src.row = r_src - ROW_OFFSET;

  move_out->queen_dst.col = CharToCol (c_dst);
  move_out->queen_dst.row = r_dst - ROW_OFFSET;

  move_out->arrow_dst.col = CharToCol (c_arr);
  move_out->arrow_dst.row = r_arr - ROW_OFFSET;

  return true;
}
