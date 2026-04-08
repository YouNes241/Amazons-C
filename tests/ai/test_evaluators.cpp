#include <gtest/gtest.h>

extern "C"
{
#include "ai_factory.h"
#include "bitboard.h"
#include "evaluator.h"
#include "utils.h"
}

// ============================================================
// Fixture — EvaluatorSymmetry
// Provides the three evaluator pointers and a helper that builds a
// diagonally symmetric state (white at bottom-left, black at top-right)
// for a given board size.
// ============================================================

class EvaluatorSymmetry : public ::testing::Test
{
protected:
  Evaluator eval_me;
  Evaluator eval_rth;
  Evaluator eval_qdkd;

  void
  SetUp () override
  {
    eval_me = get_evaluator (EVAL_ME);
    eval_rth = get_evaluator (EVAL_RTH);
    eval_qdkd = get_evaluator (EVAL_QD_KD);
  }

  // Builds a state where white is at (size-1, 0) and black is at (0, size-1),
  // a strictly diagonal-symmetric position. All three evaluators must return
  // 0.
  BitboardState
  make_corner_symmetric (uint8_t size)
  {
    BitboardState state = create_empty_state (size);
    bitboard_set_bit (&state.white_queens, (uint8_t)((size - 1) * size));
    bitboard_set_bit (&state.black_queens, (uint8_t)(size - 1));
    return state;
  }

  // Builds a state where white is at (1,1) and black is at its diagonal
  // mirror, plus two arrow mirrors. All three evaluators must return 0.
  BitboardState
  make_inner_symmetric (uint8_t size)
  {
    BitboardState state = create_empty_state (size);
    uint8_t r1 = 1, c1 = 1;
    uint8_t r2 = size - 1 - r1, c2 = size - 1 - c1;
    bitboard_set_bit (&state.white_queens, (uint8_t)(r1 * size + c1));
    bitboard_set_bit (&state.black_queens, (uint8_t)(r2 * size + c2));
    bitboard_set_bit (&state.arrows, (uint8_t)(0 * size + 1));
    bitboard_set_bit (&state.arrows,
                      (uint8_t)((size - 1) * size + (size - 2)));
    return state;
  }
};

// ============================================================
// Fixture — EvaluatorWin
// Provides a board where white has a completely free row and black is
// isolated in the opposite corner.
// ============================================================

class EvaluatorWin : public ::testing::Test
{
protected:
  static const uint8_t SIZE = 10;
  BitboardState state;

  void
  SetUp () override
  {
    state = create_empty_state (SIZE);

    // Fill the board with arrows, then carve out white's free path and
    // black's isolated corner.
    for (uint8_t i = 0; i < SIZE * SIZE; i++)
      bitboard_set_bit (&state.arrows, i);

    // White queen at bottom-left corner.
    bitboard_clear_bit (&state.arrows, (uint8_t)((SIZE - 1) * SIZE));
    bitboard_set_bit (&state.white_queens, (uint8_t)((SIZE - 1) * SIZE));

    // Free the row to the right of white so she can move.
    for (uint8_t col = 1; col < SIZE - 1; col++)
      bitboard_clear_bit (&state.arrows, (uint8_t)((SIZE - 1) * SIZE + col));

    // One empty escape square adjacent to the black queen's corner.
    bitboard_clear_bit (&state.arrows,
                        (uint8_t)((SIZE - 2) * SIZE + (SIZE - 1)));

    // Black queen at top-right corner (surrounded by arrows on all other
    // sides).
    bitboard_clear_bit (&state.arrows, (uint8_t)(SIZE - 1));
    bitboard_set_bit (&state.black_queens, (uint8_t)(SIZE - 1));
  }
};

// ============================================================
// eval_me — Mobility evaluator
// ============================================================

// A centered queen has access to far more squares than a corner queen, so
// the score must be strictly positive for the side with the central piece.
TEST (EvalMe, CentralQueenOutscoresCornerQueen)
{
  BitboardState state = create_empty_state (10);
  bitboard_set_bit (&state.white_queens, 45); // near-center
  bitboard_set_bit (&state.black_queens, 0);  // corner

  EXPECT_GT (get_evaluator (EVAL_ME) (&state), 0.0f);
}

// A perfectly symmetric position must score exactly 0 for all board sizes.
TEST_F (EvaluatorSymmetry, MeCornerSymmetry)
{
  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = make_corner_symmetric (size);
      EXPECT_NEAR (eval_me (&state), 0.0f, 0.001f)
          << "eval_me corner symmetry failed at size " << (int)size;
    }
}

