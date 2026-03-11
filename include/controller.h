#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "config.h"

/* ==========================================================================
 * CONSTANTS
 * ==========================================================================
 */
#define DEFAULT_PORT 12345           // default parameter
#define DEFAULT_IP "127.0.0.1"       // default parameter
#define DEFAULT_USERNAME "Anonymous" // default parameter

#define MAX_IP_LEN 63
#define MAX_USERNAME_LEN 31
#define PROMPT_MAX_LEN 64

#define POLL_STDIN_TIMEOUT_MS 100 // Timeout for poll on keyboard input
#define STDIN_FD_COUNT 1    // Number of descriptors for poll (just STDIN)
#define STDIN_FD_INDEX 0    // STDIN index in the fds table
#define READLINE_NO_CLEAR 0 // Used to tell Readline not to erase

// Controller-specific return codes
#define PARSE_MOVE_SUCCESS 0

// Command lengths (for strncmp)
#define CMD_LEN_HELP 4
#define CMD_LEN_DETAILS 7
#define CMD_LEN_SERVER_START 12
#define CMD_LEN_JOIN 4
#define CMD_LEN_NEW 3
#define CMD_LEN_UNDO 4
#define CMD_LEN_REDO 4
#define CMD_LEN_SAVE 5
#define CMD_LEN_LOAD 5
#define CMD_LEN_SHOW 4
#define CMD_SPACE_OFFSET                                                      \
  1 // Indentation to ignore the space after a command (e.g., "join")

void run_interactive_mode (game_config_t *config);

#endif