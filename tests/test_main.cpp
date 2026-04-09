#include <gtest/gtest.h>

/**
 * @file test_main.cpp
 * @brief Unit tests for main.c functions (parse_options, add_ai_param, etc.)
 *
 * Tests cover command-line argument parsing, AI parameter configuration,
 * and various startup modes.
 */

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>

extern "C"
{
#include "../src/cli_parser.h"
#include "config.h"
#include "controller.h"
#include "network_common.h"
}

class MainTest : public ::testing::Test
{
protected:
  game_config_t *config;

  void
  SetUp () override
  {
    config = (game_config_t *)malloc (sizeof (game_config_t));
    memset (config, 0, sizeof (game_config_t));
    config->start_server = false;
    config->daemon_mode = false;
    config->server_port = 12345;

    // Ensure no config file interferes
    unsetenv ("HOME");
    remove (".amazonsrc");
  }

  void
  TearDown () override
  {
    if (config)
      {
        free (config);
        config = nullptr;
      }
    remove (".amazonsrc");
    // Reset optind for getopt
    optind = 1;
  }
};

/* ==========================================================================
 * BOARD SIZE TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsBoardSizeValid)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-n", (char *)"8", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->board_size, 8);
}

TEST_F (MainTest, ParseOptionsBoardSizeTooSmall)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-n", (char *)"2", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_FAILURE);
}

TEST_F (MainTest, ParseOptionsBoardSizeTooLarge)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-n", (char *)"99", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_FAILURE);
}

TEST_F (MainTest, ParseOptionsBoardSizeLongOption)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--size", (char *)"6", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->board_size, 6);
}

TEST_F (MainTest, ParseOptionsBoardSizeMinimum)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-n", (char *)"4", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->board_size, 4);
}

TEST_F (MainTest, ParseOptionsBoardSizeMaximum)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-n", (char *)"11", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->board_size, 11);
}

/* ==========================================================================
 * BLITZ MODE TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsBlitzMode)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-b", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->blitz_mode);
}

TEST_F (MainTest, ParseOptionsBlitzModeLong)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--blitz", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->blitz_mode);
}

/* ==========================================================================
 * TIME OPTION TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsTimeWithBlitz)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-b", (char *)"-t", (char *)"15",
                   nullptr };
  int argc = 4;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->blitz_mode);
  EXPECT_EQ (config->time_per_player, 15 * 60);
}

TEST_F (MainTest, ParseOptionsTimeWithoutBlitz)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-t", (char *)"20", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_FALSE (config->blitz_mode);
  EXPECT_EQ (config->time_per_player, DEFAULT_TIME_BLITZ);
}

TEST_F (MainTest, ParseOptionsTimeInvalid_Zero)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-b", (char *)"-t", (char *)"0",
                   nullptr };
  int argc = 4;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_FAILURE);
}

TEST_F (MainTest, ParseOptionsTimeInvalid_TooLarge)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-b", (char *)"-t",
                   (char *)"100", nullptr };
  int argc = 4;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_FAILURE);
}

TEST_F (MainTest, ParseOptionsTimeInvalid_Negative)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-b", (char *)"-t", (char *)"-5",
                   nullptr };
  int argc = 4;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_FAILURE);
}

TEST_F (MainTest, ParseOptionsTimeLong)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-b", (char *)"--time",
                   (char *)"30", nullptr };
  int argc = 4;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->time_per_player, 30 * 60);
}

/* ==========================================================================
 * MODE TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsContestMode)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-c", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->mode, MODE_CONTEST);
}

TEST_F (MainTest, ParseOptionsContestModeLong)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--contest", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->mode, MODE_CONTEST);
}

TEST_F (MainTest, ParseOptionsGuiMode)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-g", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->mode, MODE_GUI);
}

TEST_F (MainTest, ParseOptionsGuiModeLong)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--gui", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->mode, MODE_GUI);
}

/* ==========================================================================
 * AI OPTION TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsAIWhite)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-white", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_white_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsAIBlack)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-black", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsAIModeRandom)
{
  optind = 1;
  char *argv[]
      = { (char *)"amazons", (char *)"--ai-mode", (char *)"random", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_STREQ (config->ai_black_cfg.name, "random");
}

TEST_F (MainTest, ParseOptionsAIModeMinimax)
{
  optind = 1;
  char *argv[]
      = { (char *)"amazons", (char *)"--ai-mode", (char *)"minimax", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_STREQ (config->ai_black_cfg.name, "minimax");
}

TEST_F (MainTest, ParseOptionsAIModeIterative)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-mode", (char *)"iterative",
                   nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_GE (config->ai_black_cfg.param_count, 1);
}

TEST_F (MainTest, ParseOptionsAIModeMcts)
{
  optind = 1;
  char *argv[]
      = { (char *)"amazons", (char *)"--ai-mode", (char *)"mcts", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_STREQ (config->ai_black_cfg.name, "mcts");
}

TEST_F (MainTest, ParseOptionsAIMinimaxDepth)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-minimax-depth",
                   (char *)"5", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_GE (config->ai_black_cfg.param_count, 1);
}

TEST_F (MainTest, ParseOptionsAIMinimaxScoring)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-minimax-scoring",
                   (char *)"rth", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_GE (config->ai_black_cfg.param_count, 1);
}

TEST_F (MainTest, ParseOptionsAIMinimaxFwPruningEnabled)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-minimax-fwpruning",
                   (char *)"1", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsAIMinimaxFwPruningDisabled)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-minimax-fwpruning",
                   (char *)"0", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsAIMinimaxFwPruningFalse)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-minimax-fwpruning",
                   (char *)"false", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsAIMinimaxFwPruningDisabledKeyword)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-minimax-fwpruning",
                   (char *)"disabled", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsAIMctsSelection)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--ai-mcts-selection",
                   (char *)"uct", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsAITime)
{
  optind = 1;
  char *argv[]
      = { (char *)"amazons", (char *)"--ai-time", (char *)"5", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_GE (config->ai_black_cfg.param_count, 1);
}

TEST_F (MainTest, ParseOptionsAIWhiteThenBlack)
{
  optind = 1;
  char *argv[] = { (char *)"amazons",    (char *)"--ai-white",
                   (char *)"--ai-mode",  (char *)"mcts",
                   (char *)"--ai-black", (char *)"--ai-mode",
                   (char *)"minimax",    nullptr };
  int argc = 7;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_white_cfg.enabled);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
  EXPECT_STREQ (config->ai_white_cfg.name, "mcts");
  EXPECT_STREQ (config->ai_black_cfg.name, "minimax");
}

/* ==========================================================================
 * SERVER OPTIONS TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsServerShortNoPort)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-s", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->start_server);
  EXPECT_EQ (config->server_port, 12345);
}

TEST_F (MainTest, ParseOptionsServerWithPort)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-s", (char *)"9999", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->start_server);
  EXPECT_EQ (config->server_port, 9999);
}

TEST_F (MainTest, ParseOptionsServerLong)
{
  optind = 1;
  char *argv[]
      = { (char *)"amazons", (char *)"--server", (char *)"8888", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->start_server);
  EXPECT_EQ (config->server_port, 8888);
}

TEST_F (MainTest, ParseOptionsDaemon)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--daemon", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->daemon_mode);
}

/* ==========================================================================
 * DEBUG/VERBOSE TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsVerbose)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-v", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->verbose);
}

TEST_F (MainTest, ParseOptionsVerboseLong)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--verbose", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->verbose);
}

TEST_F (MainTest, ParseOptionsDebugShortWithServer)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-s", (char *)"-d", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->start_server);
  EXPECT_TRUE (config->daemon_mode);
}

TEST_F (MainTest, ParseOptionsDebugShortWithoutServer)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-d", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->verbose);
}

TEST_F (MainTest, ParseOptionsDebugLong)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"--debug", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->verbose);
}

/* ==========================================================================
 * UNKNOWN ARGUMENT TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsUnknownArgument)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"unknown_arg", nullptr };
  int argc = 2;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_FAILURE);
}

/* ==========================================================================
 * COMBINED OPTIONS TESTS
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsCombined)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-n",         (char *)"8",
                   (char *)"-b",      (char *)"-t",         (char *)"10",
                   (char *)"-v",      (char *)"--ai-black", nullptr };
  int argc = 8;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->board_size, 8);
  EXPECT_TRUE (config->blitz_mode);
  EXPECT_EQ (config->time_per_player, 10 * 60);
  EXPECT_TRUE (config->verbose);
  EXPECT_TRUE (config->ai_black_cfg.enabled);
}

TEST_F (MainTest, ParseOptionsNoArgs)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", nullptr };
  int argc = 1;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_EQ (config->board_size, DEFAULT_BOARD_SIZE);
  EXPECT_EQ (config->mode, MODE_INTERACTIVE);
}

/* ==========================================================================
 * EDGE CASES
 * ==========================================================================
 */

