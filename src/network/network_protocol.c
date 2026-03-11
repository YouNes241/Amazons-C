#include "network_protocol.h"
#include <stdio.h>
#include <string.h>

// Helper to convert a column (0-9) into letters ('a'-'j')static char
static char
ColToChar (int col)
{
  return (char)(MIN_COL_CHAR + col);
}

// Helper to convert a letter (MIN_COL_CHAR-'j') into a column (0-9)
static int
CharToCol (char c)
{
  return (int)(c - MIN_COL_CHAR);
}

void
Protocol_MoveToString (Move m, char *buffer_out, size_t buf_size)
{
  if (!buffer_out)
    return;

  // Final format: "MOVE a1-a4-b4\n"
  // Note: often in board games, row 0 is marked '1'.
  snprintf (buffer_out, buf_size, "MOVE %c%d-%c%d-%c%d\n",
            ColToChar (m.queen_src.col), m.queen_src.row + 1,
            ColToChar (m.queen_dst.col), m.queen_dst.row + 1,
            ColToChar (m.arrow_dst.col), m.arrow_dst.row + 1);
}

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