#include "bitboard.h"
#include <stdio.h>

/**
 * @brief Initialize an empty bitboard with all bits set to 0.
 */
Bitboard
bitboard_create_empty (void)
{
  Bitboard bb;
  bb.low = 0;
  bb.high = 0;
  return bb;
}

/**
 * @brief Initialize a full bitboard with all bits set to 1.
 */
Bitboard
bitboard_create_full (void)
{
  Bitboard bb;
  // Bitwise NOT of 0 fills all bits with 1
  bb.low = ~0ULL;
  bb.high = ~0ULL;
  return bb;
}

/**
 * @brief Set a specific bit to 1 at the given index.
 *
 * For indices 0-63, operates on the low part.
 * For indices 64-127, operates on the high part (with index - 64 offset).
 */
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

/**
 * @brief Clear a specific bit to 0 at the given index.
 *
 * Creates an inverted mask and ANDs it to clear only that bit.
 */
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

/**
 * @brief Check if a specific bit is set to 1.
 *
 * Returns non-zero (true) if the bit is set, zero (false) otherwise.
 */
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

/**
 * @brief Toggle a specific bit (flip 0 to 1 or 1 to 0).
 */
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

/**
 * @brief Bitwise OR: combines two bitboards.
 *
 * Resulting bitboard has 1 where either input has a 1.
 */
Bitboard
bitboard_or (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low | b.low;
  res.high = a.high | b.high;
  return res;
}

/**
 * @brief Bitwise AND: keeps only common bits.
 *
 * Resulting bitboard has 1 only where both inputs have a 1.
 */
Bitboard
bitboard_and (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low & b.low;
  res.high = a.high & b.high;
  return res;
}

/**
 * @brief Bitwise XOR: keeps differing bits.
 *
 * Resulting bitboard has 1 where inputs differ.
 */
Bitboard
bitboard_xor (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low ^ b.low;
  res.high = a.high ^ b.high;
  return res;
}

/**
 * @brief Bitwise NOT: inverts all bits.
 */
Bitboard
bitboard_not (Bitboard a)
{
  Bitboard res;
  res.low = ~a.low;
  res.high = ~a.high;
  return res;
}

/**
 * @brief Check if two bitboards are equal.
 */
bool
bitboard_equals (Bitboard a, Bitboard b)
{
  return (a.low == b.low) && (a.high == b.high);
}

/**
 * @brief Check if bitboard is empty (all bits are 0).
 */
bool
bitboard_is_zero (Bitboard bb)
{
  return (bb.low == 0) && (bb.high == 0);
}

/**
 * @brief Shift all bits left by the specified amount (towards higher indices).
 *
 * Used to compute moves toward South/East.
 * Bits overflow from low to high when shift < 64.
 * For shift >= 64, all of low moves into high.
 */
Bitboard
bitboard_shift_left (Bitboard bb, uint8_t shift)
{
  Bitboard res = { 0, 0 };

  if (shift >= 128)
    {
      return res;
    }

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

/**
 * @brief Shift all bits right by the specified amount (towards lower indices).
 *
 * Used to compute moves toward North/West.
 * Bits overflow from high to low when shift < 64.
 * For shift >= 64, all of high moves into low.
 */
Bitboard
bitboard_shift_right (Bitboard bb, uint8_t shift)
{
  Bitboard res = { 0, 0 };

  if (shift >= 128)
    {
      return res;
    }

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

int
bitboard_count_bits (Bitboard bb)
{
  return __builtin_popcountll (bb.low) + __builtin_popcountll (bb.high);
}