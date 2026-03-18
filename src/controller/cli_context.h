#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H

#include "ai_interface.h"
#include "config.h"
#include "game_types.h"
#include "network_common.h"
#include <stdbool.h>
#include <time.h>

/**
 * @brief Groups the overall state of the application in CLI mode.
 */
typedef struct
{
  GameState *current_game;
  game_config_t *config;
  NetworkManager *net_manager;

  bool game_running;
  bool in_network_match;
  bool in_main_menu;

  PlayerColor
      my_role; // Black or White in a network game, ignored in local mode

  // IA
  AI_Interface ai;
  bool ai_initialized;

  // Timer
  time_t turn_start_time;
  int timeout_limit;
  bool timeout_sent;
} CliContext;

#endif // CLI_CONTEXT_H