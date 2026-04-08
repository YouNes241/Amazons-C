#include "utils.h"
#include <stdint.h>
#include <sys/time.h>

static uint64_t s[4];

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

void
pos_to_string (Position pos, char *out, size_t out_size)
{
  snprintf (out, out_size, "%c%d", 'a' + pos.col, pos.row + 1);
}

static inline uint64_t
rotl (const uint64_t x, int k)
{
  return (x << k) | (x >> (64 - k));
}

uint64_t
next_xoshiro (void)
{
  const uint64_t result = rotl (s[1] * 5, 7) * 9;
  const uint64_t t = s[1] << 17;
  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];
  s[2] ^= t;
  s[3] = rotl (s[3], 45);
  return result;
}

void
seed_xoshiro (uint64_t seed)
{
  uint64_t z = (seed + 0x9E3779B97F4A7C15);
  for (int i = 0; i < 4; i++)
    {
      z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
      s[i] = z ^ (z >> 31);
      z += 0x9E3779B97F4A7C15;
    }
}

uint64_t
random_uint (uint64_t limit)
{
  if (limit == 0)
    return 0;
  return next_xoshiro () % limit;
}
