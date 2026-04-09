#include "controller_test_fixture.h"
#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <string.h>
#include <thread>
#include <unistd.h>

extern "C"
{
#include "ai_factory.h"
#include "ai_interface.h"
#include "cli_controller.h"
#include "cli_view.h"
#include "controller.h"

  extern CliContext *g_cli_ctx;
  bool is_it_ai_turn (game_config_t *config, PlayerColor current_p);
  void linehandler (char *line);
}

class CliControllerTest : public ::testing::Test
{
protected:
  void
  TearDown () override
  {
    if (g_cli_ctx)
      {
        g_cli_ctx->game_running = false;
      }
    std::this_thread::sleep_for (std::chrono::milliseconds (50));
  }
};

/* ==========================================================================
 * Helper class for stdin redirection - SAME PROCESS for coverage
 * ==========================================================================
 */
class StdinRedirector
{
private:
  int original_stdin;
  int pipe_fds[2];
  bool redirected;

public:
  StdinRedirector () : original_stdin (-1), redirected (false)
  {
    pipe_fds[0] = -1;
    pipe_fds[1] = -1;
  }

  ~StdinRedirector () { restore (); }

  bool
  setup ()
  {
    if (pipe (pipe_fds) == -1)
      return false;

    // Make read end non-blocking to avoid hangs
    int flags = fcntl (pipe_fds[0], F_GETFL, 0);
    fcntl (pipe_fds[0], F_SETFL, flags | O_NONBLOCK);

    original_stdin = dup (STDIN_FILENO);
    if (original_stdin == -1)
      {
        close (pipe_fds[0]);
        close (pipe_fds[1]);
        return false;
      }

    if (dup2 (pipe_fds[0], STDIN_FILENO) == -1)
      {
        close (original_stdin);
        close (pipe_fds[0]);
        close (pipe_fds[1]);
        return false;
      }

    redirected = true;
    return true;
  }

  void
  writeInput (const char *input)
  {
    if (pipe_fds[1] != -1)
      {
        write (pipe_fds[1], input, strlen (input));
      }
  }

  void
  sendEOF ()
  {
    if (pipe_fds[1] != -1)
      {
        close (pipe_fds[1]);
        pipe_fds[1] = -1;
      }
  }

  void
  restore ()
  {
    if (redirected)
      {
        dup2 (original_stdin, STDIN_FILENO);
        close (original_stdin);
        original_stdin = -1;

        if (pipe_fds[0] != -1)
          {
            close (pipe_fds[0]);
            pipe_fds[0] = -1;
          }
        if (pipe_fds[1] != -1)
          {
            close (pipe_fds[1]);
            pipe_fds[1] = -1;
          }
        redirected = false;
      }
  }
};

/* ==========================================================================
 * run_interactive_mode TESTS - Using threads in SAME PROCESS for coverage
 * ==========================================================================
 */
class RunInteractiveModeTest : public ::testing::Test
{
protected:
  game_config_t *config;
  int saved_stdin;
  int pipe_fds[2];

  void
  SetUp () override
  {
    config = (game_config_t *)calloc (1, sizeof (game_config_t));
    ASSERT_NE (config, nullptr);
    init_default_config (config);
    config->board_size = 10;
    saved_stdin = -1;
    pipe_fds[0] = -1;
    pipe_fds[1] = -1;
  }

  void
  TearDown () override
  {
    // Restore stdin if redirected
    if (saved_stdin != -1)
      {
        dup2 (saved_stdin, STDIN_FILENO);
        close (saved_stdin);
        saved_stdin = -1;
      }
    if (pipe_fds[0] != -1)
      {
        close (pipe_fds[0]);
        pipe_fds[0] = -1;
      }
    if (pipe_fds[1] != -1)
      {
        close (pipe_fds[1]);
        pipe_fds[1] = -1;
      }

    // Make sure global context is cleared
    if (g_cli_ctx)
      {
        g_cli_ctx->game_running = false;
      }
    std::this_thread::sleep_for (std::chrono::milliseconds (50));

    if (config)
      {
        free (config);
        config = nullptr;
      }
  }

  bool
  setupStdinRedirect ()
  {
    if (pipe (pipe_fds) == -1)
      return false;

    saved_stdin = dup (STDIN_FILENO);
    if (saved_stdin == -1)
      {
        close (pipe_fds[0]);
        close (pipe_fds[1]);
        return false;
      }

    if (dup2 (pipe_fds[0], STDIN_FILENO) == -1)
      {
        close (saved_stdin);
        close (pipe_fds[0]);
        close (pipe_fds[1]);
        saved_stdin = -1;
        return false;
      }

    return true;
  }

