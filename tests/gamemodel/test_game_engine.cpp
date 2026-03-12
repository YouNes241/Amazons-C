#include <gtest/gtest.h>

extern "C"
{
#include "../include/game_engine.h"
}

// Rules and movement tests

TEST (GameEngineTest, LegalMoveValidation)
{
  GameState state = {};
  game_init (&state);

  // Basic move: the queen at (3,0) moves down to (3,1) and shoots to (3,2)
  Move valid_move;
  valid_move.queen_src.col = 3;
  valid_move.queen_src.row = 0;
  valid_move.queen_dst.col = 3;
  valid_move.queen_dst.row = 1;
  valid_move.arrow_dst.col = 3;
  valid_move.arrow_dst.row = 2;

  EXPECT_TRUE (is_move_legal (&state, valid_move));

  game_cleanup (&state);
}

TEST (GameEngineTest, IllegalMoveThroughObstacles)
{
  GameState state = {};
  game_init (&state);

  // Illegal move: black queen at (3,9) tries to cross
  // to (3,1). It would hit a white queen.
  Move illegal_move;
  illegal_move.queen_src.col = 3;
  illegal_move.queen_src.row = 9;
  illegal_move.queen_dst.col = 3;
  illegal_move.queen_dst.row = 0;
  illegal_move.arrow_dst.col = 3;
  illegal_move.arrow_dst.row = 2;

  state.current_player = PLAYER_BLACK;

  EXPECT_FALSE (is_move_legal (&state, illegal_move));

  game_cleanup (&state);
}

TEST (GameEngineTest, MoveWrongPlayerQueen)
{
  GameState state = {};
  game_init (&state);

  // It's White's turn, but we try to move a Black queen at (3,9)
  Move cheat_move;
  cheat_move.queen_src.col = 3;
  cheat_move.queen_src.row = 9;
  cheat_move.queen_dst.col = 3;
  cheat_move.queen_dst.row = 8;
  cheat_move.arrow_dst.col = 3;
  cheat_move.arrow_dst.row = 7;

  EXPECT_FALSE (is_move_legal (&state, cheat_move));

  game_cleanup (&state);
}

TEST (GameEngineTest, MoveEmptySquare)
{
  GameState state = {};
  game_init (&state);

  // Attempting to move from an empty square (5,5)
  Move ghost_move;
  ghost_move.queen_src.col = 5;
  ghost_move.queen_src.row = 5;
  ghost_move.queen_dst.col = 5;
  ghost_move.queen_dst.row = 6;
  ghost_move.arrow_dst.col = 5;
  ghost_move.arrow_dst.row = 7;

  EXPECT_FALSE (is_move_legal (&state, ghost_move));

  game_cleanup (&state);
}

TEST (GameEngineTest, ApplyMoveUpdatesBoard)
{
  GameState state = {};
  game_init (&state);

  Move move;
  move.queen_src.col = 3;
  move.queen_src.row = 0;
  move.queen_dst.col = 3;
  move.queen_dst.row = 1;
  move.arrow_dst.col = 4;
  move.arrow_dst.row = 1;

  apply_move (&state, move);

  // The old square should be empty
  EXPECT_EQ (state.board[0][3], CONTENT_EMPTY);
  // The queen should be on the new square
  EXPECT_EQ (state.board[1][3], CONTENT_WHITE_QUEEN);
  // The arrow should be present
  EXPECT_EQ (state.board[1][4], CONTENT_ARROW);
  // The turn should pass to Black
  EXPECT_EQ (state.current_player, PLAYER_BLACK);

  game_cleanup (&state);
}

TEST (GameEngineTest, ShootArrowOnFreeSquare)
{
  GameState state = {};
  game_init (&state);

  Move valid_move;
  valid_move.queen_src.col = 3;
  valid_move.queen_src.row = 0;
  valid_move.queen_dst.col = 3;
  valid_move.queen_dst.row = 1;

  // The arrow's destination square is the queen's previous square
  valid_move.arrow_dst.col = 3;
  valid_move.arrow_dst.row = 0;

  EXPECT_TRUE (is_move_legal (&state, valid_move));

  game_cleanup (&state);
}

TEST (GameEngineTest, IllegalMoveShape)
{
  GameState state = {};
  game_init (&state);

  // "L" shaped move
  Move knight_move;
  knight_move.queen_src.col = 3;
  knight_move.queen_src.row = 0;
  knight_move.queen_dst.col = 4;
  knight_move.queen_dst.row = 2;
  knight_move.arrow_dst.col = 4;
  knight_move.arrow_dst.row = 3;

  EXPECT_FALSE (is_move_legal (&state, knight_move));

  game_cleanup (&state);
}

TEST (GameEngineTest, LegalDiagonalMove)
{
  GameState state = {};
  game_init (&state);

  Move diag_move;
  diag_move.queen_src.col = 3;
  diag_move.queen_src.row = 0;
  diag_move.queen_dst.col = 5;
  diag_move.queen_dst.row = 2;
  diag_move.arrow_dst.col = 5;
  diag_move.arrow_dst.row = 3;

  EXPECT_TRUE (is_move_legal (&state, diag_move));

  game_cleanup (&state);
}

TEST (GameEngineTest, MoveOutOfBounds)
{
  GameState state = {};
  game_init (&state);
  uint8_t size = state.size; // 10
  // We try to move a queen outside the board limits
  Move out_move;
  out_move.queen_src.col = 3;
  out_move.queen_src.row = 0;
  out_move.queen_dst.col = size;
  out_move.queen_dst.row = 0;
  out_move.arrow_dst.col = 3;
  out_move.arrow_dst.row = 1;

  EXPECT_FALSE (is_move_legal (&state, out_move));

  game_cleanup (&state);
}

