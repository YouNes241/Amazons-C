#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>

// We include the C files in an extern "C" block
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
    // Initialisation de la config
    config.board_size = 10;

    // Initialisation du state
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

// test 1 : Cycle complet de sauvegarde et chargement
TEST_F (SaveLoadTest, FullCycle)
{
  const char *path = "test_ok.txt";
  state.board[9][0] = CONTENT_WHITE_QUEEN; // a10
  state.current_player = PLAYER_BLACK;

  ASSERT_EQ (save_game (path, &config, &state), 0);

  // Reinitialize to make sure load is actually working
  state.board[9][0] = CONTENT_EMPTY;

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (state.board[9][0], CONTENT_WHITE_QUEEN);
  EXPECT_EQ (state.current_player, PLAYER_BLACK);
}

// test 2: corrupted file (missing data)
TEST_F (SaveLoadTest, CorruptedFile)
{
  const char *path = "test_corrupt.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n[game]\n"; // pas de plateau, pas de tour
  f.close ();

  // We expect the program not to crash
  EXPECT_EQ (load_game (path, &config, &state), 0);
}

// test 3: board larger than expected
// Test whether the code overruns allocated memory or stops
// correctly at the size defined in config.board_size
TEST_F (SaveLoadTest, BoardOverflowInFile)
{
  const char *path = "test_overflow.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=4\n[game]\nW\n";
  f << "Q Q Q Q Q Q Q Q Q Q\n"; // 10 reines pour un board_size=4
  f.close ();

  // The code must stop at col < state->size (4)
  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_EQ (config.board_size, 4);
}

// test 4: history saturated
TEST_F (SaveLoadTest, HistorySaturated)
{
  const char *path = "test_hist.txt";
  std::ofstream f (path);
  f << "[settings]\nboard_size=10\n[history]\n";
  for (int i = 0; i < 150; i++)
    { // 150 moves for a capacity of 100
      f << "a1-a2(a3)\n";
    }
  f.close ();

  ASSERT_EQ (load_game (path, &config, &state), 0);
  EXPECT_LE (state.undo_stack.top, 99);
}

// test 5: complex commentary
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