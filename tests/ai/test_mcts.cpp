#include <cmath>
#include <gtest/gtest.h>

#define MCTS_TEST_MODE

extern "C"
{
#include "../../src/ai/mcts/mcts_includes/mcts_internal.h"
#include "ai_factory.h"
#include "bitboard_state.h"
#include "mcts.h"
#include "mcts_types.h"
#include "uct.h"
#include "utils.h"
}

// ============================================================
// Helpers
// ============================================================

static BitboardState
make_open_state (uint8_t size = 10)
{
  BitboardState s = create_empty_state (size);
  bitboard_set_bit (&s.white_queens, 0);
  bitboard_set_bit (&s.black_queens, (uint8_t)(size * size - 1));
  s.white_to_move = true;
  return s;
}

// Surrounds the queen at (r,c) with arrows on every adjacent square so it
// has no legal moves.
static void
trap_at (BitboardState *s, bool white, uint8_t r, uint8_t c)
{
  uint8_t sz = s->size;
  if (white)
    bitboard_set_bit (&s->white_queens, (uint8_t)(r * sz + c));
  else
    bitboard_set_bit (&s->black_queens, (uint8_t)(r * sz + c));

  for (int dr = -1; dr <= 1; dr++)
    for (int dc = -1; dc <= 1; dc++)
      {
        if (dr == 0 && dc == 0)
          continue;
        int nr = r + dr, nc = c + dc;
        if (nr >= 0 && nr < sz && nc >= 0 && nc < sz)
          bitboard_set_bit (&s->arrows, (uint8_t)(nr * sz + nc));
      }
}

// Allocates a 10×10 GameState board with one white and one black queen at
// fixed positions. Caller is responsible for calling free_gs_board.
static GameState
make_game_state (uint8_t white_r, uint8_t white_c, uint8_t black_r,
                 uint8_t black_c)
{
  const uint8_t SIZE = 10;
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * SIZE);
  for (int i = 0; i < SIZE; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * SIZE);
      for (int j = 0; j < SIZE; j++)
        cc[i][j] = CONTENT_EMPTY;
    }
  cc[white_r][white_c] = CONTENT_WHITE_QUEEN;
  cc[black_r][black_c] = CONTENT_BLACK_QUEEN;
  return { SIZE, cc, PLAYER_WHITE, GAME_IN_PROGRESS, 0, {}, {} };
}

static void
free_gs_board (GameState *state)
{
  for (int i = 0; i < state->size; i++)
    free (state->board[i]);
  free (state->board);
}

// Builds a root MCTS_Node for the given state (no parent, no move).
static MCTS_Node *
make_root (const BitboardState *state)
{
  return alloc_mcts_node (NULL, state, NULL);
}

// Allocates and returns a UCT ISelector. Caller must call free_selector.
static ISelector *
make_uct_selector ()
{
  ISelector *sel = (ISelector *)malloc (sizeof (ISelector));
  create_select_uct (sel, 1.414f);
  return sel;
}

static void
free_selector (ISelector *sel)
{
  if (sel->vtable && sel->vtable->clean)
    sel->vtable->clean (sel);
  free (sel);
}

// ============================================================
// Fixture — MCTSAI
// (which improves with each iteration)
// Manages the full AI lifecycle (create → init → [test] → clean).
// ============================================================

class MCTSAI : public ::testing::Test
{
protected:
  AI_Interface ai;

  void
  SetUp () override
  {
    create_mcts_ai (&ai);
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
    ConfigMap cm = { params, count };
    ai.vtable->init (&ai, &cm);
  }
};

// ============================================================
// is_move_null
// ============================================================

// A move where src == dst (both zero) is the canonical null move.
TEST (IsMoveNull, TrueForZeroMove)
{
  Move m = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
  EXPECT_TRUE (is_move_null (&m));
}

// A move with a distinct destination is not null.
TEST (IsMoveNull, FalseForValidMove)
{
  Move m = { { 0, 0 }, { 1, 1 }, { 2, 2 } };
  EXPECT_FALSE (is_move_null (&m));
}

// Only the queen src/dst positions are checked — arrow position is irrelevant.
TEST (IsMoveNull, OnlyQueenPositionsChecked)
{
  Move m = { { 3, 3 }, { 3, 3 }, { 5, 5 } };
  EXPECT_TRUE (is_move_null (&m));
}

// ============================================================
// alloc_mcts_node
// ============================================================

