#include <gtest/gtest.h>
extern "C"
{
#include "minimax.h"
}

TEST (AmazonsLimitTest, FindMaxMoves11x11)
{
  BitboardState state;
  state.size = 11;
  state.white_to_move = true;
  state.white_queens = { 0, 0 }; // Clear
  state.black_queens = { 0, 0 };
  state.arrows = { 0, 0 };

  // Place queens in the "sweet spots" for 11x11 mobility
  // Indices: 38, 40, 80, 82 (Calculated for max 8-way reach)
  bitboard_set_bit (&state.white_queens, 38);
  bitboard_set_bit (&state.white_queens, 40);
  bitboard_set_bit (&state.white_queens, 80);
  bitboard_set_bit (&state.white_queens, 82);

  // Call your function with a massive overflow-safe buffer
  Move massive_buffer[10000];
  int count = get_legal_moves (state, massive_buffer);

  printf ("\n[REPORT] Max moves found on open 11x11 board: %d\n", count);

  // This is the starting position check you mentioned
  // Adjust based on your actual initial board setup
  EXPECT_GT (count, 2176);
}