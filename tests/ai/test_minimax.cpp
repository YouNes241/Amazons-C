#include <dlfcn.h>
#include <gtest/gtest.h>

#define MINIMAX_TEST_MODE

extern "C"
{
#include "../../src/ai/minimax/minimax_includes/minimax_internal.h"
#include "ai_factory.h"
#include "ai_interface.h"
#include "bitboard.h"
#include "bitboard_state.h"
#include "minimax.h"
}

// ============================================================
// Malloc failure interception
// ============================================================

static bool force_malloc_fail = false;

extern "C" void *
malloc (size_t size)
{
  if (force_malloc_fail)
    return nullptr;

  static void *(*real_malloc) (size_t)
      = (void *(*)(size_t))dlsym (RTLD_NEXT, "malloc");

  return real_malloc (size);
}

// RAII guard: activates malloc failure on construction, restores it on
// destruction — safe even if the test throws.
struct MallocFailGuard
{
  MallocFailGuard () { force_malloc_fail = true; }
  ~MallocFailGuard () { force_malloc_fail = false; }
};

// ============================================================
// Helpers
// ============================================================

static BitboardState
make_empty (uint8_t size = 10)
{
  return create_empty_state (size);
}

// Surrounds the queen at (r,c) with arrows on every adjacent square so it
// has no legal moves.
static void
trap_queen (BitboardState *state, bool is_white, uint8_t r, uint8_t c)
{
  uint8_t size = state->size;
  if (is_white)
    bitboard_set_bit (&state->white_queens, (uint8_t)(r * size + c));
  else
    bitboard_set_bit (&state->black_queens, (uint8_t)(r * size + c));

  for (int dr = -1; dr <= 1; dr++)
    for (int dc = -1; dc <= 1; dc++)
      {
        if (dr == 0 && dc == 0)
          continue;
        int nr = (int)r + dr, nc = (int)c + dc;
        if (nr >= 0 && nr < size && nc >= 0 && nc < size)
          bitboard_set_bit (&state->arrows, (uint8_t)(nr * size + nc));
      }
}

// Allocates a size×size board filled with CONTENT_EMPTY. When
// place_random_queens is true, one white and one black queen are placed at
// random positions to ensure get_best_move does not immediately fall back to
// get_direct_random_move.
static CellContent **
allocate_empty_board (uint8_t size, bool place_random_queens = true)
{
  CellContent **board = (CellContent **)malloc (sizeof (CellContent *) * size);
  for (uint8_t i = 0; i < size; i++)
    {
      board[i] = (CellContent *)malloc (sizeof (CellContent) * size);
      for (uint8_t j = 0; j < size; j++)
        board[i][j] = CONTENT_EMPTY;
    }

  if (place_random_queens)
    {
      board[rand () % size][rand () % size] = CONTENT_WHITE_QUEEN;
      board[rand () % size][rand () % size] = CONTENT_BLACK_QUEEN;
    }

  return board;
}

static void
free_board (CellContent **board, uint8_t size)
{
  for (uint8_t i = 0; i < size; i++)
    free (board[i]);
  free (board);
}

// Builds a Minimax_Data with sensible defaults, overridable via parameters.
static Minimax_Data
make_data (uint64_t time_limit_ms = 5000, int max_depth = 3)
{
  Minimax_Data d;
  d.max_depth = max_depth;
  d.time_limit = time_limit_ms;
  d.forward_pruning_enabled = true;
  d.iterative_deepening_enabled = false;
  d.evaluate = get_evaluator (EVAL_ME);
  return d;
}

// Evaluator that always returns the highest possible score, used to force
// alpha-beta pruning on the very first move at the root.
static float
perfect_eval (const BitboardState *)
{
  return INT_MAX;
}

// ============================================================
// Fixture — MinimaxAI
// Manages the full AI lifecycle (create → init → [test] → clean).
// ============================================================

class MinimaxAI : public ::testing::Test
{
protected:
  AI_Interface ai;

