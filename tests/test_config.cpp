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

/* ==========================================================================
 * init_default_config TESTS
 * ==========================================================================
 */

TEST_F (ConfigTest, InitDefaultConfig)
{
  EXPECT_EQ (config.board_size, 10);
  EXPECT_EQ (config.mode, MODE_INTERACTIVE);
  EXPECT_FALSE (config.blitz_mode);
  EXPECT_FALSE (config.ai_white_cfg.enabled);
  EXPECT_FALSE (config.ai_black_cfg.enabled);
}

TEST_F (ConfigTest, InitDefaultConfig_AINames)
{
  EXPECT_STREQ (config.ai_white_cfg.name, "minimax");
  EXPECT_STREQ (config.ai_black_cfg.name, "minimax");
}

TEST_F (ConfigTest, InitDefaultConfig_ParamCounts)
{
  EXPECT_EQ (config.ai_white_cfg.param_count, 0);
  EXPECT_EQ (config.ai_black_cfg.param_count, 0);
}

TEST_F (ConfigTest, InitDefaultConfig_TimePerPlayer)
{
  EXPECT_EQ (config.time_per_player, DEFAULT_TIME_BLITZ);
}

TEST_F (ConfigTest, InitDefaultConfig_Verbose)
{
  EXPECT_FALSE (config.verbose);
}

TEST_F (ConfigTest, InitDefaultConfig_Language)
{
  EXPECT_EQ (config.language, LANG_FR);
}

/* ==========================================================================
 * get_config_path TESTS
 * ==========================================================================
 */

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

TEST_F (ConfigTest, GetConfigPathSmallBuffer)
{
  char path[32];
  setenv ("HOME", "/very/long/path/that/exceeds", 1);

  // Should truncate properly
  get_config_path (path, sizeof (path));

  EXPECT_GT (strlen (path), 0u);
}

/* ==========================================================================
 * load_config_file - AUTO CREATE TESTS
 * ==========================================================================
 */

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

TEST_F (ConfigTest, AutoCreateConfigFile_ContestMode)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.mode = MODE_CONTEST;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("mode = contest") != std::string::npos);
}

TEST_F (ConfigTest, AutoCreateConfigFile_GUIMode)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.mode = MODE_GUI;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("mode = gui") != std::string::npos);
}

TEST_F (ConfigTest, AutoCreateConfigFile_InteractiveMode)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.mode = MODE_INTERACTIVE;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("mode = interactive") != std::string::npos);
}

TEST_F (ConfigTest, AutoCreateConfigFile_WithAIEnabled)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.ai_white_cfg.enabled = true;
  strncpy (config.ai_white_cfg.name, "mcts", MAX_AI_STR_LEN);
  config.ai_black_cfg.enabled = true;
  strncpy (config.ai_black_cfg.name, "random", MAX_AI_STR_LEN);

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("ai_white = mcts") != std::string::npos);
  EXPECT_TRUE (content.find ("ai_black = random") != std::string::npos);
}

TEST_F (ConfigTest, AutoCreateConfigFile_WithAIDisabled)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.ai_white_cfg.enabled = false;
  config.ai_black_cfg.enabled = false;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("ai_white = false") != std::string::npos);
  EXPECT_TRUE (content.find ("ai_black = false") != std::string::npos);
}

TEST_F (ConfigTest, AutoCreateConfigFile_VerboseEnabled)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.verbose = true;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("verbose = true") != std::string::npos);
}

TEST_F (ConfigTest, AutoCreateConfigFile_LanguageEnglish)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.language = LANG_EN;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("language = english") != std::string::npos);
}

TEST_F (ConfigTest, AutoCreateConfigFile_LanguageFrench)
{
  unsetenv ("HOME");
  remove (".amazonsrc");

  config.language = LANG_FR;

  load_config_file (&config);

  std::ifstream f (".amazonsrc");
  std::string content ((std::istreambuf_iterator<char> (f)),
                       std::istreambuf_iterator<char> ());
  f.close ();

  EXPECT_TRUE (content.find ("language = french") != std::string::npos);
}

