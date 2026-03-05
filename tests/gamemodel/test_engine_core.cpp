#include <gtest/gtest.h>

// Tell the C++ compiler that these headers are plain C
extern "C"
{
#include "../include/game_engine.h"
}

// Coords Tests
TEST (EngineCore, CoordsToIndex)
{
  uint8_t size = 10;
  EXPECT_EQ (coords_to_index (3, 4, size), 43); // x=3, y=4 -> 4*10+3
  EXPECT_EQ (coords_to_index (0, 0, size), 0);
  EXPECT_EQ (coords_to_index (9, 9, size), 99);
}

TEST (EngineCore, IndexToCoords)
{
  uint8_t size = 10;
  uint8_t x, y;
  index_to_coords (43, size, &x, &y);
  EXPECT_EQ (x, 3);
  EXPECT_EQ (y, 4);

  index_to_coords (0, size, &x, &y);
  EXPECT_EQ (x, 0);
  EXPECT_EQ (y, 0);

  index_to_coords (99, size, &x, &y);
  EXPECT_EQ (x, 9);
  EXPECT_EQ (y, 9);

  index_to_coords (50, size, NULL, &y);
  index_to_coords (50, size, &x, NULL);
  SUCCEED ();
}

TEST (EngineCore, IsValidCoord)
{
  uint8_t size = 10;

  // Inside the board
  EXPECT_TRUE (is_valid_coord (0, 0, size));
  EXPECT_TRUE (is_valid_coord (5, 5, size));
  EXPECT_TRUE (is_valid_coord (9, 9, size));

  // Outside (upper bounds)
  EXPECT_FALSE (is_valid_coord (10, 5, size));
  EXPECT_FALSE (is_valid_coord (5, 10, size));
  EXPECT_FALSE (is_valid_coord (10, 10, size));

  // Outside (lower bounds / negative numbers)
  EXPECT_FALSE (is_valid_coord (-1, 5, size));
  EXPECT_FALSE (is_valid_coord (5, -1, size));
}

// Initialization and Cleanup Tests

TEST (EngineCore, GameInitDefaultSize)
{
  GameState state = {}; // Initialize everything to zero
  game_init (&state);

  // Verify the default size
  EXPECT_EQ (state.size, 10);

  // Verify that memory was allocated
  EXPECT_NE (state.board, nullptr);

  // Check game variables
  EXPECT_EQ (state.current_player, PLAYER_WHITE);
  EXPECT_EQ (state.turn_count, 0);
  EXPECT_EQ (state.status, GAME_IN_PROGRESS);

  game_cleanup (&state);
}

TEST (EngineCore, GameInitCustomSize)
{
  GameState state = {};
  state.size = 8; // Force a different size before init
  game_init (&state);

  // The engine should respect this size and not force 10
  EXPECT_EQ (state.size, 8);
  EXPECT_NE (state.board, nullptr);

  game_cleanup (&state);
}

TEST (EngineCore, GameInitNullHandling)
{
  // If a NULL pointer is passed, the game should not crash
  game_init (NULL);
  SUCCEED ();
}

TEST (EngineCore, InitialQueenPlacementSize10)
{
  GameState state = {};
  game_init (&state);
  uint8_t size = state.size;

  /* Expected board :
     Légende : (.) Vide, (W) Reine Blanche, (B) Reine Noire

       0 1 2 3 4 5 6 7 8 9 (Columns)
     0 . . . W . . W . . .
     1 . . . . . . . . . .
     2 . . . . . . . . . .
     3 W . . . . . . . . W
     4 . . . . . . . . . .
     5 . . . . . . . . . .
     6 B . . . . . . . . B
     7 . . . . . . . . . .
     8 . . . . . . . . . .
     9 . . . B . . B . . .
  */

  // Verify presence of the 4 white queens
  EXPECT_EQ (state.board[0][3], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[0][size - 4], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[3][0], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[3][size - 1], CONTENT_WHITE_QUEEN);

  // Verify presence of the 4 black queens
  EXPECT_EQ (state.board[size - 1][3], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[size - 1][size - 4], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[size - 4][0], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[size - 4][size - 1], CONTENT_BLACK_QUEEN);

  // Verify that the rest is empty (pick a random cell in the middle)
  EXPECT_EQ (state.board[5][5], CONTENT_EMPTY);

  game_cleanup (&state);
}

TEST (EngineCore, InitialQueenPlacementSize5)
{
  GameState state = {};
  state.size = 5;
  game_init (&state);

  /* Expected board :

       0 1 2 3 4 (Columns)
     0 . W . W .
     1 W . . . W
     2 . . . . .
     3 B . . . B
     4 . B . B .
  */

  EXPECT_EQ (state.board[0][1], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[0][3], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[1][0], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[1][4], CONTENT_WHITE_QUEEN);

  EXPECT_EQ (state.board[4][1], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[4][3], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[3][0], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[3][4], CONTENT_BLACK_QUEEN);

  EXPECT_EQ (state.board[2][2], CONTENT_EMPTY);

  game_cleanup (&state);
}

TEST (EngineCore, InitialQueenPlacementSize8)
{
  GameState state = {};
  state.size = 8;
  game_init (&state);

  /* Expected board :

       0 1 2 3 4 5 6 7 (Columns)
     0 . . W . . W . .
     1 . . . . . . . .
     2 W . . . . . . W
     3 . . . . . . . .
     4 . . . . . . . .
     5 B . . . . . . B
     6 . . . . . . . .
     7 . . B . . B . .
  */

  EXPECT_EQ (state.board[0][2], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[0][5], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[2][0], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[2][7], CONTENT_WHITE_QUEEN);

  EXPECT_EQ (state.board[7][2], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[7][5], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[5][0], CONTENT_BLACK_QUEEN);
  EXPECT_EQ (state.board[5][7], CONTENT_BLACK_QUEEN);

  EXPECT_EQ (state.board[4][4], CONTENT_EMPTY);

  game_cleanup (&state);
}

TEST (EngineCore, GameCleanupRobustness)
{
  // Cleaning a NULL pointer should not crash
  game_cleanup (NULL);

  // Cleaning a state whose board hasn't been allocated should not crash
  GameState empty_state = {};
  game_cleanup (&empty_state);

  // Double cleanup should not crash
  GameState state = {};
  game_init (&state);
  game_cleanup (&state);
  game_cleanup (&state); // Should be ignored

  SUCCEED ();
}