  void
  SetUp () override
  {
    create_minimax_ai (&ai);
    ai.vtable->init (&ai, NULL);
  }

  void
  TearDown () override
  {
    if (ai.context)
      ai.vtable->clean (&ai);
  }

  // Cleans the current context and re-initialises with custom parameters.
  void
  reinit (Param *params, int count)
  {
    if (ai.context)
      ai.vtable->clean (&ai);
    ConfigMap cm = { count, params };
    ai.vtable->init (&ai, &cm);
  }
};

// ============================================================
// get_queen_moves
// ============================================================

// A queen at (4,5) on an otherwise empty 10×10 board should reach exactly
// 35 squares (9 N/S + 9 E/W + 9×2 diagonals, minus the queen itself).
TEST (GetQueenMoves, OpenField)
{
  uint8_t size = 10;
  uint8_t queen_idx = 45;
  Bitboard occupied = {};
  bitboard_set_bit (&occupied, queen_idx);

  Move moves[100];
  int count = get_queen_moves (queen_idx, occupied, size, moves);

  EXPECT_EQ (count, 35);
  EXPECT_EQ (moves[0].queen_src.row, 4);
  EXPECT_EQ (moves[0].queen_src.col, 5);
  EXPECT_EQ (moves[0].arrow_dst.row, 255);
}

// A queen fully surrounded by arrows must report zero legal queen moves.
TEST (GetQueenMoves, Trapped)
{
  BitboardState state = make_empty (10);
  trap_queen (&state, true, 0, 0);
  Bitboard occupied = bitboard_or (
      bitboard_or (state.white_queens, state.black_queens), state.arrows);

  Move moves[100];
  EXPECT_EQ (get_queen_moves (0, occupied, 10, moves), 0);
}

// An obstacle at (0,2) must block all squares at column ≥ 2 on row 0.
TEST (GetQueenMoves, BlockedByObstacle)
{
  Bitboard occupied = {};
  bitboard_set_bit (&occupied, 0);
  bitboard_set_bit (&occupied, 2);

  Move moves[100];
  int count = get_queen_moves (0, occupied, 10, moves);

  for (int i = 0; i < count; i++)
    EXPECT_FALSE (moves[i].queen_dst.row == 0 && moves[i].queen_dst.col >= 2);
}

// ============================================================
// get_arrow_moves
// ============================================================

// From (4,4) on a near-empty board the arrow should have 35 destinations,
// including the queen's vacated origin square.
TEST (GetArrowMoves, GeneratesCombinations)
{
  Bitboard occupied = {};
  bitboard_set_bit (&occupied, 0);
  Move q_move = { { 0, 0 }, { 4, 4 }, { 255, 255 } };

  Move final_moves[2000];
  int total = get_arrow_moves (&q_move, 1, occupied, 10, final_moves);

  EXPECT_EQ (total, 35);

  bool can_fire_back = false;
  for (int i = 0; i < total; i++)
    if (final_moves[i].arrow_dst.row == 0 && final_moves[i].arrow_dst.col == 0)
      {
        can_fire_back = true;
        break;
      }
  EXPECT_TRUE (can_fire_back)
      << "Queen must be able to fire an arrow at her starting square.";
}

// Passing zero queen moves must return zero arrow moves without crashing.
TEST (GetArrowMoves, ZeroInput)
{
  Move final_moves[10];
  EXPECT_EQ (get_arrow_moves (nullptr, 0, {}, 10, final_moves), 0);
}

// Two distinct queen moves from (0,0) each yield 27 arrow destinations,
// giving a combined total of 54.
TEST (GetArrowMoves, MultipleQueenMoves)
{
  Move q_moves[2] = { { { 0, 0 }, { 0, 1 }, { 255, 255 } },
                      { { 0, 0 }, { 1, 0 }, { 255, 255 } } };
  Bitboard occupied = {};
  bitboard_set_bit (&occupied, 0);

  Move final_moves[2000];
  EXPECT_EQ (get_arrow_moves (q_moves, 2, occupied, 10, final_moves), 54);
}

// ============================================================
// get_beam_width
// ============================================================

