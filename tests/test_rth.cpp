#include <gtest/gtest.h>
extern "C"
{
#include "bitboard.h"
#include "evaluator.h"
#include "minimax.h"
#include "utils.h"
}

// RTH : Symmetric Initial Position
// Score should be 0 due to the fact that any position is equally symetrically
// reachable by the other part.

class EvalRthTest : public ::testing::Test
{
protected:
  BitboardState state;
  Evaluator eval_rth;

  void
  SetUp () override
  {
    state.white_queens = { 0, 0 };
    state.black_queens = { 0, 0 };
    state.arrows = { 0, 0 };
    state.size = 10;
    state.white_to_move = true;
    eval_rth = get_evaluator_by_name ("rth");
  }
};

TEST_F (EvalRthTest, SymmetricInitialPosition)
{
  // Standard starting squares in 10x10, following the original rules of the
  // amazons game.
  bitboard_set_bit (&state.black_queens, 3);
  bitboard_set_bit (&state.black_queens, 6);
  bitboard_set_bit (&state.black_queens, 30);
  bitboard_set_bit (&state.black_queens, 39);
  bitboard_set_bit (&state.white_queens, 60);
  bitboard_set_bit (&state.white_queens, 69);
  bitboard_set_bit (&state.white_queens, 93);
  bitboard_set_bit (&state.white_queens, 96);

  float score = eval_rth (&state);
  EXPECT_NEAR (score, 0.0f, 0.001f);
}

// Test 2: Absolute Territory Advantage
// Black being blocked means white should have an absolute advantage
// Since we're using RTH, the score will be *very* high in that situation.
// That's because we emphasize on moves that gives an absolute advantage
// (ie. "killing" a queen).
// If this was MSP, score would be the number of available squares.
TEST_F (EvalRthTest, WhiteTerritoryAbsoluteAdvantage)
{
  // Isolate a player
  bitboard_set_bit (&state.black_queens, 0); // a1
  bitboard_set_bit (&state.arrows, 1);       // a2
  bitboard_set_bit (&state.arrows, 10);      // b1
  bitboard_set_bit (&state.arrows, 11);      // b2

  // Setup another player, which is free to move
  bitboard_set_bit (&state.white_queens, 99); // j10

  float score = eval_rth (&state);

  bitboard_state_print (&state);
  printf ("Score reached was %f !", score);

  EXPECT_GT (score, 0.0f);
}

// Test 3 : Barricade impact
TEST_F (EvalRthTest, RelativeBarricadeImpact)
{

  // Isolate a player
  bitboard_set_bit (&state.black_queens, 0); // a1
  bitboard_set_bit (&state.arrows, 10);      // b1
  bitboard_set_bit (&state.arrows, 11);      // b2

  bitboard_set_bit (&state.white_queens, 99); // j10

  float score = eval_rth (&state);

  bitboard_state_print (&state);
  printf ("Score reached was %f !", score);

  bitboard_set_bit (&state.arrows, 12); // b3

  float new_score = eval_rth (&state);

  EXPECT_GT (score, 0.0f);
  EXPECT_LT (score, 100.0f);
  EXPECT_GT (new_score, score); // just slightly
}