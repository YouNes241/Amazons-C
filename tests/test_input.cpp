#include <gtest/gtest.h>

extern "C"
{
#include "../include/input.h"
}

::testing::AssertionResult
MoveEquals (const Move &m, uint8_t sc, uint8_t sr, uint8_t dc, uint8_t dr,
            uint8_t ac, uint8_t ar)
{
  if (m.queen_src.col == sc && m.queen_src.row == sr && m.queen_dst.col == dc
      && m.queen_dst.row == dr && m.arrow_dst.col == ac
      && m.arrow_dst.row == ar)
    {
      return ::testing::AssertionSuccess ();
    }
  return ::testing::AssertionFailure ()
         << "Move mismatch. Expected: (" << (int)sc << "," << (int)sr << ")->("
         << (int)dc << "," << (int)dr << ")x(" << (int)ac << "," << (int)ar
         << ") But got: (" << (int)m.queen_src.col << ","
         << (int)m.queen_src.row << ")->(" << (int)m.queen_dst.col << ","
         << (int)m.queen_dst.row << ")x(" << (int)m.arrow_dst.col << ","
         << (int)m.arrow_dst.row << ")";
}

TEST (InputTest, ParseCoordinatesSuccess)
{
  Move m;

  EXPECT_EQ (parse_coordinates ("a1-b2/c3", &m), 0);
  EXPECT_TRUE (MoveEquals (m, 0, 0, 1, 1, 2, 2));

  EXPECT_EQ (parse_coordinates ("A1-B2/C3", &m), 0);
  EXPECT_TRUE (MoveEquals (m, 0, 0, 1, 1, 2, 2));

  EXPECT_EQ (parse_coordinates ("  d4 - e5 / f6  ", &m), 0);
  EXPECT_TRUE (MoveEquals (m, 3, 3, 4, 4, 5, 5));

  EXPECT_EQ (parse_coordinates ("a10-b10/c10", &m), 0);
  EXPECT_TRUE (MoveEquals (m, 0, 9, 1, 9, 2, 9));
}

TEST (InputTest, ParseCoordinatesFormatErrors)
{
  Move m;

  EXPECT_EQ (parse_coordinates ("hello", &m), -1);

  // no arrow part
  EXPECT_EQ (parse_coordinates ("a1-b2", &m), -1);

  // not the right separators
  EXPECT_EQ (parse_coordinates ("a1_b2_c3", &m), -1);
}

TEST (InputTest, ParseCoordinatesColumnErrors)
{
  Move m;

  // Wrong column characters (not a-z or A-Z)
  EXPECT_EQ (parse_coordinates ("11-b2/c3", &m), -1);
  EXPECT_EQ (parse_coordinates ("a1-!2/c3", &m), -1);
  EXPECT_EQ (parse_coordinates ("a1-b2/[3", &m), -1);
}

TEST (InputTest, ParseCoordinatesRowErrors)
{
  Move m;

  // Lines at 0 (checks the row < 1 protection added to prevent underflow)
  EXPECT_EQ (parse_coordinates ("a0-b2/c3", &m), -1);
  EXPECT_EQ (parse_coordinates ("a1-b0/c3", &m), -1);
  EXPECT_EQ (parse_coordinates ("a1-b2/c0", &m), -1);

  // Negative lines
  EXPECT_EQ (parse_coordinates ("a-1-b2/c3", &m), -1);
}