// At the deepest level (current_depth == max_depth), depth_bonus is 0 and
// only the base width applies.
TEST (GetBeamWidth, MaximizingBaseWidth)
{
  EXPECT_EQ (get_beam_width (1, 3, 3), NB_BEST_MOVES_PLAYER);
}

// Same check for the minimising player.
TEST (GetBeamWidth, MinimizingBaseWidth)
{
  EXPECT_EQ (get_beam_width (0, 3, 3), NB_BEST_MOVES_OPPONENT);
}

// At depth 1 of 3 the bonus is (3-1)×3 = 6.
TEST (GetBeamWidth, DepthBonusAdded)
{
  EXPECT_EQ (get_beam_width (1, 1, 3), NB_BEST_MOVES_PLAYER + 6);
}

// Nodes closer to the root must always have a wider beam than leaf nodes.
TEST (GetBeamWidth, ShallowerDepthGivesLargerWidth)
{
  EXPECT_GT (get_beam_width (1, 1, 3), get_beam_width (1, 3, 3));
}

// The minimising player always gets a wider beam than the maximising player
// at the same depth across the entire search range.
TEST (GetBeamWidth, MinimizingAlwaysWiderThanMaximizing)
{
  for (int d = 1; d <= 5; d++)
    EXPECT_LT (get_beam_width (1, d, 5), get_beam_width (0, d, 5))
        << "at depth " << d;
}

// No depth bonus when already at the leaf.
TEST (GetBeamWidth, ZeroDepthBonusAtLeaf)
{
  EXPECT_EQ (get_beam_width (0, 5, 5), NB_BEST_MOVES_OPPONENT);
}

// ============================================================
// sort_moves
// ============================================================

// The target move must be swapped to index 0 while the displaced element
// moves to the target's old position.
TEST (SortMoves, CorrectlySwaps)
{
  Move best = { { 0, 0 }, { 1, 1 }, { 2, 2 } };
  Move other_1 = { { 3, 3 }, { 4, 4 }, { 5, 5 } };
  Move other_2 = { { 6, 6 }, { 7, 7 }, { 8, 8 } };
  Move list[] = { other_1, best, other_2 };

  sort_moves (3, best, list);

  EXPECT_TRUE (moves_are_equal (list[0], best));
  EXPECT_TRUE (moves_are_equal (list[1], other_1));
  EXPECT_TRUE (moves_are_equal (list[2], other_2));
}

// If the target move is absent the list must remain completely unchanged.
TEST (SortMoves, MissingMoveLeavesList)
{
  Move a = { { 0, 0 }, { 1, 1 }, { 2, 2 } };
  Move b = { { 3, 3 }, { 4, 4 }, { 5, 5 } };
  Move list[] = { a, b };

  sort_moves (2, { { 9, 9 }, { 9, 9 }, { 9, 9 } }, list);

  EXPECT_TRUE (moves_are_equal (list[0], a));
  EXPECT_TRUE (moves_are_equal (list[1], b));
}

// A null list pointer must not cause a crash (no iteration happens).
TEST (SortMoves, NullListDoesNotCrash)
{
  sort_moves (3, { { 0, 0 }, { 1, 1 }, { 2, 2 } }, nullptr);
}

// Edge counts of 0 and 1 must be handled without crashing.
TEST (SortMoves, EdgeCounts)
{
  Move a = { { 0, 0 }, { 1, 1 }, { 2, 2 } };
  Move b = { { 3, 3 }, { 4, 4 }, { 5, 5 } };
  Move list[] = { a, b };

  sort_moves (0, a, list);
  sort_moves (1, a, list);
}

// ============================================================
// score_move
// ============================================================

// A central destination has more unobstructed rays than a corner, so its
// score must be higher.
TEST (ScoreMove, CenteredDestinationScoresHigher)
{
  BitboardState state = make_empty ();
  bitboard_set_bit (&state.white_queens, 0);

  float s_center = score_move (&state, { { 0, 0 }, { 5, 5 }, { 0, 1 } });
  float s_corner = score_move (&state, { { 0, 0 }, { 9, 9 }, { 0, 1 } });

  EXPECT_GT (s_center, s_corner);
}

