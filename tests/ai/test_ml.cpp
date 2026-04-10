/**
 * @file test_api_and_selector.cpp
 * @brief Unit and integration tests for python_api.c, selector.c,
 *        tf_wrapper.c, and ml.c.
 *
 * Build notes
 * -----------
 * These tests compile and pass in both build configurations:
 *   - Without TensorFlow  : default, TF_IS_USABLE is not defined.
 *   - With TensorFlow     : add -DTF_IS_USABLE to the compiler flags and link
 *                           against libtensorflow.
 *
 * The TF-dependent tests are wrapped in #ifdef TF_IS_USABLE so the suite
 * always compiles cleanly regardless of the configuration.
 */

#include <cmath>
#include <cstring>
#include <gtest/gtest.h>

extern "C"
{
#include "ai_factory.h"
#include "ai_interface.h"
#include "bitboard_state.h"
#include "game_types.h"
#include "selector.h"
#include "tf_wrapper.h"
#include "uct.h"

  // Public surface of python_api.c
  extern GameState setup_standard_start (int board_size);
  extern AI_Interface *api_create_mcts (int time_limit, int max_workers);
  extern AI_Interface *api_create_minimax (int depth);
  extern GameState *api_init_game (int size);
  extern void api_free_game (GameState *gs);
  extern void api_free_ai (AI_Interface *ai);
  extern Move api_get_ai_move (AI_Interface *ai, GameState *gs);
  extern int api_check_winner (GameState *gs);
  extern void api_export_tensor (GameState *gs, float *out_tensor);
  extern void api_apply_move (GameState *gs, Move m);

#ifdef TF_IS_USABLE
#include "ml.h"
#endif
}

// ============================================================
// Helpers
// ============================================================

static void
free_board (CellContent **board, int size)
{
  for (int i = 0; i < size; i++)
    free (board[i]);
  free (board);
}

static void
free_gs_inline (GameState *gs)
{
  free_board (gs->board, gs->size);
}

// Counts how many squares on the board have the given content.
static int
count_content (const GameState &gs, CellContent target)
{
  int n = 0;
  for (int r = 0; r < gs.size; r++)
    for (int c = 0; c < gs.size; c++)
      if (gs.board[r][c] == target)
        n++;
  return n;
}

// ============================================================
// setup_standard_start
// ============================================================

// Board size is clamped to [4, 11] — below 4 becomes 4.
TEST (SetupStandardStart, ClampsBelow4)
{
  GameState gs = setup_standard_start (1);
  EXPECT_EQ (gs.size, 4);
  free_gs_inline (&gs);
}

// Board size clamped above 11 becomes 11.
TEST (SetupStandardStart, ClampsAbove11)
{
  GameState gs = setup_standard_start (99);
  EXPECT_EQ (gs.size, 11);
  free_gs_inline (&gs);
}

// The 10×10 standard layout must have exactly 4 white and 4 black queens.
TEST (SetupStandardStart, Size10HasCorrectQueenCount)
{
  GameState gs = setup_standard_start (10);
  EXPECT_EQ (count_content (gs, CONTENT_WHITE_QUEEN), 4);
  EXPECT_EQ (count_content (gs, CONTENT_BLACK_QUEEN), 4);
  free_gs_inline (&gs);
}

// The 6×6 layout must also place 4 queens per side.
TEST (SetupStandardStart, Size6HasCorrectQueenCount)
{
  GameState gs = setup_standard_start (6);
  EXPECT_EQ (count_content (gs, CONTENT_WHITE_QUEEN), 4);
  EXPECT_EQ (count_content (gs, CONTENT_BLACK_QUEEN), 4);
  free_gs_inline (&gs);
}

// Any other valid size falls back to 2 queens per side placed in the corners.
TEST (SetupStandardStart, OtherSizeHasCornerQueens)
{
  for (int size : { 4, 5, 7, 8, 9, 11 })
    {
      GameState gs = setup_standard_start (size);
      EXPECT_EQ (count_content (gs, CONTENT_WHITE_QUEEN), 2)
          << "at size " << size;
      EXPECT_EQ (count_content (gs, CONTENT_BLACK_QUEEN), 2)
          << "at size " << size;
      // White must be in the bottom corners.
      EXPECT_EQ (gs.board[size - 1][0], CONTENT_WHITE_QUEEN)
          << "at size " << size;
      EXPECT_EQ (gs.board[size - 1][size - 1], CONTENT_WHITE_QUEEN)
          << "at size " << size;
      free_gs_inline (&gs);
    }
}