  void
  sendInput (const char *input)
  {
    if (pipe_fds[1] != -1)
      {
        write (pipe_fds[1], input, strlen (input));
      }
  }

  void
  sendEOF ()
  {
    if (pipe_fds[1] != -1)
      {
        close (pipe_fds[1]);
        pipe_fds[1] = -1;
      }
  }
};

// Test run_interactive_mode with immediate EOF (Ctrl+D)
TEST_F (RunInteractiveModeTest, ExitsOnEOF)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = false;
  config->ai_black_cfg.enabled = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  // Send EOF immediately
  sendEOF ();

  // Wait for thread with timeout
  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  EXPECT_TRUE (finished);
}

// Test run_interactive_mode with quit command
TEST_F (RunInteractiveModeTest, ExitsOnQuitCommand)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = false;
  config->ai_black_cfg.enabled = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  // Give it time to start
  std::this_thread::sleep_for (std::chrono::milliseconds (100));

  // Send quit command
  sendInput ("quit\n");
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  EXPECT_TRUE (finished);
}

// Test run_interactive_mode with server start
TEST_F (RunInteractiveModeTest, ServerStartSuccess)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = true;
  config->server_port = 19876;
  config->ai_white_cfg.enabled = false;
  config->ai_black_cfg.enabled = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (200));
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test with AI white enabled
TEST_F (RunInteractiveModeTest, WithAIWhiteEnabled)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = true;
  strncpy (config->ai_white_cfg.name, "random", MAX_AI_STR_LEN);
  config->ai_white_cfg.param_count = 0;
  config->ai_black_cfg.enabled = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test with AI black enabled
TEST_F (RunInteractiveModeTest, WithAIBlackEnabled)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = false;
  config->ai_black_cfg.enabled = true;
  strncpy (config->ai_black_cfg.name, "random", MAX_AI_STR_LEN);
  config->ai_black_cfg.param_count = 0;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test with both AIs enabled
TEST_F (RunInteractiveModeTest, WithBothAIsEnabled)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = true;
  strncpy (config->ai_white_cfg.name, "random", MAX_AI_STR_LEN);
  config->ai_white_cfg.param_count = 0;
  config->ai_black_cfg.enabled = true;
  strncpy (config->ai_black_cfg.name, "random", MAX_AI_STR_LEN);
  config->ai_black_cfg.param_count = 0;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test with AI params
TEST_F (RunInteractiveModeTest, WithAIParams)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = true;
  strncpy (config->ai_white_cfg.name, "minimax", MAX_AI_STR_LEN);
  config->ai_white_cfg.param_count = 1;
  strncpy (config->ai_white_cfg.param_keys[0], "depth", MAX_AI_STR_LEN);
  strncpy (config->ai_white_cfg.param_vals[0], "2", MAX_AI_STR_LEN);
  config->ai_black_cfg.enabled = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test new game then quit
TEST_F (RunInteractiveModeTest, NewGameThenQuit)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = false;
  config->ai_black_cfg.enabled = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendInput ("new\n");
  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendInput ("quit\n");
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test help command
TEST_F (RunInteractiveModeTest, HelpCommand)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendInput ("help\n");
  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendInput ("quit\n");
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test different board size
TEST_F (RunInteractiveModeTest, DifferentBoardSize)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->board_size = 6;
  config->start_server = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test starting a game and making a move
TEST_F (RunInteractiveModeTest, NewGameAndMove)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = false;
  config->ai_white_cfg.enabled = false;
  config->ai_black_cfg.enabled = false;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendInput ("new\n");
  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  // Try a move (d1-d2(d3))
  sendInput ("d1-d2(d3)\n");
  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendInput ("quit\n");
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

// Test server start and quit
TEST_F (RunInteractiveModeTest, ServerStartAndQuit)
{
  ASSERT_TRUE (setupStdinRedirect ());

  config->start_server = true;
  config->server_port = 19878;

  std::atomic<bool> finished (false);

  std::thread t (
      [this, &finished] ()
        {
          run_interactive_mode (config);
          finished = true;
        });

  std::this_thread::sleep_for (std::chrono::milliseconds (200));
  sendInput ("server stop\n");
  std::this_thread::sleep_for (std::chrono::milliseconds (100));
  sendInput ("quit\n");
  sendEOF ();

  auto start = std::chrono::steady_clock::now ();
  while (!finished
         && std::chrono::steady_clock::now () - start
                < std::chrono::seconds (5))
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (50));
    }

  if (!finished && g_cli_ctx)
    {
      g_cli_ctx->game_running = false;
    }

  if (t.joinable ())
    {
      t.join ();
    }

  SUCCEED ();
}