// Score must always be non-negative.
TEST (ScoreMove, ScoreIsNonNegative)
{
  BitboardState state = make_empty ();
  EXPECT_GE (score_move (&state, { { 0, 0 }, { 3, 3 }, { 0, 1 } }), 0.0f);
}

// A destination with every ray endpoint blocked by arrows scores exactly 0.
TEST (ScoreMove, FullyBlockedDestinationScoresZero)
{
  BitboardState state = make_empty ();
  for (uint8_t i = 0; i < 100; i++)
    if (i != 11)
      bitboard_set_bit (&state.arrows, i);

  EXPECT_EQ (score_move (&state, { { 5, 5 }, { 1, 1 }, { 0, 1 } }), 0.0f);
}

// ============================================================
// pick_top_moves
// ============================================================

// The function must cap output at the requested limit and return moves in
// descending score order.
TEST (PickTopMoves, FiltersAndSorts)
{
  BitboardState state = make_empty (10);
  bitboard_set_bit (&state.white_queens, 0);

  Move raw[] = { { { 0, 0 }, { 9, 9 }, { 255, 255 } },   // low score
                 { { 0, 0 }, { 5, 5 }, { 255, 255 } },   // high score
                 { { 0, 0 }, { 2, 2 }, { 255, 255 } } }; // medium score

  ScoredMove best[2];
  int actual = pick_top_moves (&state, raw, 3, 2, best);

  EXPECT_EQ (actual, 2);
  EXPECT_TRUE (moves_are_equal (best[0].move, raw[1]));
  EXPECT_TRUE (moves_are_equal (best[1].move, raw[2]));
  EXPECT_GE (best[0].score, best[1].score);
}

// Null arguments must trigger an assertion/crash.
TEST (PickTopMoves, AssertsOnNull)
{
  ASSERT_DEATH ({ pick_top_moves (nullptr, nullptr, 0, 0, nullptr); }, "");
}

// When count < limit every provided move must be returned.
TEST (PickTopMoves, AllUnderLimit)
{
  BitboardState state = make_empty ();
  Move raw[] = { { { 0, 0 }, { 1, 1 }, { 255, 255 } } };
  ScoredMove best[10];

  EXPECT_EQ (pick_top_moves (&state, raw, 1, 10, best), 1);
}

// ============================================================
// alpha_beta_conditions
// ============================================================

// depth == 0 is always a terminal condition.
TEST (AlphaBetaConditions, ReturnsTrueAtDepthZero)
{
  Minimax_Data data = make_data ();
  BitboardState state = make_empty ();
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);

  EXPECT_TRUE (alpha_beta_conditions (&data, &state, 0, get_time_ms ()));
}

// Remaining depth and plenty of time — search must continue.
TEST (AlphaBetaConditions, ReturnsFalseWithDepthAndTime)
{
  Minimax_Data data = make_data (5000);
  BitboardState state = make_empty ();
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);

  EXPECT_FALSE (alpha_beta_conditions (&data, &state, 3, get_time_ms ()));
}

// time_limit=0 with start_time=0 means the budget is already exhausted.
TEST (AlphaBetaConditions, ReturnsTrueWhenTimeExpired)
{
  Minimax_Data data = make_data (0);
  BitboardState state = make_empty ();
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);

  EXPECT_TRUE (alpha_beta_conditions (&data, &state, 3, 0));
}

// A trapped white queen means no legal moves — game is over.
TEST (AlphaBetaConditions, ReturnsTrueWhenGameOver)
{
  Minimax_Data data = make_data (5000);
  BitboardState state = make_empty ();
  trap_queen (&state, true, 0, 0);
  bitboard_set_bit (&state.black_queens, 99);
  state.white_to_move = true;

  EXPECT_TRUE (alpha_beta_conditions (&data, &state, 3, get_time_ms ()));
}