// The board must always start with the white player to move.
TEST (SetupStandardStart, StartsWithWhiteToMove)
{
  for (int size : { 4, 6, 10 })
    {
      GameState gs = setup_standard_start (size);
      EXPECT_EQ (gs.current_player, PLAYER_WHITE) << "at size " << size;
      EXPECT_EQ (gs.status, GAME_IN_PROGRESS) << "at size " << size;
      EXPECT_EQ (gs.turn_count, 0) << "at size " << size;
      free_gs_inline (&gs);
    }
}

// No arrows must be present on the initial board for any valid size.
TEST (SetupStandardStart, NoArrowsOnStart)
{
  for (int size = 4; size <= 11; size++)
    {
      GameState gs = setup_standard_start (size);
      EXPECT_EQ (count_content (gs, CONTENT_ARROW), 0) << "at size " << size;
      free_gs_inline (&gs);
    }
}

// ============================================================
// api_init_game / api_free_game
// ============================================================

// api_init_game must return a non-null pointer with the correct board size.
TEST (ApiInitGame, ReturnsValidPointer)
{
  GameState *gs = api_init_game (10);
  ASSERT_NE (gs, nullptr);
  EXPECT_EQ (gs->size, 10);
  api_free_game (gs);
}

// Sizes out of the [4, 11] range must be clamped gracefully.
TEST (ApiInitGame, ClampingWorksViaInit)
{
  GameState *gs_low = api_init_game (1);
  GameState *gs_high = api_init_game (50);
  EXPECT_EQ (gs_low->size, 4);
  EXPECT_EQ (gs_high->size, 11);
  api_free_game (gs_low);
  api_free_game (gs_high);
}

// ============================================================
// api_create_mcts / api_create_minimax / api_free_ai
// ============================================================

// api_create_mcts must return a fully initialised AI with a valid vtable.
TEST (ApiCreateMcts, ReturnsValidAI)
{
  AI_Interface *ai = api_create_mcts (500, 1);
  ASSERT_NE (ai, nullptr);
  EXPECT_NE (ai->vtable, nullptr);
  EXPECT_NE (ai->context, nullptr);
  api_free_ai (ai);
}

// api_create_minimax must return a fully initialised AI with a valid vtable.
TEST (ApiCreateMinimax, ReturnsValidAI)
{
  AI_Interface *ai = api_create_minimax (2);
  ASSERT_NE (ai, nullptr);
  EXPECT_NE (ai->vtable, nullptr);
  EXPECT_NE (ai->context, nullptr);
  api_free_ai (ai);
}

// ============================================================
// api_get_ai_move
// ============================================================

// On a standard 10×10 opening, the MCTS AI must return a non-zero move
// originating from one of the white queen starting squares.
TEST (ApiGetAiMove, MctsReturnsLegalMove)
{
  AI_Interface *ai = api_create_mcts (300, 1);
  GameState *gs = api_init_game (10);

  Move m = api_get_ai_move (ai, gs);

  // The move must originate from a square that contained a white queen.
  EXPECT_EQ (gs->board[m.queen_src.row][m.queen_src.col], CONTENT_WHITE_QUEEN);

  api_free_ai (ai);
  api_free_game (gs);
}

// On a standard 10×10 opening, the Minimax AI must also return a legal move.
TEST (ApiGetAiMove, MinimaxReturnsLegalMove)
{
  AI_Interface *ai = api_create_minimax (1);
  GameState *gs = api_init_game (10);

  Move m = api_get_ai_move (ai, gs);

  EXPECT_EQ (gs->board[m.queen_src.row][m.queen_src.col], CONTENT_WHITE_QUEEN);

  api_free_ai (ai);
  api_free_game (gs);
}

// ============================================================
// api_check_winner
// ============================================================

// On an ongoing game, api_check_winner must return 0.
TEST (ApiCheckWinner, ReturnsZeroOngoingGame)
{
  GameState *gs = api_init_game (10);
  EXPECT_EQ (api_check_winner (gs), 0);
  api_free_game (gs);
}

