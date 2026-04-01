/**
 * @file bitboard.h
 * @brief This file defines the Bitboard structure and associated functions for
 * representing and manipulating the game board state efficiently using bitwise
 * operations. The Bitboard allows for fast move generation, state evaluation,
 * and other operations critical to the performance of the AI.
 */
#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @struct Bitboard
 * @brief Represents the state of 128 squares (sufficient for a board up to
 * 11x11).
 * @var Bitboard::low
 * bitboard indices 0 to 63
 * @var Bitboard::high
 * bitboard indices 64 to 127
 */
typedef struct
{
  uint64_t low;
  uint64_t high;
} Bitboard;

// statics

/**
 * @brief Create an empty bitboard with all bits set to 0.
 * @return A new empty bitboard.
 */
static inline Bitboard
bitboard_create_empty (void)
{
  Bitboard res;
  res.low = 0;
  res.high = 0;
  return res;
}

/**
 * @brief Create a full bitboard with all bits set to 1.
 * @return A new full bitboard.
 */
static inline Bitboard
bitboard_create_full (void)
{
  Bitboard res;
  res.low = ~0ULL;
  res.high = ~0ULL;
  return res;
}

/**
 * @brief Set the bit at the given index to 1.
 * @param bb Pointer to the bitboard to modify.
 * @param index Bit index (0-127); indices 0-63 map to low, 64-127 map to high.
 */
static inline void
bitboard_set_bit (Bitboard *bb, uint8_t index)
{
  ((uint64_t *)bb)[index >> 6] |= (1ULL << (index & 63));
}

/**
 * @brief Clear the bit at the given index to 0.
 * @param bb Pointer to the bitboard to modify.
 * @param index Bit index (0-127).
 */
static inline void
bitboard_clear_bit (Bitboard *bb, uint8_t index)
{
  ((uint64_t *)bb)[index >> 6] &= ~(1ULL << (index & 63));
}

/**
 * @brief Check if a specific bit is set to 1.
 * @param bb The bitboard to check.
 * @param index Bit index (0-127).
 * @return True if the bit is set, false otherwise.
 */
static inline bool
bitboard_get_bit (Bitboard bb, uint8_t index)
{
  return (((uint64_t *)&bb)[index >> 6] & (1ULL << (index & 63))) != 0;
}

/**
 * @brief Toggle a bit (flip 0 to 1 or 1 to 0) (branchless).
 * @param bb Pointer to the bitboard to modify.
 * @param index Bit index (0-127).
 */
static inline void
bitboard_toggle_bit (Bitboard *bb, uint8_t index)
{
  ((uint64_t *)bb)[index >> 6] ^= (1ULL << (index & 63));
}

/**
 * @brief Bitwise OR: combine two bitboards.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return New bitboard with bits set where either input has a 1.
 */
static inline Bitboard
bitboard_or (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low | b.low;
  res.high = a.high | b.high;
  return res;
}

/**
 * @brief Bitwise AND: keep only common bits.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return New bitboard with bits set only where both inputs have a 1.
 */
static inline Bitboard
bitboard_and (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low & b.low;
  res.high = a.high & b.high;
  return res;
}

/**
 * @brief Bitwise XOR: keep differing bits.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return New bitboard with bits set where inputs differ.
 */
static inline Bitboard
bitboard_xor (Bitboard a, Bitboard b)
{
  Bitboard res;
  res.low = a.low ^ b.low;
  res.high = a.high ^ b.high;
  return res;
}

/**
 * @brief Bitwise NOT: invert all bits.
 * @param a The bitboard to invert.
 * @return New bitboard with all bits inverted.
 */
static inline Bitboard
bitboard_not (Bitboard a)
{
  Bitboard res;
  res.low = ~a.low;
  res.high = ~a.high;
  return res;
}

/**
 * @brief Check if two bitboards are equal.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return True if both bitboards are identical, false otherwise.
 */
static inline bool
bitboard_equals (Bitboard a, Bitboard b)
{
  return (a.low == b.low) && (a.high == b.high);
}

/**
 * @brief Check if bitboard is empty (all bits are 0).
 * @param bb The bitboard to check.
 * @return True if all bits are zero, false otherwise.
 */
static inline bool
bitboard_is_zero (Bitboard bb)
{
  return (bb.low == 0) && (bb.high == 0);
}

/**
 * @brief Count the number of set bits (population count).
 * @param bb The bitboard to count.
 * @return Number of bits set to 1.
 */
static inline int
bitboard_count_bits (Bitboard bb)
{
  return __builtin_popcountll (bb.low) + __builtin_popcountll (bb.high);
}

/**
 * @brief Shift bits left (towards higher indices).
 * @param bb The bitboard to shift.
 * @param shift Number of positions to shift left.
 * @return New bitboard with bits shifted left.
 * @note Useful to compute moves toward South/East.
 */
