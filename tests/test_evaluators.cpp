#include <gtest/gtest.h>
extern "C"
{
#include "bitboard.h"
#include "evaluator.h"
#include "minimax.h"
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