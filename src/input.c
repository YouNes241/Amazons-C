#include "input.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Transforms a string "d4-d7(g10)" into a Move structure.
 * @return 0 on success, -1 if the format is invalid.
 */
int
parse_coordinates (const char *input, Move *move)
{
  char c_src, c_dst, c_arr; // To store columns
  int r_src, r_dst, r_arr;  // To store rows

  /* sscanf reads the string according to a format. */
  int result = sscanf (input, " %c%d - %c%d ( %c%d )", &c_src, &r_src, &c_dst,
                       &r_dst, &c_arr, &r_arr);

  // Invalid input when there are not exactly 6 elements
  if (result != 6)
    {
      return -1;
    }

  /* Conversion des lettres en indices de colonnes */
  int col_src = (c_src >= 'a' && c_src <= 'z')   ? c_src - 'a'
                : (c_src >= 'A' && c_src <= 'Z') ? c_src - 'A'
                                                 : -1;

  int col_dst = (c_dst >= 'a' && c_dst <= 'z')   ? c_dst - 'a'
                : (c_dst >= 'A' && c_dst <= 'Z') ? c_dst - 'A'
                                                 : -1;

  int col_arr = (c_arr >= 'a' && c_arr <= 'z')   ? c_arr - 'a'
                : (c_arr >= 'A' && c_arr <= 'Z') ? c_arr - 'A'
                                                 : -1;

  if (col_src == -1 || col_dst == -1 || col_arr == -1)
    {
      return -1;
    }

  /* Fill the Move structure with converted coordinates.
   * Note: rows are zero-indexed in the structure, hence the -1.
   */
  move->queen_src.col = (uint8_t)col_src;
  move->queen_src.row = (uint8_t)(r_src - 1);

  move->queen_dst.col = (uint8_t)col_dst;
  move->queen_dst.row = (uint8_t)(r_dst - 1);

  move->arrow_dst.col = (uint8_t)col_arr;
  move->arrow_dst.row = (uint8_t)(r_arr - 1);

  return 0;
}