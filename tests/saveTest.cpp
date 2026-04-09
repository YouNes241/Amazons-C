#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>

extern "C"
{
#include "../include/config.h"
#include "../include/game_types.h"
#include "../include/save.h"
}

class SaveLoadTest : public ::testing::Test
{
protected:
  game_config_t config;
  GameState state;

  void
  SetUp () override
  {
    config.board_size = 10;

    state.size = 10;
    state.current_player = PLAYER_WHITE;
    state.turn_count = 0;

    // Allocation dynamique du plateau
    state.board = (CellContent **)malloc (state.size * sizeof (CellContent *));
    for (int i = 0; i < state.size; i++)
      {
        state.board[i]
            = (CellContent *)malloc (state.size * sizeof (CellContent));
        for (int j = 0; j < state.size; j++)
          {
            state.board[i][j] = CONTENT_EMPTY;
          }
      }

    // Allocation des piles
    state.undo_stack.capacity = 100;
    state.undo_stack.top = -1;
    state.undo_stack.data
        = (Move *)malloc (state.undo_stack.capacity * sizeof (Move));

    state.redo_stack.capacity = 100;
    state.redo_stack.top = -1;
    state.redo_stack.data
        = (Move *)malloc (state.redo_stack.capacity * sizeof (Move));
  }

  void
  TearDown () override
  {
    for (int i = 0; i < state.size; i++)
      {
        free (state.board[i]);
      }
    free (state.board);
    free (state.undo_stack.data);
    free (state.redo_stack.data);
  }
};

TEST_F (SaveLoadTest, FullCycle)
{
  const char *path = "test_ok.txt";
  state.board[9][0] = CONTENT_WHITE_QUEEN; // a10
  state.current_player = PLAYER_BLACK;

  ASSERT_EQ (save_game (path, &config, &state), 0);

  state.board[9][0] = CONTENT_EMPTY;

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (state.board[9][0], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.current_player, PLAYER_BLACK);
}

TEST_F (SaveLoadTest, CorruptedFile)
{
  const char *path = "test_corrupt.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n[game]\n";
  f.close ();

  EXPECT_EQ (load_game (path, &config, &state), 0);
}

TEST_F (SaveLoadTest, BoardOverflowInFile)
{
  const char *path = "test_overflow.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=4\n[game]\nW\n";
  f << "Q Q Q Q Q Q Q Q Q Q\n"; // 10 columns instead of 4
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (config.board_size, 4);
}

TEST_F (SaveLoadTest, HistorySaturated)
{
  const char *path = "test_hist.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n[history]\n";
  for (int i = 0; i < 150; i++)
    {
      f << "a1-a2(a3)\n";
    }
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_LE (state.undo_stack.top, 99);
}

TEST_F (SaveLoadTest, ComplexComments)
{
  const char *path = "test_comments.txt";
  std::ofstream f (path);
  f << "# Commentaire simple\n"
    << "[settings] { Bloc complexe }\n"
    << "board_size=10 # Taille\n"
    << "[game]\n"
    << "W\n"
    << "Q _ _ _ _ _ _ _ _ _\n";
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (config.board_size, 10);
  EXPECT_EQ (state.board[9][0], CONTENT_WHITE_QUEEN);
}

TEST_F (SaveLoadTest, MissingFile)
{
  EXPECT_EQ (load_game ("file_inexistant.txt", &config, &state), -1);
}

TEST_F (SaveLoadTest, BoardSizeOutOfBounds)
{
  const char *path = "test_bounds.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=999\n";
  f << "ai_black=false\n";
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (config.board_size, 10);
}

TEST_F (SaveLoadTest, InvalidHistoryMoves)
{
  const char *path = "test_inv_hist.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n[history]\n";
  f << "a0-b2/c3\n";
  f << "z9-z8/z7\n";
  f << "invalid format line\n";
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (state.undo_stack.top, -1);
}

TEST_F (SaveLoadTest, ShortAndInvalidBoardLines)
{
  const char *path = "test_short_board.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=5\n[game]\nW\n";
  // Only 3 cases at instead of 5, with an invalid '$' character
  f << "Q $ x\n";
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);

  EXPECT_EQ (state.board[4][0], CONTENT_WHITE_QUEEN);
  // $ must be CONTENT_EMPTY
  EXPECT_EQ (state.board[4][1], CONTENT_EMPTY);
  EXPECT_EQ (state.board[4][2], CONTENT_ARROW);
  // The missing cases must be filled with CONTENT_EMPTY
  EXPECT_EQ (state.board[4][3], CONTENT_EMPTY);
  EXPECT_EQ (state.board[4][4], CONTENT_EMPTY);
}

TEST_F (SaveLoadTest, MultilineBlockComments)
{
  const char *path = "test_multiline.txt";
  std::ofstream f (path);
  f << "{ Ce commentaire démarre ici\n"
    << "et il saute des lignes\n"
    << "avant de fermer la parenthèse }\n"
    << "[settings]\n"
    << "board_size=8\n";
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (config.board_size, 8);
}

TEST_F (SaveLoadTest, AINameTruncation)
{
  const char *path = "test_ai_name.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n";
  f << "ai_black=this_name_is_way_too_long_and_should_be_truncated_to_31_"
       "characters_in_the"
       "structure\n";
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_TRUE (config.ai_black_cfg.enabled);
  EXPECT_EQ (strlen (config.ai_black_cfg.name), 31);
}