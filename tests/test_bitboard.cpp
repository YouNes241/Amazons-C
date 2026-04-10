#include <gtest/gtest.h>

// Tell the C++ compiler that these headers are plain C
extern "C"
{
#include "bitboard.h"
}

// ---------------------------------------------------------
// Unit Access Tests
// Tests basic bitboard manipulations (set, clear, get, toggle)
// ---------------------------------------------------------

TEST (BitboardTest, UnitAccessExtremes)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 0);
  bitboard_set_bit (&bb, 127);

  EXPECT_EQ (bitboard_get_bit (bb, 0), 1);
  EXPECT_EQ (bitboard_get_bit (bb, 127), 1);
}

TEST (BitboardTest, UnitAccessBoundary)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 63);
  bitboard_set_bit (&bb, 64);

  EXPECT_EQ (bitboard_get_bit (bb, 63), 1);
  EXPECT_EQ (bitboard_get_bit (bb, 64), 1);
}

TEST (BitboardTest, UnitAccessIsolation)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 40);

  EXPECT_EQ (bitboard_get_bit (bb, 40), 1);
  EXPECT_EQ (bitboard_get_bit (bb, 104), 0);
}

TEST (BitboardTest, UnitAccessClearing)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 10);
  bitboard_set_bit (&bb, 80);
  bitboard_clear_bit (&bb, 80);

  EXPECT_EQ (bitboard_get_bit (bb, 10), 1);
  EXPECT_EQ (bitboard_get_bit (bb, 80), 0);
}

TEST (BitboardTest, UnitAccessToggle)
{
  Bitboard bb = bitboard_create_empty ();

  bitboard_toggle_bit (&bb, 50);
  EXPECT_EQ (bitboard_get_bit (bb, 50), 1);

  bitboard_toggle_bit (&bb, 50);
  EXPECT_EQ (bitboard_get_bit (bb, 50), 0);
}

// ---------------------------------------------------------
// Shift Tests
// Tests bitwise shifts handling 128-bit boundaries correctly
// ---------------------------------------------------------

TEST (BitboardTest, ShiftsCrossUp)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 63);
  bb = bitboard_shift_left (bb, 1);

  EXPECT_EQ (bitboard_get_bit (bb, 64), 1);
  EXPECT_EQ (bitboard_get_bit (bb, 63), 0);
}

TEST (BitboardTest, ShiftsCrossDown)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 64);
  bb = bitboard_shift_right (bb, 1);

  EXPECT_EQ (bitboard_get_bit (bb, 63), 1);
  EXPECT_EQ (bitboard_get_bit (bb, 64), 0);
}

TEST (BitboardTest, ShiftsFullSwitch)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 10);
  bb = bitboard_shift_left (bb, 64);

  EXPECT_EQ (bitboard_get_bit (bb, 74), 1);
  EXPECT_EQ (bb.low, 0);
}

TEST (BitboardTest, ShiftsGiant)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 5);
  bb = bitboard_shift_left (bb, 70);

  EXPECT_EQ (bitboard_get_bit (bb, 75), 1);
  EXPECT_EQ (bb.low, 0);
}

TEST (BitboardTest, ShiftsNull)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 25);
  bb = bitboard_shift_left (bb, 0);

  EXPECT_EQ (bitboard_get_bit (bb, 25), 1);
  EXPECT_EQ (bb.high, 0);
}

TEST (BitboardTest, ShiftsInvalidBounds)
{
  Bitboard bb = bitboard_create_empty ();
  bitboard_set_bit (&bb, 10);
  bitboard_set_bit (&bb, 100);
  bb = bitboard_shift_left (bb, 130);

  EXPECT_EQ (bb.low, 0);
  EXPECT_EQ (bb.high, 0);
}

// ---------------------------------------------------------
// Logical Operations Tests
// Tests logical OR, AND, NOT across the 128-bit structure
// ---------------------------------------------------------

TEST (BitboardTest, LogicalOr)
{
  Bitboard bb1 = bitboard_create_empty ();
  Bitboard bb2 = bitboard_create_empty ();

  bitboard_set_bit (&bb1, 15);
  bitboard_set_bit (&bb2, 85);

  Bitboard result_or = bitboard_or (bb1, bb2);

  EXPECT_EQ (bitboard_get_bit (result_or, 15), 1);
  EXPECT_EQ (bitboard_get_bit (result_or, 85), 1);
}

TEST (BitboardTest, LogicalAnd)
{
  Bitboard bb1 = bitboard_create_empty ();
  Bitboard bb2 = bitboard_create_empty ();

  bitboard_set_bit (&bb1, 42);
  bitboard_set_bit (&bb2, 42);
  bitboard_set_bit (&bb2, 90);

  Bitboard result_and = bitboard_and (bb1, bb2);

  EXPECT_EQ (bitboard_get_bit (result_and, 42), 1);
  EXPECT_EQ (bitboard_get_bit (result_and, 90), 0);
}

TEST (BitboardTest, LogicalNot)
{
  Bitboard bb = bitboard_create_empty ();
  Bitboard result_not = bitboard_not (bb);

  EXPECT_EQ (result_not.low, ~0ULL);
  EXPECT_EQ (result_not.high, ~0ULL);
}