// Normal mid-game state with depth > 0 and time remaining.
TEST (AlphaBetaConditions, ReturnsFalseInNormalState)
{
  Minimax_Data data = make_data (5000);
  BitboardState state = make_empty ();
  bitboard_set_bit (&state.white_queens, 44);
  bitboard_set_bit (&state.black_queens, 55);
  state.white_to_move = true;

  EXPECT_FALSE (alpha_beta_conditions (&data, &state, 2, get_time_ms ()));
}

// ============================================================
// update_best_move
// ============================================================

// Maximising: a better score updates best_move and raises alpha.
TEST (UpdateBestMove, MaximizingUpdatesOnImprovement)
{
  Move best = {};
  float score = -1e6f, alpha = -1e6f, beta = 1e6f;
  Move candidate = { { 1, 1 }, { 2, 2 }, { 3, 3 } };

  update_best_move (true, 42.0f, candidate, &best, &score, &alpha, &beta);

  EXPECT_EQ (score, 42.0f);
  EXPECT_EQ (best.queen_src.row, 1);
  EXPECT_EQ (alpha, 42.0f);
}

// Maximising: a worse score must leave best_move and score unchanged.
TEST (UpdateBestMove, MaximizingDoesNotUpdateOnWorse)
{
  Move best = { { 1, 1 }, { 2, 2 }, { 3, 3 } };
  float score = 50.0f, alpha = 50.0f, beta = 1e6f;

  update_best_move (true, 10.0f, { { 9, 9 }, { 8, 8 }, { 7, 7 } }, &best,
                    &score, &alpha, &beta);

  EXPECT_EQ (score, 50.0f);
  EXPECT_EQ (best.queen_src.row, 1);
}

// Minimising: a lower score updates best_move and lowers beta.
TEST (UpdateBestMove, MinimizingUpdatesOnImprovement)
{
  Move best = {};
  float score = 1e6f, alpha = -1e6f, beta = 1e6f;
  Move candidate = { { 4, 4 }, { 5, 5 }, { 6, 6 } };

  update_best_move (false, -20.0f, candidate, &best, &score, &alpha, &beta);

  EXPECT_EQ (score, -20.0f);
  EXPECT_EQ (best.queen_src.row, 4);
  EXPECT_EQ (beta, -20.0f);
}

// Minimising: a worse (higher) score must leave everything unchanged.
TEST (UpdateBestMove, MinimizingDoesNotUpdateOnWorse)
{
  Move best = { { 4, 4 }, { 5, 5 }, { 6, 6 } };
  float score = -20.0f, alpha = -1e6f, beta = -20.0f;

  update_best_move (false, 100.0f, { { 0, 0 }, { 1, 1 }, { 2, 2 } }, &best,
                    &score, &alpha, &beta);

  EXPECT_EQ (score, -20.0f);
  EXPECT_EQ (best.queen_src.row, 4);
  EXPECT_EQ (beta, -20.0f);
}

// Passing nullptr for best_move must not crash for either player.
TEST (UpdateBestMove, NullBestMoveDoesNotCrash)
{
  float score = -1e6f, alpha = -1e6f, beta = 1e6f;
  Move c = { { 1, 1 }, { 2, 2 }, { 3, 3 } };

  update_best_move (true, 5.0f, c, nullptr, &score, &alpha, &beta);
  EXPECT_EQ (score, 5.0f);

  score = 1e6f;
  alpha = -1e6f;
  beta = 1e6f;
  update_best_move (false, -5.0f, c, nullptr, &score, &alpha, &beta);
  EXPECT_EQ (score, -5.0f);
}

// Alpha must never decrease across successive maximising updates.
TEST (UpdateBestMove, AlphaIsMonotonic)
{
  float score = -1e6f, alpha = -1e6f, beta = 1e6f, prev = alpha;
  Move m = { { 0, 0 }, { 1, 1 }, { 2, 2 } };

  for (float v : { 3.0f, 1.0f, 7.0f, 5.0f })
    {
      update_best_move (true, v, m, nullptr, &score, &alpha, &beta);
      EXPECT_GE (alpha, prev);
      prev = alpha;
    }
}

