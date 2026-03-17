#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Represents the state of 128 squares (sufficient for a board up to
 * 11x11).
 *
 * The bitboard is split into two 64-bit parts:
 * - low  : indices 0 to 63
 * - high : indices 64 to 127
 */
typedef struct
{
  uint64_t low;
  uint64_t high;
} Bitboard;

/**
 * @brief Create an empty bitboard with all bits set to 0.
 * @return A new empty bitboard.
 */
Bitboard bitboard_create_empty (void);

/**
 * @brief Create a full bitboard with all bits set to 1.
 * @return A new full bitboard.
 */
Bitboard bitboard_create_full (void);

/**
 * @brief Set the bit at the given index to 1.
 * @param bb Pointer to the bitboard to modify.
 * @param index Bit index (0-127); indices 0-63 map to low, 64-127 map to high.
 */
void bitboard_set_bit (Bitboard *bb, uint8_t index);

/**
 * @brief Clear the bit at the given index to 0.
 * @param bb Pointer to the bitboard to modify.
 * @param index Bit index (0-127).
 */
void bitboard_clear_bit (Bitboard *bb, uint8_t index);

/**
 * @brief Check if a specific bit is set to 1.
 * @param bb The bitboard to check.
 * @param index Bit index (0-127).
 * @return True if the bit is set, false otherwise.
 */
bool bitboard_get_bit (Bitboard bb, uint8_t index);

/**
 * @brief Toggle a bit (flip 0 to 1 or 1 to 0).
 * @param bb Pointer to the bitboard to modify.
 * @param index Bit index (0-127).
 */
void bitboard_toggle_bit (Bitboard *bb, uint8_t index);

/**
 * @brief Bitwise OR: combine two bitboards.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return New bitboard with bits set where either input has a 1.
 */
Bitboard bitboard_or (Bitboard a, Bitboard b);

/**
 * @brief Bitwise AND: keep only common bits.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return New bitboard with bits set only where both inputs have a 1.
 */
Bitboard bitboard_and (Bitboard a, Bitboard b);

/**
 * @brief Bitwise XOR: keep differing bits.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return New bitboard with bits set where inputs differ.
 */
Bitboard bitboard_xor (Bitboard a, Bitboard b);

/**
 * @brief Bitwise NOT: invert all bits.
 * @param a The bitboard to invert.
 * @return New bitboard with all bits inverted.
 */
Bitboard bitboard_not (Bitboard a);

/**
 * @brief Applies NOT while ensuring the mask is applied
 * to prevent leakage
 * @param a The bitboard to invert
 * @param size The size of the bitboard
 * @return New bitboard with all bits inverted, while masking unused bits.
 */
Bitboard bitboard_masked_not (Bitboard a, uint8_t size);

/**
 * @brief Shift bits left (towards higher indices).
 * @param bb The bitboard to shift.
 * @param shift Number of positions to shift left.
 * @return New bitboard with bits shifted left.
 * @note Useful to compute moves toward South/East.
 */
Bitboard bitboard_shift_left (Bitboard bb, uint8_t shift);

/**
 * @brief Shift bits right (towards lower indices).
 * @param bb The bitboard to shift.
 * @param shift Number of positions to shift right.
 * @return New bitboard with bits shifted right.
 * @note Useful to compute moves toward North/West.
 */
Bitboard bitboard_shift_right (Bitboard bb, uint8_t shift);

/**
 * @brief Count the number of set bits (population count).
 * @param bb The bitboard to count.
 * @return Number of bits set to 1.
 */
int bitboard_count_bits (Bitboard bb);

/**
 * @brief Check if two bitboards are equal.
 * @param a First bitboard.
 * @param b Second bitboard.
 * @return True if both bitboards are identical, false otherwise.
 */
bool bitboard_equals (Bitboard a, Bitboard b);

/**
 * @brief Check if bitboard is empty (all bits are 0).
 * @param bb The bitboard to check.
 * @return True if all bits are zero, false otherwise.
 */
bool bitboard_is_zero (Bitboard bb);

/**
 * @brief Print the bitboard to the console.
 * @param bb The bitboard to print.
 * @param width Grid width for display (typically 10 for a 10x10 board). Must
 * be 4-11.
 */
void bitboard_print (Bitboard bb, uint8_t width);

// Get a bitboard of all reachable squares from a starting index.
Bitboard bitboard_get_reachable (uint8_t start_index, Bitboard occupied,
                                 uint8_t size);

void init_board_masks ();

Bitboard get_board_mask (uint8_t size);

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
#endif