// Endgame tests

TEST (GameEngineTest, InitialStateIsNotGameOver)
{
  GameState state = {};
  game_init (&state);

  EXPECT_FALSE (is_game_over (&state, state.current_player));
  EXPECT_EQ (get_winner (&state), NO_PLAYER);

  game_cleanup (&state);
}

TEST (GameEngineTest, GameOverWhenOneSquareLeft)
{
  GameState state = {};
  state.size = 5;
  game_init (&state);

  // Empty the board first
  for (uint8_t y = 0; y < state.size; y++)
    {
      for (uint8_t x = 0; x < state.size; x++)
        {
          state.board[y][x] = CONTENT_EMPTY;
        }
    }

  /* Only one square left for the white queen.
       0 1 2 3 4
     0 W W X . .
     1 W W X . .
     2 . X X . .
     3 X . . B B
     4 . . . B B
  */

  state.board[0][0] = CONTENT_WHITE_QUEEN;
  state.board[0][1] = CONTENT_WHITE_QUEEN;
  state.board[1][0] = CONTENT_WHITE_QUEEN;
  state.board[1][1] = CONTENT_WHITE_QUEEN;

  state.board[3][3] = CONTENT_BLACK_QUEEN;
  state.board[3][4] = CONTENT_BLACK_QUEEN;
  state.board[4][3] = CONTENT_BLACK_QUEEN;
  state.board[4][4] = CONTENT_BLACK_QUEEN;

  state.board[0][2] = CONTENT_ARROW;
  state.board[1][2] = CONTENT_ARROW;
  state.board[2][1] = CONTENT_ARROW;
  state.board[2][2] = CONTENT_ARROW;
  state.board[3][0] = CONTENT_ARROW;

  state.current_player = PLAYER_WHITE;

  EXPECT_FALSE (is_game_over (&state, state.current_player));

  game_cleanup (&state);
}

TEST (GameEngineTest, GameOverWhenPlayerIsBlocked)
{
  GameState state = {};
  state.size = 5;
  game_init (&state);

  for (uint8_t y = 0; y < state.size; y++)
    {
      for (uint8_t x = 0; x < state.size; x++)
        {
          state.board[y][x] = CONTENT_EMPTY;
        }
    }

  /* The white queen is totally blocked.
       0 1 2 3 4
     0 W W X . .
     1 W W X . .
     2 X X X . .
     3 . . . B B
     4 . . . B B
  */

  state.board[0][0] = CONTENT_WHITE_QUEEN;
  state.board[0][1] = CONTENT_WHITE_QUEEN;
  state.board[1][0] = CONTENT_WHITE_QUEEN;
  state.board[1][1] = CONTENT_WHITE_QUEEN;

  state.board[3][3] = CONTENT_BLACK_QUEEN;
  state.board[3][4] = CONTENT_BLACK_QUEEN;
  state.board[4][3] = CONTENT_BLACK_QUEEN;
  state.board[4][4] = CONTENT_BLACK_QUEEN;

  state.board[0][2] = CONTENT_ARROW;
  state.board[1][2] = CONTENT_ARROW;
  state.board[2][0] = CONTENT_ARROW;
  state.board[2][1] = CONTENT_ARROW;
  state.board[2][2] = CONTENT_ARROW;

  state.current_player = PLAYER_WHITE;

  EXPECT_TRUE (is_game_over (&state, state.current_player));

  EXPECT_EQ (get_winner (&state), PLAYER_BLACK);

  game_cleanup (&state);
}

TEST (GameEngineTest, UndoMoveRestoresPreviousState)
{
  GameState state = {};
  game_init (&state);
  Move move;
  move.queen_src = { 0, 3 };
  move.queen_dst = { 1, 3 };
  move.arrow_dst = { 2, 3 };

  apply_move (&state, move);
  EXPECT_EQ (state.turn_count, 1);
  EXPECT_EQ (state.current_player, PLAYER_BLACK);
  EXPECT_TRUE (undo_move (&state));
  EXPECT_EQ (state.turn_count, 0);
  EXPECT_EQ (state.current_player, PLAYER_WHITE);
  EXPECT_EQ (state.board[0][3], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[1][3], CONTENT_EMPTY);
  EXPECT_EQ (state.board[2][3], CONTENT_EMPTY);
  game_cleanup (&state);
}

TEST (GameEngineTest, RedoMoveRestoresMoveAfterUndo)
{
  GameState state = {};
  game_init (&state);
  Move move;
  move.queen_src = { 0, 3 };
  move.queen_dst = { 1, 3 };
  move.arrow_dst = { 2, 3 };

  apply_move (&state, move);
  undo_move (&state);
  EXPECT_TRUE (redo_move (&state));
  EXPECT_EQ (state.turn_count, 1);
  EXPECT_EQ (state.current_player, PLAYER_BLACK);
  EXPECT_EQ (state.board[0][3], CONTENT_EMPTY);
  EXPECT_EQ (state.board[1][3], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.board[2][3], CONTENT_ARROW);
  game_cleanup (&state);
}

TEST (GameEngineTest, UndoAtStartReturnsFalse)
{
  GameState state = {};
  game_init (&state);
  EXPECT_FALSE (undo_move (&state));
  game_cleanup (&state);
}