
/**
 * @file cli_context.h
 * @brief Header for the CLI context structure in the Amazons game.
 *
 * This header defines the CliContext structure, which holds the overall state
 * of the application in CLI mode, including game state, network management,
 * and user interface settings.
 */

#ifndef CLI_CONTEXT_H
#define CLI_CONTEXT_H

#include "ai_interface.h"
#include "config.h"
#include "game_types.h"
#include "network_common.h"
#include <stdbool.h>
#include <time.h>

#define READLINE_NO_CLEAR 0       // Used to tell Readline not to erase
#define POLL_STDIN_TIMEOUT_MS 100 // Timeout for poll on keyboard input
#define STDIN_FD_COUNT 1 // Number of descriptors for poll (just STDIN)
#define STDIN_FD_INDEX 0 // STDIN index in the fds table

#define MAX_IP_LEN 63
#define MAX_USERNAME_LEN 31
#define PROMPT_MAX_LEN 64
#define LOBBY_LIST_BUFFER_SIZE 1024 // Buffer size for displaying players

/**
 * @brief Groups the overall state of the application in CLI mode.
 */
typedef struct
{
  GameState *current_game;
  game_config_t *config;
  NetworkManager *net_manager;
  bool is_away;
  bool game_running;
  bool in_network_match;
  bool in_main_menu;

  PlayerColor
      my_role; // Black or White in a network game, ignored in local mode

  // AI
  AI_Interface ai_white;
  bool ai_white_initialized;

  AI_Interface ai_black;
  bool ai_black_initialized;
  // Network
  int last_invited_id; // To keep track of the last invited player in the lobby
  // Timer
  time_t turn_start_time;
  int timeout_limit;
  bool timeout_sent;

} CliContext;

#endif // CLI_CONTEXT_H