static inline Bitboard
bitboard_shift_left (Bitboard bb, uint8_t shift)
{
  Bitboard res = { 0, 0 }; // Initialize to zero

  if (shift == 0)
    {
      return bb;
    }

  if (shift < 64)
    {
      res.low = bb.low << shift;
      res.high = (bb.high << shift) | (bb.low >> (64 - shift));
    }
  else if (shift < 128)
    {
      res.low = 0;
      res.high = bb.low << (shift - 64);
    }

  return res;
}

/**
 * @brief Shift bits right (towards lower indices).
 * @param bb The bitboard to shift.
 * @param shift Number of positions to shift right.
 * @return New bitboard with bits shifted right.
 * @note Useful to compute moves toward North/West.
 */
static inline Bitboard
bitboard_shift_right (Bitboard bb, int shift)
{
  Bitboard res;
  if (shift >= 64)
    {
      res.low = bb.high >> (shift - 64);
      res.high = 0;
    }
  else if (shift > 0)
    {
      res.low = (bb.low >> shift) | (bb.high << (64 - shift));
      res.high = bb.high >> shift;
    }
  else
    {
      res = bb;
    }
  return res;
}

/**
 * @brief Pops the least significant bit (rightmost 1) from the bitboard
 * given and returns the removed index.
 * @param bb Pointer to the Bitboard (modified in-place).
 * @return Index (0-127) of the popped bit, or -1 if the bitboard is empty.
 */
static inline int
pop_lsb (Bitboard *bb)
{
  if (bb->low != 0)
    {
      int idx = __builtin_ctzll (bb->low);
      bb->low &= (bb->low - 1);
      return idx;
    }
  else if (bb->high != 0)
    {
      int idx = __builtin_ctzll (bb->high);
      bb->high &= (bb->high - 1);
      return idx + 64;
    }
  return -1;
}

static inline Bitboard
kogge_stone_left (Bitboard sources, Bitboard empty, Bitboard boundary,
                  uint8_t shift)
{
  Bitboard flood = sources;
  Bitboard prop = bitboard_and (empty, boundary);

  // Step 1: 1x shift
  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_left (flood, shift)));
  prop = bitboard_and (prop, bitboard_shift_left (prop, shift));

  // Step 2: 2x shift
  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_left (flood, 2 * shift)));
  prop = bitboard_and (prop, bitboard_shift_left (prop, 2 * shift));

  // Step 3: 4x shift
  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_left (flood, 4 * shift)));
  prop = bitboard_and (prop, bitboard_shift_left (prop, 4 * shift));

  // Step 4: 8x shift (Covers up to 15 squares total, max board width is 11)
  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_left (flood, 8 * shift)));

  // Return only the generated attacks, excluding the starting sources
  return bitboard_xor (flood, sources);
}

static inline Bitboard
kogge_stone_right (Bitboard sources, Bitboard empty, Bitboard boundary,
                   uint8_t shift)
{
  Bitboard flood = sources;
  Bitboard prop = bitboard_and (empty, boundary);

  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_right (flood, shift)));
  prop = bitboard_and (prop, bitboard_shift_right (prop, shift));

  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_right (flood, 2 * shift)));
  prop = bitboard_and (prop, bitboard_shift_right (prop, 2 * shift));

  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_right (flood, 4 * shift)));
  prop = bitboard_and (prop, bitboard_shift_right (prop, 4 * shift));

  flood = bitboard_or (
      flood, bitboard_and (prop, bitboard_shift_right (flood, 8 * shift)));

  return bitboard_xor (flood, sources);
}

// others

void init_board_masks ();
Bitboard get_board_mask (uint8_t size);

/**
 * @brief Applies NOT while ensuring the mask is applied to prevent leakage.
 */
static inline Bitboard
bitboard_masked_not (Bitboard a, uint8_t size)
{
  return bitboard_and (bitboard_not (a), get_board_mask (size));
}

Bitboard expand_queen_slide (Bitboard sources, Bitboard empty, uint8_t width);

Bitboard expand_king_step (Bitboard sources, Bitboard empty, uint8_t width);

/**
 * @brief Print the bitboard to the console.
 * @param bb The bitboard to print.
 * @param width Grid width for display (typically 10 for a 10x10 board). Must
 * be 4-11.
 */
void bitboard_print (Bitboard bb, uint8_t width);

/**
 * @brief Get a bitboard of all reachable squares from a starting index, by
 * expanding onto the empty available squares.
 * @param start_index Starting index of movement
 * @param occupied All occupied squares to be taken into account
 * @param size The size of the board
 */
static inline Bitboard
bitboard_get_reachable (uint8_t start_index, Bitboard occupied, uint8_t size)
{
  Bitboard start_bb = bitboard_create_empty ();
  bitboard_set_bit (&start_bb, start_index);

  Bitboard empty = bitboard_masked_not (occupied, size);

  return expand_queen_slide (start_bb, empty, size);
}
#endif // BITBOARD_H