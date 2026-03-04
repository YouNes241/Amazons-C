#include "network_protocol.h"
#include <stdio.h>
#include <string.h>

// Helper to convert a column (0-9) into letters ('a'-'j')static char
static char
ColToChar (int col)
{
  return (char)('a' + col);
}

// Helper to convert a letter ('a'-'j') into a column (0-9)
static int
CharToCol (char c)
{
  return (int)(c - 'a');
}

void
Protocol_MoveToString (Move m, char *buffer_out)
{
  if (!buffer_out)
    return;

  // Final format: "MOVE a1-a4-b4\n"
  // Note: often in board games, row 0 is marked '1'.
  sprintf (buffer_out, "MOVE %c%d-%c%d-%c%d\n", ColToChar (m.queen_src.col),
           m.queen_src.row + 1, ColToChar (m.queen_dst.col),
           m.queen_dst.row + 1, ColToChar (m.arrow_dst.col),
           m.arrow_dst.row + 1);
}

bool
Protocol_StringToMove (const char *message, Move *move_out)
{
  if (!message || !move_out)
    return false;

  char c_src, c_dst, c_arr;
  int r_src, r_dst, r_arr;

  // sscanf reads the string and extracts the variables according to the given
  // format. It returns the number of variables found (we expect 6).
  int parsed = sscanf (message, "MOVE %c%d-%c%d-%c%d", &c_src, &r_src, &c_dst,
                       &r_dst, &c_arr, &r_arr);

  if (parsed == 6)
    {
      move_out->queen_src.col = CharToCol (c_src);
      move_out->queen_src.row = r_src - 1; // We reset index to 0

      move_out->queen_dst.col = CharToCol (c_dst);
      move_out->queen_dst.row = r_dst - 1;

      move_out->arrow_dst.col = CharToCol (c_arr);
      move_out->arrow_dst.row = r_arr - 1;

      return true;
    }

  return false;
}