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

// Test symmetrical board
TEST (EvaluatorTest, SymmetryReturnsZero)
{
  BitboardState state = create_empty_state (10);

  // Original starting pos in amazons
  bitboard_set_bit (&state.black_queens, 3);
  bitboard_set_bit (&state.black_queens, 6);
  bitboard_set_bit (&state.black_queens, 30);
  bitboard_set_bit (&state.black_queens, 39);
  bitboard_set_bit (&state.white_queens, 60);
  bitboard_set_bit (&state.white_queens, 69);
  bitboard_set_bit (&state.white_queens, 93);
  bitboard_set_bit (&state.white_queens, 96);

  bitboard_state_print (&state);

  Evaluator eval_me = get_evaluator_by_name ("me");
  Evaluator eval_qdkd = get_evaluator_by_name ("qd&kd");
  Evaluator eval_rth = get_evaluator_by_name ("rth");

  EXPECT_NEAR (eval_me (&state), 0.0f, 0.001f);
  EXPECT_NEAR (eval_qdkd (&state), 0.0f, 0.001f);
  EXPECT_NEAR (eval_rth (&state), 0.0f, 0.001f);
}

// 2. Test de Mobilité : Une reine blanche au centre doit battre une reine
// noire dans un coin.
TEST (EvaluatorTest, MobilityAdvantage)
{
  BitboardState state = create_empty_state (10);

  // Blanche au centre (plus de mouvements)
  bitboard_set_bit (&state.white_queens, 45);
  // Noire dans un coin (très peu de mouvements)
  bitboard_set_bit (&state.black_queens, 0);

  Evaluator eval_me = get_evaluator_by_name ("me");
  // Le score doit être positif car avantage Blanc
  EXPECT_GT (eval_me (&state), 0.0f);
}

// 3. Test d'Enfermement (RTH) : Si une reine noire est totalement entourée
// d'indices/flèches.
TEST (EvaluatorTest, BlackTrappedRTH)
{
  BitboardState state = create_empty_state (10);

  // Reine blanche libre
  bitboard_set_bit (&state.white_queens, 55);

  // Reine noire en (0,0) mais entourée de flèches en (0,1), (1,1), (1,0)
  bitboard_set_bit (&state.black_queens, 0);
  bitboard_set_bit (&state.arrows, 1);
  bitboard_set_bit (&state.arrows, 10);
  bitboard_set_bit (&state.arrows, 11);

  Evaluator eval_rth = get_evaluator_by_name ("rth");
  float score = eval_rth (&state);

  // La blanche possède quasiment tout le terrain, le score doit être
  // massivement positif
  EXPECT_GT (score, 50.0f);
}

// 4. Test QDKD : Vérifier que le mélange Queen/King distance fonctionne
TEST (EvaluatorTest, QDKDWeightedScore)
{
  BitboardState state = create_empty_state (10);

  // Position asymétrique simple
  bitboard_set_bit (&state.white_queens, 44);
  bitboard_set_bit (&state.black_queens, 0);

  Evaluator eval_qdkd = get_evaluator_by_name ("qd&kd");
  float score = eval_qdkd (&state);

  EXPECT_NE (score, 0.0f);
}

TEST (EvaluatorTest, Symmetry4x4)
{
  BitboardState state = create_empty_state (4);
  // Let's place one queen for each at mirrored spots
  bitboard_set_bit (&state.white_queens, 12); // a1 (bottom left)
  bitboard_set_bit (&state.black_queens, 3);  // d4 (top right)

  bitboard_state_print (&state);

  Evaluator eval_me = get_evaluator_by_name ("me");
  Evaluator eval_qdkd = get_evaluator_by_name ("qd&kd");
  Evaluator eval_rth = get_evaluator_by_name ("rth");

  float rth_score = eval_rth (&state);
  EXPECT_NEAR (rth_score, 0.0f, 0.001f);

  float qdkd_score = eval_qdkd (&state);
  printf ("qdkd : %f", qdkd_score);
  EXPECT_NEAR (qdkd_score, 0.0f, 0.001f);

  float me_score = eval_me (&state);
  EXPECT_NEAR (me_score, 0.0f, 0.001f);
}

TEST (EvaluatorTest, Symmetry11x11)
{
  BitboardState state = create_empty_state (11);
  // Mirror pieces across the center
  bitboard_set_bit (&state.white_queens, 110); // a1 (index 110)
  bitboard_set_bit (&state.black_queens, 10);  // k11 (index 10)

  bitboard_state_print (&state);

  Evaluator eval_me = get_evaluator_by_name ("me");
  Evaluator eval_qdkd = get_evaluator_by_name ("qd&kd");
  Evaluator eval_rth = get_evaluator_by_name ("rth");

  float rth_score = eval_rth (&state);
  EXPECT_NEAR (rth_score, 0.0f, 0.001f);

  float qdkd_score = eval_qdkd (&state);
  printf ("qdkd : %f", qdkd_score);
  EXPECT_NEAR (qdkd_score, 0.0f, 0.001f);

  float me_score = eval_me (&state);
  EXPECT_NEAR (me_score, 0.0f, 0.001f);
}

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

  GameState state = { .size = 10,  // Move this to the top!
                      .board = cc, // Then board...
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

  GameState state = { .size = 10,  // Move this to the top!
                      .board = cc, // Then board...
                      .current_player = PLAYER_WHITE,
                      .status = GAME_IN_PROGRESS,
                      .turn_count = 0 };

  BitboardState bs = sync_from_logic (&state);
  bitboard_state_print (&bs);

  Param depth = { .key = "depth", .value = "1" };

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