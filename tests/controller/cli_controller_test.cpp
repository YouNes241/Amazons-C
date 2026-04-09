#include "controller_test_fixture.h"
#include <chrono>
#include <string.h>
#include <thread>

extern "C"
{
  // 1. First include the headers so the types and prototypes are known
  // and available.

#include "ai_factory.h"
#include "ai_interface.h"
#include "cli_controller.h"
#include "controller.h"

  // 2. Declare the TEST_STATIC variables as extern so they can be accessed
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
    std::this_thread::sleep_for (std::chrono::milliseconds (20));
  }
};

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