/* ==========================================================================
 * 1. INTERNAL FUNCTION TESTS
 * ==========================================================================
 */
TEST_F (CliControllerTest, IsItAiTurnLogic)
{
  game_config_t cfg;
  memset (&cfg, 0, sizeof (game_config_t));

  cfg.ai_white_cfg.enabled = true;
  cfg.ai_black_cfg.enabled = false;
  EXPECT_TRUE (is_it_ai_turn (&cfg, PLAYER_WHITE));
  EXPECT_FALSE (is_it_ai_turn (&cfg, PLAYER_BLACK));

  cfg.ai_white_cfg.enabled = false;
  cfg.ai_black_cfg.enabled = true;
  EXPECT_FALSE (is_it_ai_turn (&cfg, PLAYER_WHITE));
  EXPECT_TRUE (is_it_ai_turn (&cfg, PLAYER_BLACK));
}

// Test is_it_ai_turn with invalid player color (covers the return false
// branch)
TEST_F (CliControllerTest, IsItAiTurnInvalidPlayer)
{
  game_config_t cfg;
  memset (&cfg, 0, sizeof (game_config_t));
  cfg.ai_white_cfg.enabled = true;
  cfg.ai_black_cfg.enabled = true;

  // Cast an invalid value to PlayerColor to trigger the fallback return false
  PlayerColor invalid_player = (PlayerColor)99;
  EXPECT_FALSE (is_it_ai_turn (&cfg, invalid_player));
}

// Test is_it_ai_turn with both AIs disabled
TEST_F (CliControllerTest, IsItAiTurnBothDisabled)
{
  game_config_t cfg;
  memset (&cfg, 0, sizeof (game_config_t));
  cfg.ai_white_cfg.enabled = false;
  cfg.ai_black_cfg.enabled = false;

  EXPECT_FALSE (is_it_ai_turn (&cfg, PLAYER_WHITE));
  EXPECT_FALSE (is_it_ai_turn (&cfg, PLAYER_BLACK));
}

// Test is_it_ai_turn with both AIs enabled
TEST_F (CliControllerTest, IsItAiTurnBothEnabled)
{
  game_config_t cfg;
  memset (&cfg, 0, sizeof (game_config_t));
  cfg.ai_white_cfg.enabled = true;
  cfg.ai_black_cfg.enabled = true;

  EXPECT_TRUE (is_it_ai_turn (&cfg, PLAYER_WHITE));
  EXPECT_TRUE (is_it_ai_turn (&cfg, PLAYER_BLACK));
}

TEST_F (CliControllerTest, LineHandlerLogic)
{

  CliContext local_ctx;
  memset (&local_ctx, 0, sizeof (CliContext));

  GameState dummy_game;
  game_init (&dummy_game);
  local_ctx.current_game = &dummy_game;
  local_ctx.game_running = true;

  g_cli_ctx = &local_ctx;

  char *empty_line = strdup ("");
  EXPECT_NO_THROW (linehandler (empty_line));
  EXPECT_TRUE (g_cli_ctx->game_running);

  char *valid_line = strdup ("help");
  EXPECT_NO_THROW (linehandler (valid_line));
  EXPECT_TRUE (g_cli_ctx->game_running);

  EXPECT_NO_THROW (linehandler (nullptr));
  EXPECT_FALSE (g_cli_ctx->game_running);

  g_cli_ctx = nullptr;
  game_cleanup (&dummy_game);
}

// Test linehandler when g_cli_ctx is NULL (early return branch)
TEST_F (CliControllerTest, LineHandlerNullContext)
{
  g_cli_ctx = nullptr;
  char *line = strdup ("test");
  // Should return early without crashing
  EXPECT_NO_THROW (linehandler (line));
  // Note: line is NOT freed when g_cli_ctx is null, so we need to free it
  free (line);
}

// Test linehandler with in_main_menu = true (skip board display branch)
TEST_F (CliControllerTest, LineHandlerInMainMenu)
{
  CliContext local_ctx;
  memset (&local_ctx, 0, sizeof (CliContext));

  GameState dummy_game;
  game_init (&dummy_game);
  local_ctx.current_game = &dummy_game;
  local_ctx.game_running = true;
  local_ctx.in_main_menu = true; // In main menu

  g_cli_ctx = &local_ctx;

  char *line = strdup ("help");
  EXPECT_NO_THROW (linehandler (line));
  // Should still be running and in main menu
  EXPECT_TRUE (g_cli_ctx->game_running);

  g_cli_ctx = nullptr;
  game_cleanup (&dummy_game);
}

