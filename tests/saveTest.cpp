#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>

extern "C"
{
#include "../include/config.h"
#include "../include/game_types.h"
#include "../include/save.h"
}

const char *VALID_BOARD_10X10 = "[game]\n"
                                "W\n"
                                "_ _ _ q _ _ q _ _ _\n"
                                "_ _ _ _ _ _ _ _ _ _\n"
                                "_ _ _ _ _ _ _ _ _ _\n"
                                "q _ _ _ _ _ _ _ _ q\n"
                                "_ _ _ _ _ _ _ _ _ _\n"
                                "_ _ _ _ _ _ _ _ _ _\n"
                                "Q _ _ _ _ _ _ _ _ Q\n"
                                "_ _ _ _ _ _ _ _ _ _\n"
                                "_ _ _ _ _ _ _ _ _ _\n"
                                "_ _ _ Q _ _ Q _ _ _\n";

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

    state.board[0][3] = CONTENT_WHITE_QUEEN;
    state.board[0][6] = CONTENT_WHITE_QUEEN;
    state.board[3][0] = CONTENT_WHITE_QUEEN;
    state.board[3][9] = CONTENT_WHITE_QUEEN;

    state.board[9][3] = CONTENT_BLACK_QUEEN;
    state.board[9][6] = CONTENT_BLACK_QUEEN;
    state.board[6][0] = CONTENT_BLACK_QUEEN;
    state.board[6][9] = CONTENT_BLACK_QUEEN;

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
  state.board[0][3] = CONTENT_EMPTY;
  state.board[1][3] = CONTENT_WHITE_QUEEN;
  state.current_player = PLAYER_BLACK;

  ASSERT_EQ (save_game (path, &config, &state), SAVE_OK);

  state.board[1][3] = CONTENT_EMPTY;

  ASSERT_EQ (load_game (path, &config, &state), SAVE_OK);
  EXPECT_EQ (state.board[1][3], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.current_player, PLAYER_BLACK);
}

TEST_F (SaveLoadTest, CorruptedFile)
{
  const char *path = "test_corrupt.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n[game]\n";
  f.close ();

  EXPECT_EQ (load_game (path, &config, &state), ERR_PREMATURE_EOF);
}

TEST_F (SaveLoadTest, BoardOverflowInFile)
{
  const char *path = "test_overflow.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=4\n[game]\nW\n";
  f << "Q Q Q Q Q Q Q Q Q Q\n";
  f.close ();

  EXPECT_EQ (load_game (path, &config, &state), ERR_CORRUPT_GAME_SECTION);
}

TEST_F (SaveLoadTest, HistorySaturated)
{
  const char *path = "test_hist.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n";
  f << VALID_BOARD_10X10;
  f << "[history]\n";
  for (int i = 0; i < 150; i++)
    {
      f << "a1-a2/a3\n";
    }
  f.close ();

  EXPECT_EQ (load_game (path, &config, &state), ERR_CORRUPT_HISTORY_SECTION);
}

TEST_F (SaveLoadTest, ComplexComments)
{
  const char *path = "test_comments.txt";
  std::ofstream f (path);
  f << "# Commentaire simple\n"
    << "[settings] { Bloc complexe }\n"
    << "board_size=10 # Taille\n";
  f << VALID_BOARD_10X10;
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), SAVE_OK);
  EXPECT_EQ (config.board_size, 10);
  EXPECT_EQ (state.board[0][3], CONTENT_WHITE_QUEEN);
}

TEST_F (SaveLoadTest, MissingFile)
{
  EXPECT_EQ (load_game ("file_inexistant.txt", &config, &state),
             ERR_FILE_NOT_FOUND);
}

TEST_F (SaveLoadTest, MemoryShieldSegfault)
{
  const char *path = "test_bounds.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=11\n";
  f.close ();

  EXPECT_EQ (load_game (path, &config, &state), ERR_BOARD_SIZE_MISMATCH);
}

TEST_F (SaveLoadTest, InvalidHistoryMoves)
{
  const char *path = "test_inv_hist.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n";
  f << VALID_BOARD_10X10;
  f << "[history]\n";
  f << "a0-b2/c3\n";
  f.close ();

  EXPECT_EQ (load_game (path, &config, &state), ERR_CORRUPT_HISTORY_SECTION);
}

TEST_F (SaveLoadTest, ShortAndInvalidBoardLines)
{
  const char *path = "test_short_board.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=5\n[game]\nW\n";
  f << "Q $ x\n";
  f.close ();

  int result = load_game (path, &config, &state);
  EXPECT_TRUE (result == ERR_BOARD_SIZE_MISMATCH
               || result == ERR_CORRUPT_GAME_SECTION);
}

TEST_F (SaveLoadTest, MultilineBlockComments)
{
  const char *path = "test_multiline.txt";
  std::ofstream f (path);
  f << "{ Ce commentaire démarre ici\n"
    << "et il saute des lignes\n"
    << "avant de fermer la parenthèse }\n"
    << "[settings]\n"
    << "board_size=10\n";
  f << VALID_BOARD_10X10;
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), SAVE_OK);
  EXPECT_EQ (config.board_size, 10);
}

TEST_F (SaveLoadTest, AINameTruncation)
{
  const char *path = "test_ai_name.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n";
  f << "ai_black=this_name_is_way_too_long_and_should_be_truncated_to_31_"
       "characters_in_the_structure\n";
  f << VALID_BOARD_10X10;
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), SAVE_OK);
  EXPECT_TRUE (config.ai_black_cfg.enabled);
  EXPECT_EQ (strlen (config.ai_black_cfg.name), 31);
}

TEST_F (SaveLoadTest, InvalidQueenCountAudit)
{
  const char *path = "test_invalid_queens.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n[game]\nW\n";

  f << "_ _ _ q _ _ q _ _ _\n"
       "_ _ _ _ _ _ _ _ _ _\n"
       "_ _ _ _ _ _ _ _ _ _\n"
       "q _ _ _ _ _ _ _ _ q\n"
       "_ _ _ _ _ _ _ _ _ _\n"
       "_ _ _ _ _ _ _ _ _ _\n"
       "Q _ _ _ _ _ _ _ _ Q\n"
       "_ _ _ _ _ _ _ _ _ _\n"
       "_ _ _ _ _ _ _ _ _ _\n"
       "_ _ _ Q _ _ q _ _ _\n";
  f.close ();

  EXPECT_EQ (load_game (path, &config, &state), ERR_INVALID_QUEEN_COUNT);
}