#include "utils.h"
#include <stddef.h>
#include <sys/time.h>

unsigned long
get_time_ms (void)
{
  struct timeval tv;

  gettimeofday (&tv, NULL);
  return (((unsigned long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

Position
idx_to_pos (uint8_t index, uint8_t size)
{
  return (Position){ .row = index / size, .col = index % size };
}

bool
moves_are_equal (Move a, Move b)
{
  return (a.queen_src.row == b.queen_src.row
          && a.queen_src.col == b.queen_src.col
          && a.queen_dst.row == b.queen_dst.row
          && a.queen_dst.col == b.queen_dst.col
          && a.arrow_dst.row == b.arrow_dst.row
          && a.arrow_dst.col == b.arrow_dst.col);
}