#ifndef CLI_COMMANDS_H
#define CLI_COMMANDS_H

#include "cli_context.h"

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
#define CMD_LEN_INVITE 7
#define CMD_LEN_ACCEPT 7
#define CMD_LEN_DECLINE 8
#define CMD_SPACE_OFFSET                                                      \
  1 // Indentation to ignore the space after a command (e.g., "join")

// Declaration so that Readline can use it
char **amazons_completion (const char *text, int start, int end);

// The main order processing function
void handle_command (char *input_buffer, CliContext *ctx);

// Utility function for redrawing the interface (used by commands
// and the network)
void refresh_prompt (CliContext *ctx);

void update_prompt_string (CliContext *ctx);

#endif // CLI_COMMANDS_H