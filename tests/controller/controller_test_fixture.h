#ifndef CONTROLLER_TEST_FIXTURE_H
#define CONTROLLER_TEST_FIXTURE_H

#include <gtest/gtest.h>
#include <string.h>

extern "C"
{
#include "../../src/controller/cli_commands.h"
#include "../../src/controller/cli_context.h"
#include "../../src/controller/cli_network_events.h"
#include "config.h"
#include "game_engine.h"
#include "network_common.h"
}

class ControllerTestFixture : public ::testing::Test
{
protected:
  CliContext ctx;
  GameState game;
  game_config_t config;
  NetworkManager net_mgr;
  NetworkEvents events;

  void
  SetUp () override
  {
    memset (&ctx, 0, sizeof (CliContext));
    memset (&game, 0, sizeof (GameState));
    memset (&config, 0, sizeof (game_config_t));
    memset (&net_mgr, 0, sizeof (NetworkManager));

    config.board_size = 10;
    game_init (&game);

    ctx.current_game = &game;
    ctx.config = &config;
    ctx.net_manager = &net_mgr;
    ctx.timeout_limit = 60;

    events = create_cli_network_events (&ctx);
  }

  void
  TearDown () override
  {
    create_cli_network_events (nullptr);
    game_cleanup (&game);
  }
};

#endif // CONTROLLER_TEST_FIXTURE_H