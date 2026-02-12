#include "bitboard.h"
#include <stdio.h>

Bitboard
bitboard_create_empty (void)
{
  Bitboard bb;
  bb.low = 0;
  bb.high = 0;
  return bb;
}

Bitboard
bitboard_create_full (void)
{
  Bitboard bb;
  // Bitwise NOT of 0 fills all bits with 1
  bb.low = ~0ULL;
  bb.high = ~0ULL;
  return bb;
}

void
bitboard_set_bit (Bitboard *bb, uint8_t index)
{
  if (index < 64)
    {
      // Operating on low part: create a mask with a single 1 at position
      // index, then OR it
      bb->low |= (1ULL << index);
    }
  else
    {
      // Operating on high part: adjust index by subtracting 64 to get position
      // within high
      bb->high |= (1ULL << (index - 64));
    }
}

void
bitboard_clear_bit (Bitboard *bb, uint8_t index)
{
  if (index < 64)
    {
      // Create a mask with a single 0 at position index, AND it to clear only
      // that bit
      bb->low &= ~(1ULL << index);
    }
  else
    {
      // Same logic applied to high part
      bb->high &= ~(1ULL << (index - 64));
    }
}

bool
bitboard_get_bit (Bitboard bb, uint8_t index)
{
  if (index < 64)
    {
      // AND with a single-1 mask; non-zero result means bit is set
      return (bb.low & (1ULL << index)) != 0;
    }
  else
    {
      // Same logic for high part
      return (bb.high & (1ULL << (index - 64))) != 0;
    }
}

void
bitboard_toggle_bit (Bitboard *bb, uint8_t index)
{
  if (index < 64)
    {
      // XOR with a single-1 mask flips only that bit
      bb->low ^= (1ULL << index);
    }
  else
    {
      // Same logic for high part
      bb->high ^= (1ULL << (index - 64));
    }
}

Bitboard
bitboard_or (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low | b.low;
  res.high = a.high | b.high;
  return res;
}

Bitboard
bitboard_and (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low & b.low;
  res.high = a.high & b.high;
  return res;
}

Bitboard
bitboard_xor (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low ^ b.low;
  res.high = a.high ^ b.high;
  return res;
}

Bitboard
bitboard_not (Bitboard a)
{
  Bitboard res;
  res.low = ~a.low;
  res.high = ~a.high;
  return res;
}

bool
bitboard_equals (Bitboard a, Bitboard b)
{
  return (a.low == b.low) && (a.high == b.high);
}

bool
bitboard_is_zero (Bitboard bb)
{
  return (bb.low == 0) && (bb.high == 0);
}

Bitboard
bitboard_shift_left (Bitboard bb, uint8_t shift)
{
  Bitboard res = { 0, 0 };

  // If shift is 0, no operation needed
  if (shift == 0)
    return bb;

  if (shift >= 64)
    {
      // Extreme case: shift is large enough that all of 'low' moves into
      // 'high' 'low' becomes empty (0)
      res.low = 0;
      res.high = bb.low << (shift - 64);
    }
  else
    {
      // Shift all bits; bits from low overflow into high

      // High receives its own shifted bits plus bits overflowing from low

      res.high = (bb.high << shift) | (bb.low >> (64 - shift));

      // Low is simply left-shifted; new bits on right side are 0
      res.low = bb.low << shift;
    }
  return res;
}

Bitboard
bitboard_shift_right (Bitboard bb, uint8_t shift)
{
  Bitboard res = { 0, 0 };

  // If shift is 0, no operation needed
  if (shift == 0)
    return bb;

  if (shift >= 64)
    {
      // Extreme case: shift is large enough that all of 'high' moves into
      // 'low' 'high' becomes empty (0)
      res.low = bb.high >> (shift - 64);
      res.high = 0;
    }
  else
    {
      // Low receives its own shifted bits plus bits overflowing from high
      res.low = (bb.low >> shift) | (bb.high << (64 - shift));
      // High is simply right-shifted; new bits on left side are 0
      res.high = bb.high >> shift;
    }
  return res;
}

void
bitboard_print (Bitboard bb, uint8_t width)
{
  if (width < 4 || width > 11)
    width = 10;

  printf ("\n   ");
  for (int col = 0; col < width; col++)
    printf ("%d ", col);
  printf ("\n");

  for (int row = 0; row < width; row++)
    {
      printf ("%d: ", row);
      for (int col = 0; col < width; col++)
        {
          // Calculate the bit index for the current cell (row, col)
          uint8_t index = row * width + col;

          if (bitboard_get_bit (bb, index))
            {
              printf ("X "); // Occupied
            }
          else
            {
              printf (". "); // Empty
            }
        }
      printf ("\n");
    }
  printf ("\n");
}