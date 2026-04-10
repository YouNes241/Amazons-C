
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

/** @brief Macro to indicate that Readline should not clear the screen.
 * @details This value is used to tell Readline not to erase the screen when
 * displaying the prompt.
 */
#define READLINE_NO_CLEAR 0 // Used to tell Readline not to erase

/** @brief Timeout for poll on keyboard input.
 * @details This value represents the timeout in milliseconds for polling
 * keyboard input.
 */
#define POLL_STDIN_TIMEOUT_MS 100 // Timeout for poll on keyboard input

/** @brief Number of file descriptors for poll.
 * @details This value represents the number of file descriptors to poll (just
 * STDIN).
 */
#define STDIN_FD_COUNT 1 // Number of descriptors for poll (just STDIN)

/** @brief Index of STDIN in the file descriptors table.
 * @details This value represents the index of STDIN in the array of file
 * descriptors used for polling.
 */
#define STDIN_FD_INDEX 0 // STDIN index in the fds table

/** @brief Maximum length of an IP address string.
 * @details This value represents the maximum number of characters in an IP
 * address string.
 */
#define MAX_IP_LEN 63

/** @brief Maximum length of a username string.
 * @details This value represents the maximum number of characters in a
 * username string.
 */
#define MAX_USERNAME_LEN 31

/** @brief Maximum length of the CLI prompt string.
 * @details This value represents the maximum number of characters in the CLI
 * prompt string.
 */
#define PROMPT_MAX_LEN 64

/** @brief Buffer size for displaying players in the lobby.
 * @details This value represents the size of the buffer used to display player
 * information in the lobby.
 */
#define LOBBY_LIST_BUFFER_SIZE 1024 // Buffer size for displaying players

/**
 * @brief Groups the overall state of the application in CLI mode.
 */
typedef struct
{
  /** @brief Pointer to the current game state. */
  GameState *current_game;
  /** @brief Pointer to the game configuration. */
  game_config_t *config;
  /** @brief Pointer to the network manager. */
  NetworkManager *net_manager;
  /** @brief Flag indicating if the player is away. */
  bool is_away;
  /** @brief Flag indicating if the game is running. */
  bool game_running;
  /** @brief Flag indicating if the player is in a network match. */
  bool in_network_match;
  /** @brief Flag indicating if the player is in the main menu. */
  bool in_main_menu;

  /** @brief The player's role in the game. */
  PlayerColor
      my_role; // Black or White in a network game, ignored in local mode

  // AI
  /** @brief The AI interface for the white player. */
  AI_Interface ai_white;
  /** @brief Flag indicating if the white AI is initialized. */
  bool ai_white_initialized;

  /** @brief The AI interface for the black player. */
  AI_Interface ai_black;
  /** @brief Flag indicating if the black AI is initialized. */
  bool ai_black_initialized;
  // Network
  /** @brief The ID of the last invited player in the lobby. */
  int last_invited_id; // To keep track of the last invited player in the lobby
  // Timer
  /** @brief The start time of the current turn. */
  time_t turn_start_time;
  /** @brief The timeout limit for the current turn. */
  int timeout_limit;
  /** @brief Flag indicating if the timeout has been sent. */
  bool timeout_sent;

} CliContext;

#endif // CLI_CONTEXT_H