// A root node (no parent, no move) must be zero-initialised and carry the
// given state directly.
TEST (AllocMctsNode, RootNodeHasCorrectInitialValues)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  ASSERT_NE (root, nullptr);
  EXPECT_EQ (root->parent, nullptr);
  EXPECT_EQ (root->first_child, nullptr);
  EXPECT_EQ (root->next_sibling, nullptr);
  EXPECT_EQ (root->visit_count, 0);
  EXPECT_FLOAT_EQ (root->score, 0.0f);
  EXPECT_FALSE (root->is_fully_expanded);
  EXPECT_EQ (root->untried_moves, nullptr);
  EXPECT_EQ (root->state.size, state.size);
  EXPECT_EQ (root->state.white_to_move, state.white_to_move);

  free_mcts_tree (root);
}

// A child node must point back to its parent and reflect the applied move
// by toggling the active player.
TEST (AllocMctsNode, ChildLinksParentAndAppliesMove)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  Move moves[get_move_limit (&state)];
  int count = get_legal_moves (&state, moves);
  ASSERT_GT (count, 0);

  MCTS_Node *child = alloc_mcts_node (&moves[0], NULL, root);
  ASSERT_NE (child, nullptr);
  EXPECT_EQ (child->parent, root);
  EXPECT_NE (child->state.white_to_move, state.white_to_move);

  free (child);
  free_mcts_tree (root);
}

// ============================================================
// expand
// ============================================================

// The first expansion of a fresh root must create exactly one child linked
// to the root.
TEST (Expand, FirstExpandCreatesOneChild)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  MCTS_Node *child = expand (root);

  EXPECT_NE (child, root);
  EXPECT_EQ (child->parent, root);
  EXPECT_NE (root->first_child, nullptr);

  free_mcts_tree (root);
}

// Expanding a node that is already fully expanded must return the node itself.
TEST (Expand, FullyExpandedReturnsParent)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);
  root->is_fully_expanded = true;

  EXPECT_EQ (expand (root), root);

  free_mcts_tree (root);
}

// Expanding a game-over node must return the node itself without crashing.
TEST (Expand, GameOverReturnsParent)
{
  BitboardState state = create_empty_state (10);
  trap_at (&state, true, 0, 0);
  bitboard_set_bit (&state.black_queens, 99);
  state.white_to_move = true;

  MCTS_Node *root = make_root (&state);
  EXPECT_EQ (expand (root), root);
  free_mcts_tree (root);
}

// Repeatedly expanding a root must eventually mark it as fully expanded and
// free untried_moves.
TEST (Expand, RepeatedExpandFullyExpandsNode)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  Move dummy_buffer[get_move_limit (&state)];
  int legal = get_legal_moves (&state, dummy_buffer);

  for (int i = 0; i < legal + 10 && !root->is_fully_expanded; i++)
    {
      expand (root);
    }

  EXPECT_TRUE (root->is_fully_expanded);
  EXPECT_EQ (root->untried_moves, nullptr);

  free_mcts_tree (root);
}

// The number of children after full expansion must equal the number of legal
// moves.
TEST (Expand, ChildCountMatchesLegalMoveCount)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  Move moves[get_move_limit (&state)];
  int legal = get_legal_moves (&state, moves);

  while (!root->is_fully_expanded)
    expand (root);

  int child_count = 0;
  for (MCTS_Node *c = root->first_child; c; c = c->next_sibling)
    child_count++;

  EXPECT_EQ (child_count, legal);
  free_mcts_tree (root);
}

// ============================================================
// backpropagate
// ============================================================

// A single-node backpropagation must increment visit_count and add the score.
TEST (Backpropagate, SingleNodeUpdatesVisitAndScore)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  backpropagate (root, 1.0f);

  EXPECT_EQ (root->visit_count, 1);
  EXPECT_FLOAT_EQ (root->score, 1.0f);

  free_mcts_tree (root);
}

// The score must alternate (1 → 0 → 1) as it propagates up the chain because
// each level represents the opposite player.
TEST (Backpropagate, ScoreAlternatesUpChain)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  Move moves[get_move_limit (&state)];
  get_legal_moves (&state, moves);

  MCTS_Node *child = alloc_mcts_node (&moves[0], NULL, root);
  root->first_child = child;
  MCTS_Node *grandchild = alloc_mcts_node (&moves[1], NULL, child);
  grandchild->state = apply_move_bitboard (&child->state, moves[1]);
  child->first_child = grandchild;

  backpropagate (grandchild, 1.0f);

  EXPECT_FLOAT_EQ (grandchild->score, 1.0f);
  EXPECT_FLOAT_EQ (child->score, 0.0f);
  EXPECT_FLOAT_EQ (root->score, 1.0f);
  EXPECT_EQ (grandchild->visit_count, 1);
  EXPECT_EQ (child->visit_count, 1);
  EXPECT_EQ (root->visit_count, 1);

  free_mcts_tree (root);
}