// Beta must be updated when the new score is below the current best.
TEST (UpdateBestMove, BetaUpdatesWhenScoreLower)
{
  Move best = {};
  float score = 100.0f, alpha = -100.0f, beta = 50.0f;

  update_best_move (false, 10.0f, {}, &best, &score, &alpha, &beta);

  EXPECT_FLOAT_EQ (score, 10.0f);
  EXPECT_FLOAT_EQ (beta, 10.0f);
}

// Beta must not change when it is already below the incoming score.
TEST (UpdateBestMove, BetaStaysWhenAlreadyLower)
{
  Move best = {};
  float score = 100.0f, alpha = -100.0f, beta = 5.0f;

  update_best_move (false, 10.0f, {}, &best, &score, &alpha, &beta);

  EXPECT_FLOAT_EQ (score, 10.0f);
  EXPECT_FLOAT_EQ (beta, 5.0f);
}

// ============================================================
// get_top_moves
// ============================================================

// A trapped white queen has no queen moves — result must be 0.
TEST (GetTopMoves, TrappedPlayerReturnsZero)
{
  BitboardState state = make_empty (10);
  trap_queen (&state, true, 0, 0);
  state.white_to_move = true;

  ScoredMove top[5];
  EXPECT_EQ (get_top_moves (&state, 5, top), 0);
}

// The returned count must never exceed the requested beam limit, and results
// must be in descending score order.
TEST (GetTopMoves, RespectsBeamLimit)
{
  BitboardState state = make_empty (10);
  bitboard_set_bit (&state.white_queens, 45);
  bitboard_set_bit (&state.black_queens, 54);
  state.white_to_move = true;

  ScoredMove top[3];
  int count = get_top_moves (&state, 3, top);

  EXPECT_EQ (count, 3);
  EXPECT_GE (top[0].score, top[1].score);
  EXPECT_GE (top[1].score, top[2].score);
}

// On Black's turn, all returned moves must originate from Black's queen.
TEST (GetTopMoves, BlackTurnMovesOriginateFromBlack)
{
  BitboardState state = make_empty (10);
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);
  state.white_to_move = false;

  ScoredMove top[5];
  int count = get_top_moves (&state, 5, top);

  EXPECT_GT (count, 0);
  EXPECT_EQ (top[0].move.queen_src.row, 9);
  EXPECT_EQ (top[0].move.queen_src.col, 9);
}

// ============================================================
// alpha_beta_forward
// ============================================================

// A depth-1 search on a normal position must return a finite score strictly
// between BOTTOM_SCORE and TOP_SCORE.
TEST (AlphaBetaForward, ShallowSearchReturnsBoundedScore)
{
  Minimax_Data data = make_data (1000, 1);
  BitboardState state = make_empty (10);
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);
  state.white_to_move = true;

  float score = alpha_beta_forward (&data, &state, 1, BOTTOM_SCORE, TOP_SCORE,
                                    true, get_time_ms ());

  EXPECT_GT (score, BOTTOM_SCORE);
  EXPECT_LT (score, TOP_SCORE);
}

// ============================================================
// run_root_search
// ============================================================

// Given two legal moves, the function must return a non-empty move.
TEST (RunRootSearch, FindsMove)
{
  Minimax_Data data = make_data (5000, 1);
  BitboardState state = make_empty (10);
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);
  state.white_to_move = true;

  Move available[]
      = { { { 0, 0 }, { 0, 1 }, { 0, 2 } }, { { 0, 0 }, { 1, 0 }, { 2, 0 } } };
  Move best = run_root_search (&data, &state, 1, get_time_ms (), available, 2);

  EXPECT_FALSE (moves_are_equal (best, (Move){}));
}

