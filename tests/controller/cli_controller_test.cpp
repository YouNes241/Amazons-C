#include "controller_test_fixture.h"
#include <chrono>
#include <thread>

extern "C"
{
// 1. On inclut d'abord les headers pour que les types et prototypes soient
// connus
#include "cli_controller.h"
#include "controller.h"

  // 2. On déclare les variables TEST_STATIC en extern pour y accéder
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
 * 1. TESTS DES FONCTIONS INTERNES
 * ==========================================================================
 */
TEST_F (CliControllerTest, IsItAiTurnLogic)
{
  game_config_t cfg;
  memset (&cfg, 0, sizeof (game_config_t));

  cfg.ai_white = true;
  cfg.ai_black = false;
  EXPECT_TRUE (is_it_ai_turn (&cfg, PLAYER_WHITE));
  EXPECT_FALSE (is_it_ai_turn (&cfg, PLAYER_BLACK));

  cfg.ai_white = false;
  cfg.ai_black = true;
  EXPECT_FALSE (is_it_ai_turn (&cfg, PLAYER_WHITE));
  EXPECT_TRUE (is_it_ai_turn (&cfg, PLAYER_BLACK));
}

TEST_F (CliControllerTest, LineHandlerLogic)
{
  std::thread t (
      [] ()
        {
          game_config_t cfg;
          memset (&cfg, 0, sizeof (game_config_t));
          cfg.board_size = 10;
          run_interactive_mode (&cfg);
        });

  while (!g_cli_ctx)
    std::this_thread::sleep_for (std::chrono::milliseconds (5));

  EXPECT_NO_THROW (linehandler (nullptr));

  char *empty_line = strdup ("");
  EXPECT_NO_THROW (linehandler (empty_line));

  char *valid_line = strdup ("help");
  EXPECT_NO_THROW (linehandler (valid_line));

  g_cli_ctx->game_running = false;
  t.join ();
}

/* ==========================================================================
 * 2. TESTS DE LA BOUCLE PRINCIPALE (Interactive Mode)
 * ==========================================================================
 */
TEST_F (CliControllerTest, InitializationAndServerStart)
{
  std::thread t (
      [] ()
        {
          game_config_t cfg;
          memset (&cfg, 0, sizeof (game_config_t));
          cfg.board_size = 10;
          cfg.start_server = true;
          cfg.server_port = 9425;
          run_interactive_mode (&cfg);
        });

  while (!g_cli_ctx)
    std::this_thread::sleep_for (std::chrono::milliseconds (5));

  EXPECT_FALSE (g_cli_ctx->in_main_menu);
  EXPECT_TRUE (g_cli_ctx->game_running);

  g_cli_ctx->game_running = false;
  t.join ();
}

TEST_F (CliControllerTest, GameOverLogic_LocalMode)
{
  std::thread t (
      [] ()
        {
          game_config_t cfg;
          memset (&cfg, 0, sizeof (game_config_t));
          cfg.board_size = 10;
          run_interactive_mode (&cfg);
        });

  while (!g_cli_ctx)
    std::this_thread::sleep_for (std::chrono::milliseconds (5));

  for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 10; j++)
        {
          g_cli_ctx->current_game->board[i][j] = (CellContent)2;
        }
    }

  t.join ();
  EXPECT_EQ (g_cli_ctx, nullptr);
}

TEST_F (CliControllerTest, GameOverLogic_NetworkMode)
{
  std::thread t (
      [] ()
        {
          game_config_t cfg;
          memset (&cfg, 0, sizeof (game_config_t));
          cfg.board_size = 10;
          run_interactive_mode (&cfg);
        });

  while (!g_cli_ctx)
    std::this_thread::sleep_for (std::chrono::milliseconds (5));

  g_cli_ctx->net_manager->current_mode = NET_MODE_CLIENT;
  g_cli_ctx->in_network_match = true;
  g_cli_ctx->in_main_menu = false;
  g_cli_ctx->my_role = PLAYER_WHITE;

  for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 10; j++)
        {
          g_cli_ctx->current_game->board[i][j] = (CellContent)2;
        }
    }

  bool game_ended = false;
  for (int i = 0; i < 50; i++)
    {
      if (!g_cli_ctx->in_network_match)
        {
          game_ended = true;
          break;
        }
      std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

  EXPECT_TRUE (game_ended);
  EXPECT_TRUE (g_cli_ctx->game_running);

  g_cli_ctx->game_running = false;
  t.join ();
}

TEST_F (CliControllerTest, SynchronizedTimeoutLogic)
{
  std::thread t (
      [] ()
        {
          game_config_t cfg;
          memset (&cfg, 0, sizeof (game_config_t));
          cfg.board_size = 10;
          run_interactive_mode (&cfg);
        });

  while (!g_cli_ctx)
    std::this_thread::sleep_for (std::chrono::milliseconds (5));

  g_cli_ctx->net_manager->current_mode = NET_MODE_CLIENT;
  g_cli_ctx->in_network_match = true;
  g_cli_ctx->in_main_menu = false;
  g_cli_ctx->current_game->current_player = PLAYER_WHITE;
  g_cli_ctx->my_role = PLAYER_WHITE;
  g_cli_ctx->timeout_sent = false;

  g_cli_ctx->turn_start_time = time (NULL) - 100;

  bool timeout_triggered = false;
  for (int i = 0; i < 50; i++)
    {
      if (g_cli_ctx->current_game->current_player == PLAYER_BLACK)
        {
          timeout_triggered = true;
          break;
        }
      std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }

  EXPECT_TRUE (timeout_triggered);

  g_cli_ctx->game_running = false;
  t.join ();
}

TEST_F (CliControllerTest, AITurnLogic)
{
  std::thread t (
      [] ()
        {
          game_config_t cfg;
          memset (&cfg, 0, sizeof (game_config_t));
          cfg.board_size = 10;
          cfg.ai_white = true;
          run_interactive_mode (&cfg);
        });

  while (!g_cli_ctx || !g_cli_ctx->ai_initialized)
    {
      std::this_thread::sleep_for (std::chrono::milliseconds (5));
    }

  bool ai_played = false;
  for (int i = 0; i < 500; i++)
    {
      if (g_cli_ctx->current_game->current_player == PLAYER_BLACK)
        {
          ai_played = true;
          break;
        }
      std::this_thread::sleep_for (std::chrono::milliseconds (20));
    }

  EXPECT_TRUE (ai_played);

  g_cli_ctx->game_running = false;
  t.join ();
}