#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Represents the state of 128 squares (sufficient for a board up to 11x11).
 * - low  : indices 0 to 63
 * - high : indices 64 to 127
 */
typedef struct
{
  uint64_t low;
  uint64_t high;
} Bitboard;

// Create an empty bitboard (all zeros)
Bitboard bitboard_create_empty (void);

// Create a full bitboard (all ones)
Bitboard bitboard_create_full (void);

// Set the bit at the given index (0-127) to 1
void bitboard_set_bit (Bitboard *bb, uint8_t index);

// Clear the bit at the given index
void bitboard_clear_bit (Bitboard *bb, uint8_t index);

// Returns true if the bit is set (1)
bool bitboard_get_bit (Bitboard bb, uint8_t index);

// Toggle the bit (0 becomes 1, 1 becomes 0)
void bitboard_toggle_bit (Bitboard *bb, uint8_t index);

// Union : combine two bitboards
Bitboard bitboard_or (Bitboard a, Bitboard b);

// Intersection : keep the common bits
Bitboard bitboard_and (Bitboard a, Bitboard b);

// A XOR B : keep the differing bits
Bitboard bitboard_xor (Bitboard a, Bitboard b);

// Negation : invert all bits
Bitboard bitboard_not (Bitboard a);

// Shift bits left (towards higher indices)
// Useful to compute moves toward South/East
Bitboard bitboard_shift_left (Bitboard bb, uint8_t shift);

// Shift bits right (towards lower indices)
// Useful to compute moves toward North/West
Bitboard bitboard_shift_right (Bitboard bb, uint8_t shift);

// Count the number of set bits
int bitboard_count_bits (Bitboard bb);

// Check if two bitboards are identical
bool bitboard_equals (Bitboard a, Bitboard b);

// Check if the bitboard is empty (all bits zero)
bool bitboard_is_zero (Bitboard bb);

// Print the bitboard to the console
// Parameter 'width': grid width for display (e.g., 10)
void bitboard_print (Bitboard bb, uint8_t width);

#endif