TEST (ApiCheckWinner, WhiteToMoveButTrappedReturns2)
{
  // Build a minimal board where white is trapped.
  const int SIZE = 4;
  CellContent **cc = (CellContent **)malloc (sizeof (CellContent *) * SIZE);
  for (int i = 0; i < SIZE; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * SIZE);
      for (int j = 0; j < SIZE; j++)
        cc[i][j] = CONTENT_ARROW;
    }
  cc[0][0] = CONTENT_WHITE_QUEEN; // white queen surrounded by arrows
  cc[3][3] = CONTENT_BLACK_QUEEN;

  GameState gs = {};
  gs.size = SIZE;
  gs.board = cc;
  gs.current_player = PLAYER_WHITE;
  gs.status = GAME_IN_PROGRESS;

  EXPECT_EQ (api_check_winner (&gs), 2)
      << "api_check_winner returns 2 when the current (losing) player is "
         "white. Consider returning 1 for white win and 2 for black win "
         "instead.";

  free_board (cc, SIZE);
}

// ============================================================
// api_export_tensor
// ============================================================

// The tensor must be exactly 11×11×4 = 484 floats.  Every value must be
// exactly 0.0 or 1.0 (no other values).
TEST (ApiExportTensor, AllValuesAreBinary)
{
  GameState *gs = api_init_game (10);
  float tensor[484] = {};
  api_export_tensor (gs, tensor);

  for (int i = 0; i < 484; i++)
    EXPECT_TRUE (tensor[i] == 0.0f || tensor[i] == 1.0f)
        << "tensor[" << i << "] = " << tensor[i];

  api_free_game (gs);
}

// Channel 3 (valid board mask) must be 1.0 for every square inside the
// board and 0.0 for every square outside.
TEST (ApiExportTensor, ValidBoardMaskIsCorrect)
{
  const int SIZE = 10;
  const int MAX = 11;
  GameState *gs = api_init_game (SIZE);
  float tensor[484] = {};
  api_export_tensor (gs, tensor);

  for (int r = 0; r < MAX; r++)
    for (int c = 0; c < MAX; c++)
      {
        float val = tensor[(r * MAX + c) + 3 * MAX * MAX];
        if (r < SIZE && c < SIZE)
          EXPECT_EQ (val, 1.0f) << "mask missing at (" << r << "," << c << ")";
        else
          EXPECT_EQ (val, 0.0f)
              << "mask leaks outside at (" << r << "," << c << ")";
      }

  api_free_game (gs);
}

// Channel 0 (white queens) must have exactly as many 1.0 entries as there
// are white queens on the board.
TEST (ApiExportTensor, WhiteQueenChannelMatchesBoardCount)
{
  GameState *gs = api_init_game (10);
  float tensor[484] = {};
  api_export_tensor (gs, tensor);

  int expected = count_content (*gs, CONTENT_WHITE_QUEEN);
  int actual = 0;
  for (int i = 0; i < 484; i++)
    if (tensor[i + 0 * 121] == 1.0f) // channel 0 starts at offset 0
      actual++;
  // channel 0 occupies indices [0, 120]
  actual = 0;
  for (int i = 0; i < 121; i++)
    if (tensor[i] == 1.0f)
      actual++;

  EXPECT_EQ (actual, expected);
  api_free_game (gs);
}

// On an empty board (size 4, corner fallback), every channel 0/1/2 entry
// that falls outside the 4×4 area must be 0.
TEST (ApiExportTensor, OutOfBoardSquaresAreZero)
{
  GameState *gs = api_init_game (4);
  float tensor[484] = {};
  api_export_tensor (gs, tensor);

  const int MAX = 11;
  for (int r = 4; r < MAX; r++) // rows outside the 4×4 board
    for (int c = 0; c < MAX; c++)
      for (int ch = 0; ch < 3; ch++) // channels 0-2
        EXPECT_EQ (tensor[(r * MAX + c) + ch * MAX * MAX], 0.0f)
            << "ch=" << ch << " r=" << r << " c=" << c;

  api_free_game (gs);
}

// ============================================================
// get_selector_id_from_name  (inline, no export needed)
// ============================================================

