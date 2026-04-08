#include <gtest/gtest.h>
extern "C"
{
#include "ai_factory.h"
}

// Integration test : checks if the random AI can generate a move and that it
// is within the board limits.
TEST (RandomIntegration, GetRandomMove)
{
  uint8_t board_size = 10;
  // bitboardstate to gamestate
  CellContent **cc
      = (CellContent **)malloc (sizeof (CellContent *) * board_size);
  for (int i = 0; i < board_size; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * board_size);
      for (int j = 0; j < board_size; j++)
        {
          cc[i][j] = CONTENT_EMPTY;
        }
    }

  cc[0][3] = CONTENT_BLACK_QUEEN;
  cc[9][6] = CONTENT_WHITE_QUEEN;

  GameState state
      = { board_size, false, 0, PLAYER_WHITE, GAME_IN_PROGRESS, cc, 0, 0,
          0,          {},    {} };

  BitboardState bs = sync_from_logic (&state);
  bitboard_state_print (&bs);

  AI_Interface randomAI = create_ai ("random", { 0, NULL });
  Move move = randomAI.vtable->get_best_move (&randomAI, state);

  printf ("Random move generated: Queen from (%d,%d) to (%d,%d) with arrow to "
          "(%d,%d)\n",
          move.queen_src.row, move.queen_src.col, move.queen_dst.row,
          move.queen_dst.col, move.arrow_dst.row, move.arrow_dst.col);
  EXPECT_TRUE (move.queen_src.row >= 0 && move.queen_src.row < board_size);
  EXPECT_TRUE (move.queen_src.col >= 0 && move.queen_src.col < board_size);
  EXPECT_TRUE (move.queen_dst.row >= 0 && move.queen_dst.col < board_size);
  EXPECT_TRUE (move.queen_dst.row >= 0 && move.queen_dst.col < board_size);
  EXPECT_TRUE (move.arrow_dst.row >= 0 && move.arrow_dst.row < board_size);
  EXPECT_TRUE (move.arrow_dst.col >= 0 && move.arrow_dst.col < board_size);
}