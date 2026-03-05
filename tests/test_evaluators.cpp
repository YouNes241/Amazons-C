#include <gtest/gtest.h>
extern "C"
{
#include "ai_factory.h"
#include "bitboard.h"
#include "evaluator.h"
#include "minimax.h"
#include "random.h"
#include "utils.h"
}

// Symmetry test : a position and its mirror should have the same score
TEST (EvaluatorTest, SymmetryAllSizes)
{
  Evaluator eval_me = get_evaluator_by_name ("me");
  Evaluator eval_qdkd = get_evaluator_by_name ("qd&kd");
  Evaluator eval_rth = get_evaluator_by_name ("rth");

  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = create_empty_state (size);

      uint8_t r1 = 1, c1 = 1;
      uint8_t r2 = size - 1 - r1;
      uint8_t c2 = size - 1 - c1;

      bitboard_set_bit (&state.white_queens, r1 * size + c1);
      bitboard_set_bit (&state.black_queens, r2 * size + c2);

      bitboard_set_bit (&state.arrows, 0 * size + 1);
      bitboard_set_bit (&state.arrows, (size - 1) * size + (size - 2));

      EXPECT_NEAR (eval_me (&state), 0.0f, 0.001f)
          << "Symmetry failed for 'me' at size " << (int)size;
      EXPECT_NEAR (eval_qdkd (&state), 0.0f, 0.001f)
          << "Symmetry failed for 'qdkd' at size " << (int)size;
      EXPECT_NEAR (eval_rth (&state), 0.0f, 0.001f)
          << "Symmetry failed for 'rth' at size " << (int)size;
    }
}

// Mobility test : a centered queen vs a cornered queen
TEST (EvaluatorTest, MobilityAdvantage)
{
  BitboardState state = create_empty_state (10);

  bitboard_set_bit (&state.white_queens, 45);
  bitboard_set_bit (&state.black_queens, 0);

  Evaluator eval_me = get_evaluator_by_name ("me");

  EXPECT_GT (eval_me (&state), 0.0f);
}

TEST (EvaluatorTest, BlackTrappedEverywhere)
{
  Evaluator eval_rth = get_evaluator_by_name ("rth");

  for (uint8_t size = 4; size <= 11; size++)
    {
      for (uint8_t r = 0; r < size; r++)
        {
          for (uint8_t c = 0; c < size; c++)
            {
              BitboardState state = create_empty_state (size);
              uint8_t black_pos = r * size + c;

              // 1. Place the Black Queen at the current tile
              bitboard_set_bit (&state.black_queens, black_pos);

              // 2. Surround her with arrows on all valid neighbors
              for (int dr = -1; dr <= 1; dr++)
                {
                  for (int dc = -1; dc <= 1; dc++)
                    {
                      int nr = (int)r + dr;
                      int nc = (int)c + dc;

                      // If it's a valid neighbor and not the queen's square
                      if (nr >= 0 && nr < size && nc >= 0 && nc < size)
                        {
                          uint8_t neighbor_idx = nr * size + nc;
                          if (neighbor_idx != black_pos)
                            {
                              bitboard_set_bit (&state.arrows, neighbor_idx);
                            }
                        }
                    }
                }

              // 3. Place a free White Queen as far away as possible
              uint8_t white_r = (r < size / 2) ? (size - 1) : 0;
              uint8_t white_c = (c < size / 2) ? (size - 1) : 0;
              bitboard_set_bit (&state.white_queens, white_r * size + white_c);

              float score = eval_rth (&state);

              EXPECT_GT (score, 10.0f)
                  << "Failure: Black trapped at (" << (int)r << "," << (int)c
                  << ") on " << (int)size << "x" << (int)size
                  << " board was not penalized enough.";
            }
        }
    }
}

// QDKD test : a position with a clear mobility advantage should have a
// positive score
TEST (EvaluatorTest, QDKDWeightedScore)
{
  BitboardState state = create_empty_state (10);

  bitboard_set_bit (&state.white_queens, 44);
  bitboard_set_bit (&state.black_queens, 0);

  Evaluator eval_qdkd = get_evaluator_by_name ("qd&kd");
  float score = eval_qdkd (&state);

  EXPECT_GT (score, 0.0f);
}

