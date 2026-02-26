#include <gtest/gtest.h>

extern "C"
{
#include "bitboard.h"
#include "utils.h"
}

class GetReachableTest : public ::testing::Test
{
protected:
  uint8_t size = 10;
  Bitboard occupied;

  void
  SetUp () override
  {
    occupied = bitboard_create_empty ();
  }
};

// 1. Test movement on a completely empty board from the center
TEST_F (GetReachableTest, CenterEmptyBoard)
{
  uint8_t start_idx = 55; // (5, 5) on 10x10
  Bitboard reachable = get_reachable (start_idx, occupied, size);

  bitboard_print (reachable, size);

  // Should be able to reach all the way to the edges
  EXPECT_TRUE (bitboard_get_bit (reachable, 5));  // Far North (0, 5)
  EXPECT_TRUE (bitboard_get_bit (reachable, 95)); // Far South (9, 5)
  EXPECT_TRUE (bitboard_get_bit (reachable, 50)); // Far West (5, 0)
  EXPECT_TRUE (bitboard_get_bit (reachable, 59)); // Far East (5, 9)

  // Diagonal check: North-West to (0,0)
  EXPECT_TRUE (bitboard_get_bit (reachable, 0));

  // Print it
}

// 2. Test that obstacles correctly block the "Slide"
TEST_F (GetReachableTest, ObstacleBlocking)
{
  uint8_t start_idx = 44; // (4, 4)

  // Place an obstacle at (4, 6) which is index 46
  bitboard_set_bit (&occupied, 46);

  Bitboard reachable = get_reachable (start_idx, occupied, size);

  bitboard_print (reachable, size);

  EXPECT_TRUE (bitboard_get_bit (reachable, 45)); // One step East: OK
  EXPECT_FALSE (
      bitboard_get_bit (reachable, 46)); // The obstacle itself: BLOCKED
  EXPECT_FALSE (
      bitboard_get_bit (reachable, 47)); // Behind the obstacle: BLOCKED
}

// 3. Test that the starting square is NOT marked as reachable
TEST_F (GetReachableTest, DoesNotIncludeStart)
{
  uint8_t start_idx = 22;
  Bitboard reachable = get_reachable (start_idx, occupied, size);

  bitboard_print (reachable, size);

  EXPECT_FALSE (bitboard_get_bit (reachable, start_idx));
  EXPECT_FALSE (bitboard_get_bit (reachable, 43));
}
