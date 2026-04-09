#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>

extern "C"
{
#include "../include/config.h"
}

class ConfigTest : public ::testing::Test
{
protected:
  game_config_t config;

  void
  SetUp () override
  {
    init_default_config (&config);
  }

  void
  TearDown () override
  {
    remove ("test_dummy_config.txt");
    remove (".amazonsrc");
  }
};

TEST_F (ConfigTest, InitDefaultConfig)
{
  EXPECT_EQ (config.board_size, 10);
  EXPECT_EQ (config.mode, MODE_INTERACTIVE);
  EXPECT_FALSE (config.blitz_mode);
  EXPECT_FALSE (config.ai_white_cfg.enabled);
  EXPECT_FALSE (config.ai_black_cfg.enabled);
}

TEST_F (ConfigTest, GetConfigPathWithHome)
{
  char path[512];
  setenv ("HOME", "/tmp/temphome", 1);

  get_config_path (path, sizeof (path));

  EXPECT_STREQ (path, "/tmp/temphome/.amazonsrc");
}

TEST_F (ConfigTest, GetConfigPathWithoutHome)
{
  char path[512];

  unsetenv ("HOME");

  get_config_path (path, sizeof (path));

  EXPECT_STREQ (path, ".amazonsrc");
}

TEST_F (ConfigTest, AutoCreateConfigFileIfMissing)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.board_size = 11;
  config.blitz_mode = true;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  EXPECT_TRUE (f.good ());
  f.close ();
}

TEST_F (ConfigTest, ParseValidConfiguration)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "board_size = 11\n";
  f << "mode = gui\n";
  f << "blitz = true\n";
  f << "ai_black = minimax\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 11);
  EXPECT_EQ (config.mode, MODE_GUI);
  EXPECT_TRUE (config.blitz_mode);
  EXPECT_TRUE (config.ai_black_cfg.enabled);
  EXPECT_STREQ (config.ai_black_cfg.name, "minimax");
}

TEST_F (ConfigTest, IgnoreOutOfBoundsBoardSize)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "board_size = 999\n";
  f << "board_size = -5\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 10);
}

TEST_F (ConfigTest, IgnoreGarbageLines)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "some_garbage_text\n";
  f << "wrong_key=true\n";
  f << "mode = wrong_mode\n";
  f.close ();

  int old_board_size = config.board_size;
  int old_mode = config.mode;

  load_config_file (&config);

  EXPECT_EQ (config.board_size, old_board_size);
  EXPECT_EQ (config.mode, old_mode);
}