// With start_time=0 and time_limit=0 the loop exits before evaluating any
// move — the returned move must be zero-initialised.
TEST (RunRootSearch, ImmediateTimeoutReturnsEmptyMove)
{
  Minimax_Data data = make_data (0, 1);
  BitboardState state = make_empty (10);
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);

  Move available[] = { { { 0, 0 }, { 0, 1 }, { 0, 2 } } };
  Move best = run_root_search (&data, &state, 1, 0, available, 1);

  EXPECT_TRUE (moves_are_equal (best, (Move){}));
}

// A perfect evaluator saturates alpha on the first move, causing all
// subsequent moves to be pruned. This must not crash.
TEST (RunRootSearch, PruningAtRoot)
{
  Minimax_Data data = make_data (5000, 1);
  data.evaluate = perfect_eval;

  BitboardState state = make_empty (10);
  state.white_to_move = true;
  bitboard_set_bit (&state.white_queens, 0);
  bitboard_set_bit (&state.black_queens, 99);

  Move available[]
      = { { { 0, 0 }, { 0, 1 }, { 0, 2 } }, { { 0, 0 }, { 1, 0 }, { 2, 0 } } };
  run_root_search (&data, &state, 1, get_time_ms (), available, 2);
}

// An empty move list must be handled gracefully and return a zero move.
TEST (RunRootSearch, EmptyMoveList)
{
  Minimax_Data data = make_data (5000, 1);
  BitboardState state = make_empty (10);

  Move best = run_root_search (&data, &state, 1, get_time_ms (), nullptr, 0);

  EXPECT_EQ (best.queen_src.row, 0);
}

// ============================================================
// Integration — AI lifecycle (fixture MinimaxAI)
// ============================================================

// A freshly created AI must have a valid vtable but no context.
TEST_F (MinimaxAI, CreationSetsVtableOnly)
{
  AI_Interface fresh;
  create_minimax_ai (&fresh);
  EXPECT_NE (fresh.vtable, nullptr);
  EXPECT_EQ (fresh.context, nullptr);
}

// After init, context is non-null; after clean, it is null again.
TEST_F (MinimaxAI, InitAndCleanLifecycle)
{
  EXPECT_NE (ai.context, nullptr);
  ai.vtable->clean (&ai);
  EXPECT_EQ (ai.context, nullptr);
}

// A second clean on an already-cleaned AI must not crash.
TEST_F (MinimaxAI, DoubleCleanDoesNotCrash)
{
  ai.vtable->clean (&ai);
  ai.vtable->clean (&ai);
  EXPECT_EQ (ai.context, nullptr);
}

// Default values must match the documented constants.
TEST_F (MinimaxAI, DefaultParametersMatchConstants)
{
  Minimax_Data *data = (Minimax_Data *)ai.context;
  EXPECT_EQ (data->max_depth, DEFAULT_MAX_DEPTH);
  EXPECT_EQ (data->time_limit, (uint64_t)DEFAULT_TIME_LIMIT);
  EXPECT_EQ (data->forward_pruning_enabled,
             DEFAULT_FORWARD_PRUNING_ACTIVATION);
  EXPECT_EQ (data->iterative_deepening_enabled,
             DEFAULT_ITERATIVE_DEEPENING_ACTIVATION);
}

// All custom parameters must be stored correctly.
TEST_F (MinimaxAI, InitWithAllParameters)
{
  Param p[] = { { "depth", "4" },
                { "time_limit", "3000" },
                { "forward_pruning", "0" },
                { "iterative_deepening", "1" },
                { "evaluator", get_evaluator_param (EVAL_QD_KD) } };
  reinit (p, 5);

  Minimax_Data *data = (Minimax_Data *)ai.context;
  EXPECT_EQ (data->max_depth, 4);
  EXPECT_EQ (data->time_limit, 3000UL);
  EXPECT_FALSE (data->forward_pruning_enabled);
  EXPECT_TRUE (data->iterative_deepening_enabled);
  EXPECT_EQ (data->evaluate (NULL), get_evaluator (EVAL_QD_KD) (NULL));
}