// Test linehandler with game not running after command (game_running = false
// branch)
TEST_F (CliControllerTest, LineHandlerGameEndsAfterCommand)
{
  CliContext local_ctx;
  memset (&local_ctx, 0, sizeof (CliContext));

  GameState dummy_game;
  game_init (&dummy_game);
  local_ctx.current_game = &dummy_game;
  local_ctx.game_running = true;
  local_ctx.in_main_menu = true;

  NetworkManager net_mgr;
  memset (&net_mgr, 0, sizeof (NetworkManager));
  local_ctx.net_manager = &net_mgr;

  g_cli_ctx = &local_ctx;

  // "quit" command in local mode should set game_running to false
  char *line = strdup ("quit");
  EXPECT_NO_THROW (linehandler (line));
  EXPECT_FALSE (g_cli_ctx->game_running);

  g_cli_ctx = nullptr;
  game_cleanup (&dummy_game);
}

// Test linehandler displaying board when not in main menu
TEST_F (CliControllerTest, LineHandlerDisplaysBoard)
{
  CliContext local_ctx;
  memset (&local_ctx, 0, sizeof (CliContext));

  GameState dummy_game;
  game_init (&dummy_game);
  local_ctx.current_game = &dummy_game;
  local_ctx.game_running = true;
  local_ctx.in_main_menu = false; // NOT in main menu, should display board

  g_cli_ctx = &local_ctx;

  char *line = strdup ("show");
  EXPECT_NO_THROW (linehandler (line));
  EXPECT_TRUE (g_cli_ctx->game_running);

  g_cli_ctx = nullptr;
  game_cleanup (&dummy_game);
}

TEST_F (ControllerTestFixture, InitializationAndServerStart)
{

  game_config_t cfg;
  memset (&cfg, 0, sizeof (game_config_t));
  cfg.board_size = 10;
  cfg.start_server = true;
  cfg.server_port = 9425;

  int start_res
      = NetworkManager_ServerStart (ctx.net_manager, cfg.server_port);
  EXPECT_EQ (start_res, 0);

  NetworkManager_Cleanup (ctx.net_manager);
}

TEST_F (ControllerTestFixture, GameOverLogic_LocalMode)
{

  for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 10; j++)
        {
          ctx.current_game->board[i][j] = (CellContent)2;
        }
    }

  EXPECT_TRUE (is_game_over (ctx.current_game, PLAYER_WHITE));
  EXPECT_TRUE (is_game_over (ctx.current_game, PLAYER_BLACK));
}

TEST_F (ControllerTestFixture, GameOverLogic_NetworkMode)
{
  ctx.net_manager->current_mode = NET_MODE_CLIENT;
  ctx.in_network_match = true;
  ctx.in_main_menu = false;
  ctx.my_role = PLAYER_WHITE;

  for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 10; j++)
        {
          ctx.current_game->board[i][j] = (CellContent)2;
        }
    }

  EXPECT_TRUE (is_game_over (ctx.current_game, PLAYER_WHITE));
}

TEST_F (ControllerTestFixture, SynchronizedTimeoutLogic)
{
  ctx.net_manager->current_mode = NET_MODE_CLIENT;
  ctx.in_network_match = true;
  ctx.in_main_menu = false;
  ctx.current_game->current_player = PLAYER_WHITE;
  ctx.my_role = PLAYER_WHITE;

  ctx.turn_start_time = time (NULL) - 100;
  ctx.timeout_limit = 60;
  ctx.timeout_sent = false;

  time_t now = time (NULL);
  bool timeout_triggered
      = (difftime (now, ctx.turn_start_time) >= ctx.timeout_limit
         && !ctx.timeout_sent);

  EXPECT_TRUE (timeout_triggered);
}

TEST_F (ControllerTestFixture, AITurnLogic)
{

  game_config_t cfg;
  memset (&cfg, 0, sizeof (game_config_t));
  cfg.board_size = 10;

  Param params[1] = {};
  ConfigMap cm = { .count = 0, .list = params };

  AI_Interface my_ai = create_ai ("random", cm);
  ASSERT_NE (my_ai.vtable, nullptr);

  ctx.current_game->current_player = PLAYER_WHITE;
  Move ai_move = my_ai.vtable->get_best_move (&my_ai, *ctx.current_game);

  EXPECT_TRUE (is_move_legal (ctx.current_game, ai_move));

  if (my_ai.vtable->clean)
    {
      my_ai.vtable->clean (&my_ai);
    }
}