// Checks that all evaluators have a score of 0, for any game size.
TEST (EvaluatorTest, SymmetryRange4to11)
{
  Evaluator eval_me = get_evaluator_by_name ("me");
  Evaluator eval_qdkd = get_evaluator_by_name ("qd&kd");
  Evaluator eval_rth = get_evaluator_by_name ("rth");

  for (uint8_t size = 4; size <= 11; size++)
    {
      BitboardState state = create_empty_state (size);

      // Place White Queen at bottom-left corner: (row = size-1, col = 0)
      uint8_t white_idx = (size - 1) * size;
      // Place Black Queen at top-right corner: (row = 0, col = size-1)
      uint8_t black_idx = size - 1;

      bitboard_set_bit (&state.white_queens, white_idx);
      bitboard_set_bit (&state.black_queens, black_idx);

      // Verify symmetry (Score should be 0.0)
      EXPECT_NEAR (eval_me (&state), 0.0f, 0.001f)
          << "Failed 'me' symmetry at size " << (int)size;
      EXPECT_NEAR (eval_qdkd (&state), 0.0f, 0.001f)
          << "Failed 'qdkd' symmetry at size " << (int)size;
      EXPECT_NEAR (eval_rth (&state), 0.0f, 0.001f)
          << "Failed 'rth' symmetry at size " << (int)size;
    }
}

// Checks that random moves works.
TEST (EvaluatorTest, RandomMove)
{
  // bitboardstate to gamestate
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * 10);
  for (int i = 0; i < 10; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * 10);
      for (int j = 0; j < 10; j++)
        {
          cc[i][j] = CONTENT_EMPTY;
        }
    }

  cc[0][3] = CONTENT_BLACK_QUEEN;
  cc[9][6] = CONTENT_WHITE_QUEEN;

  GameState state = { .size = 10,
                      .board = cc,
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 0 };

  BitboardState bs = sync_from_logic (&state);
  bitboard_state_print (&bs);

  AI_Interface randomAI = create_ai ("random", { NULL, 0 });
  Move move = randomAI.vtable->get_best_move (&randomAI, state);

  printf ("Random move generated: Queen from (%d,%d) to (%d,%d) with arrow to "
          "(%d,%d)\n",
          move.queen_src.row, move.queen_src.col, move.queen_dst.row,
          move.queen_dst.col, move.arrow_dst.row, move.arrow_dst.col);
  EXPECT_TRUE (move.queen_src.row >= 0 && move.queen_src.row < 10);
  EXPECT_TRUE (move.queen_src.col >= 0 && move.queen_src.col < 10);
  EXPECT_TRUE (move.queen_dst.row >= 0 && move.queen_dst.col < 10);
  EXPECT_TRUE (move.queen_dst.row >= 0 && move.queen_dst.col < 10);
  EXPECT_TRUE (move.arrow_dst.row >= 0 && move.arrow_dst.row < 10);
  EXPECT_TRUE (move.arrow_dst.col >= 0 && move.arrow_dst.col < 10);
}