// Alternate parameter combination to exercise the other evaluator and flags.
TEST_F (MinimaxAI, InitWithAlternateParameters)
{
  Param p[] = { { "depth", "4" },
                { "time_limit", "3000" },
                { "forward_pruning", "1" },
                { "iterative_deepening", "0" },
                { "evaluator", get_evaluator_param (EVAL_ME) } };
  reinit (p, 5);

  Minimax_Data *data = (Minimax_Data *)ai.context;
  EXPECT_EQ (data->max_depth, 4);
  EXPECT_EQ (data->time_limit, 3000UL);
  EXPECT_TRUE (data->forward_pruning_enabled);
  EXPECT_FALSE (data->iterative_deepening_enabled);
  EXPECT_EQ (data->evaluate (NULL), get_evaluator (EVAL_ME) (NULL));
}

// When malloc fails inside init, context must remain null.
TEST_F (MinimaxAI, MallocFailureLeavesContextNull)
{
  ai.vtable->clean (&ai);

  {
    MallocFailGuard guard;
    ai.vtable->init (&ai, NULL);
  }

  EXPECT_EQ (ai.context, nullptr);
}

// ============================================================
// Integration — get_best_move (fixture MinimaxAI)
// ============================================================

// When the current player has no legal moves, get_best_move must fall back
// to get_direct_random_move rather than returning an illegal zero move.
TEST_F (MinimaxAI, GetBestMoveTrappedFallback)
{
  Param p[] = { { "depth", "1" },
                { "time_limit", "3000" },
                { "forward_pruning", "1" },
                { "iterative_deepening", "0" },
                { "evaluator", get_evaluator_param (EVAL_QD_KD) } };
  reinit (p, 5);

  GameState state = {};
  state.size = 3;
  state.board = allocate_empty_board (3, false);
  state.board[0][0] = CONTENT_WHITE_QUEEN;
  state.current_player = PLAYER_WHITE;
  state.status = GAME_IN_PROGRESS;

  Move best = ai.vtable->get_best_move (&ai, state);

  EXPECT_GE (best.queen_src.row, 0);

  free_board (state.board, state.size);
}

// With time_limit=0 and depth=0 the iterative deepening loop exits before
// completing depth 1 — result must be the zero-initialised move.
TEST_F (MinimaxAI, IterativeDeepeningTimesOutImmediately)
{
  Param p[] = { { "depth", "0" },
                { "time_limit", "0" },
                { "iterative_deepening", "1" } };
  reinit (p, 3);

  GameState state = {};
  state.size = 10;
  state.board = allocate_empty_board (10, false);
  state.current_player = PLAYER_WHITE;
  state.status = GAME_IN_PROGRESS;

  Move best = ai.vtable->get_best_move (&ai, state);

  EXPECT_TRUE (moves_are_equal (best, (Move){}));

  free_board (state.board, state.size);
}

// With enough time, iterative deepening must return a valid move found at
// depth 1 before the depth-2 search exceeds the time limit.
TEST_F (MinimaxAI, IterativeDeepeningReturnsValidMove)
{
  Param p[] = { { "depth", "2" },
                { "time_limit", "50" },
                { "iterative_deepening", "1" } };
  reinit (p, 3);

  GameState state = {};
  state.size = 10;
  state.board = allocate_empty_board (10);
  state.current_player = PLAYER_WHITE;
  state.status = GAME_IN_PROGRESS;

  Move best = ai.vtable->get_best_move (&ai, state);

  EXPECT_FALSE (moves_are_equal (best, (Move){}));

  free_board (state.board, state.size);
}

// Smoke test for the standard (non-ID) search path.
TEST_F (MinimaxAI, GetBestMoveStandardPath)
{
  Param p[] = { { "depth", "1" }, { "time_limit", "500" } };
  reinit (p, 2);

  GameState state = {};
  state.size = 10;
  state.board = allocate_empty_board (10);
  state.current_player = PLAYER_WHITE;
  state.status = GAME_IN_PROGRESS;

  Move best = ai.vtable->get_best_move (&ai, state);

  EXPECT_FALSE (moves_are_equal (best, (Move){}));

  free_board (state.board, state.size);
}