// Multiple backpropagations on the same node must accumulate correctly.
TEST (Backpropagate, AccumulatesAcrossMultipleCalls)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  backpropagate (root, 1.0f);
  backpropagate (root, 0.0f);
  backpropagate (root, 1.0f);

  EXPECT_EQ (root->visit_count, 3);
  EXPECT_FLOAT_EQ (root->score, 2.0f);

  free_mcts_tree (root);
}

// ============================================================
// get_mcts_win_probability
// ============================================================

// The probability must always lie in [0, 1].
TEST (GetMctsWinProbability, AlwaysInUnitRange)
{
  BitboardState state = make_open_state ();
  float p = get_mcts_win_probability (&state, true);
  EXPECT_GE (p, 0.0f);
  EXPECT_LE (p, 1.0f);
}

// A strictly symmetric position (diagonal corners) must yield 0.5.
TEST (GetMctsWinProbability, SymmetricPositionIsHalf)
{
  BitboardState state = create_empty_state (10);
  bitboard_set_bit (&state.white_queens, 90); // (9,0)
  bitboard_set_bit (&state.black_queens, 9);  // (0,9)
  state.white_to_move = true;

  EXPECT_NEAR (get_mcts_win_probability (&state, true), 0.5f, 0.01f);
}

// When the perspective player flips, the probabilities must be on opposite
// sides of 0.5.
TEST (GetMctsWinProbability, FlippedPlayerFlipsProbability)
{
  BitboardState state = create_empty_state (10);
  bitboard_set_bit (&state.white_queens, 55); // center — mobility advantage
  bitboard_set_bit (&state.black_queens, 0);  // corner

  float p_white = get_mcts_win_probability (&state, true);
  float p_black = get_mcts_win_probability (&state, false);

  EXPECT_GT (p_white, 0.5f);
  EXPECT_LT (p_black, 0.5f);
}

// The result must be clamped and remain in [0, 1] even with extreme mobility
// differences.
TEST (GetMctsWinProbability, ClampedToUnitInterval)
{
  BitboardState state = create_empty_state (10);
  bitboard_set_bit (&state.white_queens, 44);
  bitboard_set_bit (&state.white_queens, 45);
  bitboard_set_bit (&state.white_queens, 54);
  bitboard_set_bit (&state.white_queens, 55);
  bitboard_set_bit (&state.black_queens, 0);
  for (uint8_t i = 1; i < 20; i++)
    bitboard_set_bit (&state.arrows, i);

  float p = get_mcts_win_probability (&state, true);
  EXPECT_GE (p, 0.0f);
  EXPECT_LE (p, 1.0f);
}

// ============================================================
// rollout
// ============================================================

// The rollout result must be in [0, 1].
TEST (Rollout, ResultInUnitRange)
{
  BitboardState state = make_open_state ();
  float r = rollout (state);
  EXPECT_GE (r, 0.0f);
  EXPECT_LE (r, 1.0f);
}

// When the starting player is already trapped, the rollout detects immediate
// game-over and must return 0.0 (starting player loses).
TEST (Rollout, AlreadyLostReturnsZero)
{
  BitboardState state = create_empty_state (10);
  trap_at (&state, true, 0, 0);
  bitboard_set_bit (&state.black_queens, 99);
  state.white_to_move = true;

  EXPECT_FLOAT_EQ (rollout (state), 0.0f);
}

// A normal mid-game position must not crash and must not return NaN.
TEST (Rollout, OpenPositionDoesNotCrash)
{
  BitboardState state = create_empty_state (10);
  bitboard_set_bit (&state.white_queens, 3);
  bitboard_set_bit (&state.white_queens, 6);
  bitboard_set_bit (&state.black_queens, 93);
  bitboard_set_bit (&state.black_queens, 96);
  state.white_to_move = true;

  float r = rollout (state);
  EXPECT_FALSE (std::isnan (r));
  EXPECT_GE (r, 0.0f);
  EXPECT_LE (r, 1.0f);
}

// ============================================================
// select_node
// ============================================================

// An unexpanded root must be returned immediately (no children to dive into).
TEST (SelectNode, ReturnsRootWhenNotExpanded)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);
  ISelector *sel = make_uct_selector ();

  EXPECT_EQ (select_node (sel, root), root);

  free_mcts_tree (root);
  free_selector (sel);
}