/* ==========================================================================
 * load_config_file - PARSING TESTS
 * ==========================================================================
 */

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

TEST_F (ConfigTest, ParseModeInteractive)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "mode = interactive\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.mode, MODE_INTERACTIVE);
}

TEST_F (ConfigTest, ParseModeContest)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "mode = contest\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.mode, MODE_CONTEST);
}

TEST_F (ConfigTest, ParseModeGUI)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "mode = gui\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.mode, MODE_GUI);
}

TEST_F (ConfigTest, ParseBlitzFalse)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "blitz = false\n";
  f.close ();

  config.blitz_mode = true; // Set to true first

  load_config_file (&config);

  EXPECT_FALSE (config.blitz_mode);
}

TEST_F (ConfigTest, ParseTimeout)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "timeout = 3600\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.time_per_player, 3600);
}

TEST_F (ConfigTest, ParseAIWhite_True)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "ai_white = true\n";
  f.close ();

  load_config_file (&config);

  EXPECT_TRUE (config.ai_white_cfg.enabled);
  EXPECT_STREQ (config.ai_white_cfg.name, "minimax"); // Default name
}

TEST_F (ConfigTest, ParseAIWhite_False)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "ai_white = false\n";
  f.close ();

  config.ai_white_cfg.enabled = true; // Set to true first

  load_config_file (&config);

  EXPECT_FALSE (config.ai_white_cfg.enabled);
}

TEST_F (ConfigTest, ParseAIWhite_CustomName)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "ai_white = mcts\n";
  f.close ();

  load_config_file (&config);

  EXPECT_TRUE (config.ai_white_cfg.enabled);
  EXPECT_STREQ (config.ai_white_cfg.name, "mcts");
}

TEST_F (ConfigTest, ParseAIBlack_True)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "ai_black = true\n";
  f.close ();

  load_config_file (&config);

  EXPECT_TRUE (config.ai_black_cfg.enabled);
  EXPECT_STREQ (config.ai_black_cfg.name, "minimax"); // Default name
}

TEST_F (ConfigTest, ParseAIBlack_False)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "ai_black = false\n";
  f.close ();

  config.ai_black_cfg.enabled = true; // Set to true first

  load_config_file (&config);

  EXPECT_FALSE (config.ai_black_cfg.enabled);
}

TEST_F (ConfigTest, ParseAIBlack_CustomName)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "ai_black = random\n";
  f.close ();

  load_config_file (&config);

  EXPECT_TRUE (config.ai_black_cfg.enabled);
  EXPECT_STREQ (config.ai_black_cfg.name, "random");
}

TEST_F (ConfigTest, ParseVerbose_True)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "verbose = true\n";
  f.close ();

  load_config_file (&config);

  EXPECT_TRUE (config.verbose);
}

TEST_F (ConfigTest, ParseVerbose_False)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "verbose = false\n";
  f.close ();

  config.verbose = true; // Set to true first

  load_config_file (&config);

  EXPECT_FALSE (config.verbose);
}

TEST_F (ConfigTest, ParseLanguage_French)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "language = french\n";
  f.close ();

  config.language = LANG_EN; // Set to English first

  load_config_file (&config);

  EXPECT_EQ (config.language, LANG_FR);
}

TEST_F (ConfigTest, ParseLanguage_English)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "language = english\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.language, LANG_EN);
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

TEST_F (ConfigTest, IgnoreCommentLines)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "# This is a comment\n";
  f << "board_size = 8\n";
  f << "# Another comment\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 8);
}

TEST_F (ConfigTest, IgnoreEmptyLines)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "\n";
  f << "board_size = 7\n";
  f << "\n";
  f << "\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 7);
}