// Checks that minimax move works.
TEST (EvaluatorTest, MiniMaxMove)
{
  // bitboardstate to gamestate
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * 10);
  for (int i = 0; i < 10; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * 10);
      for (int j = 0; j < 10; j++)
        {
          cc[i][j] = CONTENT_EMPTY;
        }
    }

  cc[0][3] = CONTENT_BLACK_QUEEN;
  cc[0][6] = CONTENT_BLACK_QUEEN;
  cc[3][0] = CONTENT_BLACK_QUEEN;
  cc[3][9] = CONTENT_BLACK_QUEEN;
  cc[6][0] = CONTENT_WHITE_QUEEN;
  cc[6][9] = CONTENT_WHITE_QUEEN;
  cc[9][3] = CONTENT_WHITE_QUEEN;
  cc[9][6] = CONTENT_WHITE_QUEEN;

  GameState state = { .size = 10,
                      .board = cc,
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 0 };

  BitboardState bs = sync_from_logic (&state);
  bitboard_state_print (&bs);

  Param depth = { .key = "depth", .value = "3" };
  Param evaluator = { .key = "evaluator", .value = "me" };
  Param params[] = { depth, evaluator };
  ConfigMap cm = { .list = params, .count = 2 };
  AI_Interface randomAI = create_ai ("minimax", cm);
  Move move = randomAI.vtable->get_best_move (&randomAI, state);

  printf (
      "Minimax move generated: Queen from (%d,%d) to (%d,%d) with arrow to "
      "(%d,%d)\n",
      move.queen_src.row, move.queen_src.col, move.queen_dst.row,
      move.queen_dst.col, move.arrow_dst.row, move.arrow_dst.col);
  EXPECT_TRUE (move.queen_src.row >= 0 && move.queen_src.row < 10);
  EXPECT_TRUE (move.queen_src.col >= 0 && move.queen_src.col < 10);
  EXPECT_TRUE (move.queen_dst.row >= 0 && move.queen_dst.col < 10);
  EXPECT_TRUE (move.queen_dst.row >= 0 && move.queen_dst.col < 10);
  EXPECT_TRUE (move.arrow_dst.row >= 0 && move.arrow_dst.row < 10);
  EXPECT_TRUE (move.arrow_dst.col >= 0 && move.arrow_dst.col < 10);
}

// Checks that minimax move works when the opponent is completely trapped
// (should always find the winning move).
// This test also serves to check that the depth is efficiently prunned when
// unnecessary, as the winning move is at depth 1.
TEST (EvaluatorTest, MiniMaxWin)
{
  // bitboardstate to gamestate
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * 10);
  for (int i = 0; i < 10; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * 10);
      for (int j = 0; j < 10; j++)
        {
          cc[i][j] = CONTENT_ARROW;
        }
    }

  cc[9][0] = CONTENT_WHITE_QUEEN;
  for (int i = 1; i < 9; i++)
    {
      cc[9][i] = CONTENT_EMPTY;
    }
  cc[8][9] = CONTENT_EMPTY;
  cc[7][9] = CONTENT_BLACK_QUEEN;

  GameState state = { .size = 10,
                      .board = cc,
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 0 };

  BitboardState bs = sync_from_logic (&state);
  bitboard_state_print (&bs);

  Param depth = { .key = "depth", .value = "3" };

  Param evaluator_qdkd = { .key = "evaluator", .value = "qdkd" };
  Param evaluator_rth = { .key = "evaluator", .value = "rth" };
  Param evaluator_me = { .key = "evaluator", .value = "me" };

  Param params_qdkd[] = { depth, evaluator_qdkd };
  Param params_rth[] = { depth, evaluator_rth };
  Param params_me[] = { depth, evaluator_me };

  ConfigMap cm_qdkd = { .list = params_qdkd, .count = 2 };
  ConfigMap cm_rth = { .list = params_rth, .count = 2 };
  ConfigMap cm_me = { .list = params_me, .count = 2 };

  AI_Interface qdkdAI = create_ai ("minimax", cm_qdkd);
  AI_Interface rthAI = create_ai ("minimax", cm_rth);
  AI_Interface meAI = create_ai ("minimax", cm_me);

  Move move_qdkd = qdkdAI.vtable->get_best_move (&qdkdAI, state);
  Move move_rth = rthAI.vtable->get_best_move (&rthAI, state);
  Move move_me = meAI.vtable->get_best_move (&meAI, state);

  Move all_moves[] = { move_qdkd, move_rth, move_me };

  for (Move m : all_moves)
    {
      printf ("Move generated: Queen from (%d,%d) to (%d,%d) with arrow to "
              "(%d,%d)\n",
              m.queen_src.row, m.queen_src.col, m.queen_dst.row,
              m.queen_dst.col, m.arrow_dst.row, m.arrow_dst.col);
      EXPECT_EQ (m.queen_src.row, 9);
      EXPECT_EQ (m.queen_src.col, 0);
      EXPECT_EQ (m.queen_dst.row, 9);
      EXPECT_EQ (m.queen_dst.col, 8);
      EXPECT_EQ (m.arrow_dst.row, 8);
      EXPECT_EQ (m.arrow_dst.col, 9);
    }
}