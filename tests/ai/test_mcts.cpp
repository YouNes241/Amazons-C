#include <gtest/gtest.h>

extern "C"
{
#include "ai_factory.h"
#include "mcts_types.h"
}

// Helper to free GameState board
void
free_gs_board (GameState *state)
{
  for (int i = 0; i < state->size; i++)
    {
      free (state->board[i]);
    }
  free (state->board);
}

// 1. Parameter Tests for MCTS
class MCTSParamTest : public ::testing::Test
{
protected:
  AI_Interface ai;

  void
  SetUp () override
  {
    ConfigMap empty_params = { .list = NULL, .count = 0 };
    ai = create_ai ("mcts", empty_params);
  }

  void
  TearDown () override
  {
    if (ai.vtable && ai.vtable->clean)
      ai.vtable->clean (&ai);
  }
};

TEST_F (MCTSParamTest, UpdatesTimeLimitFromParams)
{
  // Re-init with specific params
  Param p[] = { { (char *)"time_limit", (char *)"1500" } };
  ConfigMap params = { .list = p, .count = 1 };

  ai.vtable->init (&ai, &params);
  MCTS_Data *data = (MCTS_Data *)ai.context;

  EXPECT_EQ (data->time_constraint, 1500);
}

// 2. Functional Test: Finding a Winning Move
// We set up a situation where White has only one winning path.
TEST (MCTSFunctTest, FindWinningMoveInRestrictedState)
{
  // Create a 10x10 board full of arrows
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * 10);
  for (int i = 0; i < 10; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * 10);
      for (int j = 0; j < 10; j++)
        cc[i][j] = CONTENT_ARROW;
    }

  // Only one legal move path for White: (9,0) -> (9,1) -> (9,2)
  cc[9][0] = CONTENT_WHITE_QUEEN;
  cc[9][1] = CONTENT_EMPTY;
  cc[9][2] = CONTENT_EMPTY;

  // Distant Black queen
  cc[0][0] = CONTENT_BLACK_QUEEN;

  GameState state = { .size = 10,
                      .board = cc,
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 50 };

  // Give MCTS 1 second to explore
  Param p = { (char *)"time_limit", (char *)"1000" };
  ConfigMap cm = { .list = &p, .count = 1 };

  AI_Interface ai = create_ai ("mcts", cm);
  ASSERT_NE (ai.vtable, nullptr);

  Move best = ai.vtable->get_best_move (&ai, state);

  // MCTS should find that moving to (9,2) is the best legal move
  EXPECT_EQ (best.queen_src.row, 9);
  EXPECT_EQ (best.queen_src.col, 0);
  EXPECT_TRUE (best.queen_dst.col == 1 || best.queen_dst.col == 2);

  ai.vtable->clean (&ai);
  free_gs_board (&state);
}

// 3. Stress Test: Deterministic Consistency
TEST (MCTSFunctTest, ConsistentMoveUnderSameSeed)
{
  srand (12345); // Set seed for reproducibility

  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * 10);
  for (int i = 0; i < 10; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * 10);
      for (int j = 0; j < 10; j++)
        cc[i][j] = CONTENT_EMPTY;
    }

  // Standard starting pos for simplified test
  cc[0][3] = CONTENT_BLACK_QUEEN;
  cc[9][6] = CONTENT_WHITE_QUEEN;

  GameState state = { .size = 10,
                      .board = cc,
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 0 };

  Param p = { (char *)"time_limit", (char *)"500" };
  ConfigMap cm = { .list = &p, .count = 1 };

  AI_Interface ai1 = create_ai ("mcts", cm);
  Move m1 = ai1.vtable->get_best_move (&ai1, state);
  ai1.vtable->clean (&ai1);

  // Reset seed to same value
  srand (12345);
  AI_Interface ai2 = create_ai ("mcts", cm);
  Move m2 = ai2.vtable->get_best_move (&ai2, state);
  ai2.vtable->clean (&ai2);

  // With the same seed and time, MCTS should reach the same conclusion
  EXPECT_EQ (m1.queen_dst.row, m2.queen_dst.row);
  EXPECT_EQ (m1.queen_dst.col, m2.queen_dst.col);

  free_gs_board (&state);
}

// 4. Factory Test: Creation
TEST (AIFactoryTest, CreateMCTSReturnsCorrectImplementation)
{
  ConfigMap empty_params = { .list = NULL, .count = 0 };
  AI_Interface ai = create_ai ("mcts", empty_params);

  ASSERT_NE (ai.vtable, nullptr);
  ASSERT_NE (ai.context, nullptr);

  ai.vtable->clean (&ai);
}

// 5. Triggering the child loop inside of MCTS to ensure coverage of that code
// path
TEST (MCTSCoverageTest, TriggerChildLoop)
{
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * 4);
  for (int i = 0; i < 4; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * 4);
      for (int j = 0; j < 4; j++)
        cc[i][j] = CONTENT_EMPTY;
    }

  cc[0][0] = CONTENT_WHITE_QUEEN;
  cc[3][3] = CONTENT_BLACK_QUEEN;

  GameState state = { .size = 4,
                      .board = cc,
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 0 };

  Param p = { (char *)"time_limit", (char *)"3000" };
  ConfigMap cm = { .list = &p, .count = 1 };

  AI_Interface ai = create_ai ("mcts", cm);
  Move m = ai.vtable->get_best_move (&ai, state);

  printf ("Best move: Queen from (%d,%d) to (%d,%d) with arrow to (%d,%d)\n",
          m.queen_src.row, m.queen_src.col, m.queen_dst.row, m.queen_dst.col,
          m.arrow_dst.row, m.arrow_dst.col);

  ai.vtable->clean (&ai);
  free_gs_board (&state);
}