// Selection must dive into a fully expanded parent and stop at the first
// unexpanded child.
TEST (SelectNode, DivesIntoFullyExpandedNode)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);
  ISelector *sel = make_uct_selector ();

  MCTS_Node *child = expand (root);
  child->visit_count = 10;
  child->score = 8.0f;
  root->visit_count = 11;
  root->is_fully_expanded = true;

  EXPECT_EQ (select_node (sel, root), child);

  free_mcts_tree (root);
  free_selector (sel);
}

// select_node must never return nullptr.
TEST (SelectNode, NeverReturnsNull)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);
  ISelector *sel = make_uct_selector ();

  EXPECT_NE (select_node (sel, root), nullptr);

  free_mcts_tree (root);
  free_selector (sel);
}

// ============================================================
// merge_and_get_best
// ============================================================

// With a single thread, the child with the highest visit count must be chosen.
TEST (MergeAndGetBest, SingleThreadPicksMostVisited)
{
  BitboardState state = make_open_state ();
  Move moves[get_move_limit (&state)];
  get_legal_moves (&state, moves);

  MCTS_Node *root = make_root (&state);
  MCTS_Node *child_a = alloc_mcts_node (&moves[0], NULL, root);
  child_a->visit_count = 5;
  MCTS_Node *child_b = alloc_mcts_node (&moves[1], NULL, root);
  child_b->visit_count = 20;
  child_b->next_sibling = child_a;
  root->first_child = child_b;

  MCTS_Data data = {};
  data.roots = &root;
  Move best = merge_and_get_best (&data, 1);

  EXPECT_EQ (best.queen_src.row, moves[1].queen_src.row);
  EXPECT_EQ (best.queen_dst.row, moves[1].queen_dst.row);

  free (child_a);
  free (child_b);
  free (root);
}

// A root with no children must return the zero move.
TEST (MergeAndGetBest, EmptyRootReturnsZeroMove)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);
  MCTS_Data data = {};
  data.roots = &root;

  Move best = merge_and_get_best (&data, 1);

  EXPECT_TRUE (is_move_null (&best));
  free_mcts_tree (root);
}

// With two threads, visit counts for the same move must be combined before
// selecting the winner (moves[0]: 3+50=53 vs moves[1]: 10+5=15).
TEST (MergeAndGetBest, TwoThreadsCombineVisits)
{
  BitboardState state = make_open_state ();
  Move moves[get_move_limit (&state)];
  get_legal_moves (&state, moves);

  MCTS_Node *root0 = make_root (&state);
  MCTS_Node *c0a = alloc_mcts_node (&moves[0], NULL, root0);
  c0a->visit_count = 3;
  MCTS_Node *c0b = alloc_mcts_node (&moves[1], NULL, root0);
  c0b->visit_count = 10;
  c0b->next_sibling = c0a;
  root0->first_child = c0b;

  MCTS_Node *root1 = make_root (&state);
  MCTS_Node *c1a = alloc_mcts_node (&moves[0], NULL, root1);
  c1a->visit_count = 50;
  MCTS_Node *c1b = alloc_mcts_node (&moves[1], NULL, root1);
  c1b->visit_count = 5;
  c1b->next_sibling = c1a;
  root1->first_child = c1b;

  MCTS_Node *roots[2] = { root0, root1 };
  MCTS_Data data = {};
  data.roots = roots;

  Move best = merge_and_get_best (&data, 2);

  EXPECT_EQ (best.queen_src.row, moves[0].queen_src.row);
  EXPECT_EQ (best.queen_dst.row, moves[0].queen_dst.row);

  free (c0a);
  free (c0b);
  free (root0);
  free (c1a);
  free (c1b);
  free (root1);
}

// ============================================================
// free_mcts_tree
// ============================================================

// Freeing nullptr must be a no-op.
TEST (FreeMctsTree, NullDoesNotCrash) { free_mcts_tree (nullptr); }

// A single node must be freed without crashing.
TEST (FreeMctsTree, SingleNode)
{
  BitboardState state = make_open_state ();
  free_mcts_tree (make_root (&state));
}

// A partially expanded tree must be freed without leaks (run under ASan).
TEST (FreeMctsTree, PartiallyExpandedTree)
{
  BitboardState state = make_open_state ();
  MCTS_Node *root = make_root (&state);

  for (int i = 0; i < 10 && !root->is_fully_expanded; i++)
    expand (root);

  free_mcts_tree (root);
}

// ============================================================
// Integration — AI lifecycle (fixture MCTSAI)
// ============================================================