TEST_F (ConfigTest, IgnoreSectionHeaders)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "[defaults]\n";
  f << "board_size = 6\n";
  f << "[other_section]\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 6);
}

TEST_F (ConfigTest, ParseAllOptions)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "[defaults]\n";
  f << "board_size = 8\n";
  f << "mode = contest\n";
  f << "blitz = true\n";
  f << "timeout = 1800\n";
  f << "ai_white = mcts\n";
  f << "ai_black = minimax\n";
  f << "verbose = true\n";
  f << "language = english\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 8);
  EXPECT_EQ (config.mode, MODE_CONTEST);
  EXPECT_TRUE (config.blitz_mode);
  EXPECT_EQ (config.time_per_player, 1800);
  EXPECT_TRUE (config.ai_white_cfg.enabled);
  EXPECT_STREQ (config.ai_white_cfg.name, "mcts");
  EXPECT_TRUE (config.ai_black_cfg.enabled);
  EXPECT_STREQ (config.ai_black_cfg.name, "minimax");
  EXPECT_TRUE (config.verbose);
  EXPECT_EQ (config.language, LANG_EN);
}

TEST_F (ConfigTest, BoardSizeBoundary_Minimum)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "board_size = 4\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 4);
}

TEST_F (ConfigTest, BoardSizeBoundary_Maximum)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "board_size = 11\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 11);
}

TEST_F (ConfigTest, BoardSizeBoundary_BelowMinimum)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "board_size = 3\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 10); // Should remain default
}

TEST_F (ConfigTest, BoardSizeBoundary_AboveMaximum)
{
  unsetenv ("HOME");
  std::ofstream f (".amazonsrc");
  f << "board_size = 12\n";
  f.close ();

  load_config_file (&config);

  EXPECT_EQ (config.board_size, 10); // Should remain default
}

/* ==========================================================================
 * display_config TESTS
 * ==========================================================================
 */

TEST_F (ConfigTest, DisplayConfig_InteractiveMode)
{
  config.mode = MODE_INTERACTIVE;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Interactive") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_ContestMode)
{
  config.mode = MODE_CONTEST;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Contest") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_GUIMode)
{
  config.mode = MODE_GUI;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("GUI") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_UnknownMode)
{
  config.mode = (game_mode_t)99; // Invalid mode

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Unknown") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_AIWhiteEnabled)
{
  config.ai_white_cfg.enabled = true;
  strncpy (config.ai_white_cfg.name, "mcts", MAX_AI_STR_LEN);
  config.ai_white_cfg.param_count = 2;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Yes") != std::string::npos);
  EXPECT_TRUE (output.find ("mcts") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_AIWhiteDisabled)
{
  config.ai_white_cfg.enabled = false;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("White player AI   : No") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_AIBlackEnabled)
{
  config.ai_black_cfg.enabled = true;
  strncpy (config.ai_black_cfg.name, "random", MAX_AI_STR_LEN);
  config.ai_black_cfg.param_count = 0;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("random") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_AIBlackDisabled)
{
  config.ai_black_cfg.enabled = false;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Black player AI   : No") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_BlitzEnabled)
{
  config.blitz_mode = true;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Enabled") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_BlitzDisabled)
{
  config.blitz_mode = false;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Disabled") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_VerboseEnabled)
{
  config.verbose = true;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("Verbose mode      : Enabled")
               != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_LanguageFrench)
{
  config.language = LANG_FR;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("French") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_LanguageEnglish)
{
  config.language = LANG_EN;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("English") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_BoardSize)
{
  config.board_size = 8;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("8 x 8") != std::string::npos);
}

TEST_F (ConfigTest, DisplayConfig_TimePerPlayer)
{
  config.time_per_player = 3600;

  testing::internal::CaptureStdout ();
  display_config (&config);
  std::string output = testing::internal::GetCapturedStdout ();

  EXPECT_TRUE (output.find ("3600 s") != std::string::npos);
}