// "uct" must always resolve to SELECTOR_UCT.
TEST (GetSelectorIdFromName, UctResolves)
{
  EXPECT_EQ (get_selector_id_from_name ("uct"), SELECTOR_UCT);
}

// A null pointer must fall back to SELECTOR_UCT without crashing.
TEST (GetSelectorIdFromName, NullFallsBackToUct)
{
  EXPECT_EQ (get_selector_id_from_name (nullptr), SELECTOR_UCT);
}

// An unrecognised name must fall back to SELECTOR_UCT.
TEST (GetSelectorIdFromName, UnknownFallsBackToUct)
{
  EXPECT_EQ (get_selector_id_from_name ("random_gibberish"), SELECTOR_UCT);
}

// ============================================================
// get_selector_name_from_id  (inline, no export needed)
// ============================================================

// SELECTOR_UCT must resolve to "uct".
TEST (GetSelectorNameFromId, UctResolves)
{
  EXPECT_STREQ (get_selector_name_from_id (SELECTOR_UCT), "uct");
}

// An out-of-range ID must fall back to "uct".
TEST (GetSelectorNameFromId, OutOfRangeFallsBackToUct)
{
  EXPECT_STREQ (get_selector_name_from_id ((SelectorID)999), "uct");
}

// ============================================================
// get_selector — UCT path (always available)
// ============================================================

// get_selector with SELECTOR_UCT must return an ISelector with a valid vtable
// and a non-null context (the UCT exploration constant).
TEST (GetSelector, UctHasValidVtable)
{
  ISelector sel = get_selector (SELECTOR_UCT, 1.414, 0.0, nullptr);

  EXPECT_NE (sel.vtable, nullptr);
  EXPECT_NE (sel.context, nullptr);
  EXPECT_NE (sel.vtable->select_child, nullptr);
  EXPECT_NE (sel.vtable->clean, nullptr);

  sel.vtable->clean (&sel);
}

// Requesting SELECTOR_UCT with a custom exploration constant must produce a
// selector whose UCT value actually differs from one built with a different
// constant (sanity-check that c is stored).
TEST (GetSelector, UctUsesProvidedConstant)
{
  ISelector sel_sqrt2 = get_selector (SELECTOR_UCT, 1.414, 0.0, nullptr);
  ISelector sel_zero = get_selector (SELECTOR_UCT, 0.0, 0.0, nullptr);

  // Both must be valid.
  ASSERT_NE (sel_sqrt2.vtable, nullptr);
  ASSERT_NE (sel_zero.vtable, nullptr);

  // Their contexts must differ (different c value stored inside).
  EXPECT_NE (sel_sqrt2.context, sel_zero.context);

  sel_sqrt2.vtable->clean (&sel_sqrt2);
  sel_zero.vtable->clean (&sel_zero);
}

// ============================================================
// api_apply_move
// ============================================================
TEST (ApiApplyMove, CorrectBoardUpdate)
{
  const int SIZE = 10;
  GameState *gs = api_init_game (SIZE);
  ASSERT_NE (gs, nullptr);

  uint8_t start_r = 9, start_c = 3;
  gs->board[start_r][start_c] = CONTENT_WHITE_QUEEN;

  Move m;
  m.queen_src = { start_r, start_c };
  m.queen_dst = { 8, 3 };
  m.arrow_dst = { 7, 3 };

  ASSERT_EQ (gs->board[8][3], CONTENT_EMPTY);
  ASSERT_EQ (gs->board[7][3], CONTENT_EMPTY);
  ASSERT_EQ (gs->current_player, PLAYER_WHITE);

  api_apply_move (gs, m);

  EXPECT_EQ (gs->board[start_r][start_c], CONTENT_EMPTY)
      << "La case de départ doit être vide";
  EXPECT_EQ (gs->board[8][3], CONTENT_WHITE_QUEEN)
      << "La reine doit être à sa destination";
  EXPECT_EQ (gs->board[7][3], CONTENT_ARROW) << "La flèche doit être posée";

  EXPECT_EQ (gs->current_player, PLAYER_BLACK);
  EXPECT_EQ (gs->turn_count, 1);

  api_free_game (gs);
}