// A freshly created AI must have a valid vtable but no context.
TEST_F (MCTSAI, CreationSetsVtableOnly)
{
  AI_Interface fresh;
  create_mcts_ai (&fresh);
  EXPECT_NE (fresh.vtable, nullptr);
  EXPECT_EQ (fresh.context, nullptr);
}

// After init, context is non-null; after clean, it is null again.
TEST_F (MCTSAI, InitAndCleanLifecycle)
{
  EXPECT_NE (ai.context, nullptr);
  ai.vtable->clean (&ai);
  EXPECT_EQ (ai.context, nullptr);
}

TEST_F (MCTSAI, DoubleCleanIsSafe)
{
  ai.vtable->clean (&ai);
  EXPECT_EQ (ai.context, nullptr);
  ai.vtable->clean (&ai);
  EXPECT_EQ (ai.context, nullptr);
}

// Default parameters must match the documented constants.
TEST_F (MCTSAI, DefaultParametersMatchConstants)
{
  MCTS_Data *data = (MCTS_Data *)ai.context;
  EXPECT_EQ (data->time_constraint, DEFAULT_TIME_CONSTRAINT);
  EXPECT_EQ (data->max_iterations, DEFAULT_MAX_ITERATIONS);
  EXPECT_EQ (data->max_workers, DEFAULT_MAX_WORKERS);
  EXPECT_NE (data->strategy_select, nullptr);
}

// All custom parameters must be stored correctly after init.
TEST_F (MCTSAI, CustomParametersAreApplied)
{
  Param p[] = { { "time_limit", "2000" },
                { "max_workers", "2" },
                { "max_iterations", "500" } };
  reinit (p, 3);

  MCTS_Data *data = (MCTS_Data *)ai.context;
  EXPECT_EQ (data->time_constraint, 2000ULL);
  EXPECT_EQ (data->max_workers, 2);
  EXPECT_EQ (data->max_iterations, 500U);
}

// ============================================================
// Integration — get_best_move (fixture MCTSAI)
// ============================================================

// On a standard open position, get_best_move must return a move originating
// from the white queen's starting square.
TEST_F (MCTSAI, GetBestMoveReturnsLegalMove)
{
  Param p = { "time_limit", "25" };
  reinit (&p, 1);

  GameState state = make_game_state (9, 6, 0, 3);
  Move best = ai.vtable->get_best_move (&ai, state);

  EXPECT_EQ (best.queen_src.row, 9);
  EXPECT_EQ (best.queen_src.col, 6);

  free_gs_board (&state);
}

// With only one legal path available, MCTS must consistently find it within
// the time budget.
TEST_F (MCTSAI, FindsOnlyLegalMove)
{
  Param p = { "time_limit", "25" };
  reinit (&p, 1);

  const uint8_t SIZE = 10;
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * SIZE);
  for (int i = 0; i < SIZE; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * SIZE);
      for (int j = 0; j < SIZE; j++)
        cc[i][j] = CONTENT_ARROW;
    }
  cc[9][0] = CONTENT_WHITE_QUEEN;
  cc[9][1] = CONTENT_EMPTY;
  cc[9][2] = CONTENT_EMPTY;
  cc[0][0] = CONTENT_BLACK_QUEEN;
  cc[0][1] = CONTENT_EMPTY;

  GameState state = { SIZE, cc, PLAYER_WHITE, GAME_IN_PROGRESS, 50, {}, {} };
  unsigned long t0 = get_time_ms ();
  Move best = ai.vtable->get_best_move (&ai, state);
  unsigned long elapsed = get_time_ms () - t0;

  EXPECT_LT (elapsed, 1100UL);
  EXPECT_EQ (best.queen_src.row, 9);
  EXPECT_EQ (best.queen_src.col, 0);

  free_gs_board (&state);
}

// With the same RNG seed, single-threaded MCTS must return the same move
// twice in a row (deterministic replay).
TEST_F (MCTSAI, DeterministicWithSameSeed)
{
  Param p[] = { { "time_limit", "25" }, { "max_workers", "1" } };
  reinit (p, 2);

  GameState state = make_game_state (9, 6, 0, 3);

  seed_xoshiro (0);
  Move m1 = ai.vtable->get_best_move (&ai, state);

  reinit (p, 2);
  seed_xoshiro (0);
  Move m2 = ai.vtable->get_best_move (&ai, state);

  EXPECT_EQ (m1.queen_dst.row, m2.queen_dst.row);
  EXPECT_EQ (m1.queen_dst.col, m2.queen_dst.col);

  free_gs_board (&state);
}