// A symmetric position with inner queens and mirror arrows must also score 0.
TEST_F (EvaluatorSymmetry, MeInnerSymmetry)
{
  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = make_inner_symmetric (size);
      EXPECT_NEAR (eval_me (&state), 0.0f, 0.001f)
          << "eval_me inner symmetry failed at size " << (int)size;
    }
}

// In a position where white has a free row and black is isolated, white's
// mobility advantage must be very large (> 1000 reachable squares difference).
TEST_F (EvaluatorWin, MeRecognisesWinningAdvantage)
{
  EXPECT_GT (get_evaluator (EVAL_ME) (&state), 1000.0f);
}

// ============================================================
// eval_rth — Relative Territory Heuristic
// ============================================================

// A symmetric position must score 0 for all board sizes.
TEST_F (EvaluatorSymmetry, RthCornerSymmetry)
{
  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = make_corner_symmetric (size);
      EXPECT_NEAR (eval_rth (&state), 0.0f, 0.001f)
          << "eval_rth corner symmetry failed at size " << (int)size;
    }
}

// A symmetric position with inner queens and mirror arrows must also score 0.
TEST_F (EvaluatorSymmetry, RthInnerSymmetry)
{
  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = make_inner_symmetric (size);
      EXPECT_NEAR (eval_rth (&state), 0.0f, 0.001f)
          << "eval_rth inner symmetry failed at size " << (int)size;
    }
}

// Wherever a black queen is placed on the board and fully surrounded by
// arrows, the RTH score must be strongly positive for white (> 10).
TEST (EvalRth, TrappedBlackQueenEveryPosition)
{
  Evaluator eval_rth = get_evaluator (EVAL_RTH);

  for (uint8_t size = 4; size <= 11; size++)
    for (uint8_t r = 0; r < size; r++)
      for (uint8_t c = 0; c < size; c++)
        {
          BitboardState state = create_empty_state (size);
          uint8_t black_pos = r * size + c;
          bitboard_set_bit (&state.black_queens, black_pos);

          for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++)
              {
                if (dr == 0 && dc == 0)
                  continue;
                int nr = (int)r + dr, nc = (int)c + dc;
                if (nr >= 0 && nr < size && nc >= 0 && nc < size)
                  bitboard_set_bit (&state.arrows, (uint8_t)(nr * size + nc));
              }

          uint8_t white_r = (r < size / 2) ? (size - 1) : 0;
          uint8_t white_c = (c < size / 2) ? (size - 1) : 0;
          bitboard_set_bit (&state.white_queens,
                            (uint8_t)(white_r * size + white_c));

          EXPECT_GT (eval_rth (&state), 10.0f)
              << "Black trapped at (" << (int)r << "," << (int)c << ") on "
              << (int)size << "x" << (int)size << " was not penalised enough.";
        }
}

// In a winning position for white, RTH must reflect the overwhelming
// territorial advantage (> 1000).
TEST_F (EvaluatorWin, RthRecognisesWinningAdvantage)
{
  EXPECT_GT (get_evaluator (EVAL_RTH) (&state), 1000.0f);
}

// ============================================================
// eval_qdkd — Queen Distance + King Distance evaluator
// ============================================================

// A centered white queen versus a cornered black queen must yield a positive
// score since white dominates more territory.
TEST (EvalQdkd, CentralQueenScoresPositive)
{
  BitboardState state = create_empty_state (10);
  bitboard_set_bit (&state.white_queens, 44);
  bitboard_set_bit (&state.black_queens, 0);

  EXPECT_GT (get_evaluator (EVAL_QD_KD) (&state), 0.0f);
}

// A symmetric position must score 0 for all board sizes.
TEST_F (EvaluatorSymmetry, QdkdCornerSymmetry)
{
  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = make_corner_symmetric (size);
      EXPECT_NEAR (eval_qdkd (&state), 0.0f, 0.001f)
          << "eval_qdkd corner symmetry failed at size " << (int)size;
    }
}

// A symmetric position with inner queens and mirror arrows must also score 0.
TEST_F (EvaluatorSymmetry, QdkdInnerSymmetry)
{
  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = make_inner_symmetric (size);
      EXPECT_NEAR (eval_qdkd (&state), 0.0f, 0.001f)
          << "eval_qdkd inner symmetry failed at size " << (int)size;
    }
}

// In a winning position for white, QDKD must reflect the overwhelming
// advantage (> 1000).
TEST_F (EvaluatorWin, QdkdRecognisesWinningAdvantage)
{
  EXPECT_GT (get_evaluator (EVAL_QD_KD) (&state), 1000.0f);
}