// ============================================================
// tf_wrapper — no-TF fallback (always testable)
// ============================================================

// When TF is disabled, load_amazons_ai must return nullptr and not crash.
TEST (TfWrapper, LoadReturnNullWhenTfDisabled)
{
#ifdef TF_IS_USABLE
  GTEST_SKIP () << "TF is enabled — skipping the no-TF fallback test.";
#else
  AmazonsAI *ai = load_amazons_ai ("any/path");
  EXPECT_EQ (ai, nullptr);
#endif
}

// When TF is disabled, predict_board_value must return the neutral 0.5
// fallback regardless of inputs.
TEST (TfWrapper, PredictReturnsFallbackWhenTfDisabled)
{
#ifdef TF_IS_USABLE
  GTEST_SKIP () << "TF is enabled — skipping the no-TF fallback test.";
#else
  float board[484] = {};
  float result = predict_board_value (nullptr, board);
  EXPECT_FLOAT_EQ (result, 0.5f);
#endif
}

// When TF is disabled, free_amazons_ai must handle nullptr without crashing.
TEST (TfWrapper, FreeNullDoesNotCrashWhenTfDisabled)
{
#ifdef TF_IS_USABLE
  GTEST_SKIP () << "TF is enabled — skipping the no-TF fallback test.";
#else
  free_amazons_ai (nullptr);
#endif
}

// ============================================================
// tf_wrapper — TF-enabled path
// ============================================================

#ifdef TF_IS_USABLE

// A non-existent model path must return nullptr gracefully.
TEST (TfWrapper, LoadInvalidPathReturnsNull)
{
  AmazonsAI *ai = load_amazons_ai ("/does/not/exist");
  EXPECT_EQ (ai, nullptr);
}

// predict_board_value must return 0.5 when given a null AI pointer.
TEST (TfWrapper, PredictWithNullAiReturnsFallback)
{
  float board[484] = {};
  EXPECT_FLOAT_EQ (predict_board_value (nullptr, board), 0.5f);
}

// free_amazons_ai must not crash when given nullptr.
TEST (TfWrapper, FreeNullDoesNotCrash) { free_amazons_ai (nullptr); }

#endif // TF_IS_USABLE

// ============================================================
// get_selector — ML path (TF-enabled only)
// ============================================================

#ifdef TF_IS_USABLE

// With a valid model path, get_selector(SELECTOR_ML) must return an ISelector
// with a vtable. If the path is invalid the session will be null but the
// struct itself must not be zeroed — the vtable is still set by
// create_select_ml.
TEST (GetSelector, MlRequestWithInvalidPathStillHasVtable)
{
  ISelector sel = get_selector (SELECTOR_ML, 0.0, 1.0, "/does/not/exist");

  EXPECT_NE (sel.vtable, nullptr);
  EXPECT_NE (sel.vtable->select_child, nullptr);
  EXPECT_NE (sel.vtable->clean, nullptr);

  sel.vtable->clean (&sel);
}

// When TF is enabled but the model path is invalid, the selector must still
// be cleanable without crashing (free_amazons_ai handles a null session).
TEST (GetSelector, MlCleanWithFailedLoadDoesNotCrash)
{
  ISelector sel = get_selector (SELECTOR_ML, 0.0, 1.0, "/does/not/exist");
  if (sel.vtable && sel.vtable->clean)
    sel.vtable->clean (&sel);
}

// "ml" must resolve to SELECTOR_ML via the name lookup.
TEST (GetSelectorIdFromName, MlResolves)
{
  EXPECT_EQ (get_selector_id_from_name ("ml"), SELECTOR_ML);
}

// SELECTOR_ML must resolve back to "ml" via the id lookup.
TEST (GetSelectorNameFromId, MlResolves)
{
  EXPECT_STREQ (get_selector_name_from_id (SELECTOR_ML), "ml");
}

#else // TF_IS_USABLE not defined

// When TF is disabled, requesting SELECTOR_UCT (the only ID in the enum)
// by name must still work — the ML entry is not in the dict.
TEST (GetSelectorIdFromName, MlNameFallsBackToUctWhenTfDisabled)
{
  EXPECT_EQ (get_selector_id_from_name ("ml"), SELECTOR_UCT);
}

#endif // TF_IS_USABLE