TEST_F (MainTest, ParseOptionsMultipleAIModes)
{
  optind = 1;
  char *argv[] = { (char *)"amazons",   (char *)"--ai-mode", (char *)"random",
                   (char *)"--ai-mode", (char *)"mcts",      nullptr };
  int argc = 5;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_STREQ (config->ai_black_cfg.name, "mcts");
}

TEST_F (MainTest, ParseOptionsServerPortAsNextArg)
{
  optind = 1;
  char *argv[] = { (char *)"amazons", (char *)"-s", (char *)"7777", nullptr };
  int argc = 3;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->start_server);
  EXPECT_EQ (config->server_port, 7777);
}

TEST_F (MainTest, ParseOptionsAllAIParamsForWhite)
{
  optind = 1;
  char *argv[] = { (char *)"amazons",
                   (char *)"--ai-white",
                   (char *)"--ai-mode",
                   (char *)"minimax",
                   (char *)"--ai-minimax-depth",
                   (char *)"4",
                   (char *)"--ai-minimax-scoring",
                   (char *)"me",
                   (char *)"--ai-minimax-fwpruning",
                   (char *)"1",
                   (char *)"--ai-time",
                   (char *)"10",
                   nullptr };
  int argc = 12;

  int result = parse_options (argc, argv, config);

  EXPECT_EQ (result, EXIT_SUCCESS);
  EXPECT_TRUE (config->ai_white_cfg.enabled);
  EXPECT_STREQ (config->ai_white_cfg.name, "minimax");
  EXPECT_GE (config->ai_white_cfg.param_count, 3);
}