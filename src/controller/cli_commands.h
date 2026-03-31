/**
 * @file cli_commands.h
 * @brief Header for CLI command handling in the Amazons game.
 *
 * This header defines the functions and constants used for processing user
 * commands in the command-line interface (CLI) of the Amazons game. It
 * includes the main command handler function, as well as utilities for
 * managing the CLI prompt and command completion.
 */

#ifndef CLI_COMMANDS_H
#define CLI_COMMANDS_H

#include "cli_context.h"

// Controller-specific return codes
#define PARSE_MOVE_SUCCESS 0

// Command lengths (for strncmp)
#define POS_STRING_MAX_LEN 6
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 64
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
#define CMD_LEN_QUIT 4
#define CMD_LEN_SET 3
#define CMD_SPACE_OFFSET                                                      \
  1 // Indentation to ignore the space after a command (e.g., "join")

/** @brief Completion function for Readline.
 *  @param text The text to complete.
 *  @param start The start position of the text.
 *  @param end The end position of the text.
 *  @return A list of possible completions.
 */
char **amazons_completion (const char *text, int start, int end);

/** @brief Handles a user command.
 *  @param input_buffer The buffer containing the user's input.
 *  @param ctx The CLI context.
 */
void handle_command (char *input_buffer, CliContext *ctx);

/** @brief Refreshes the CLI prompt.
 *  @param ctx The CLI context.
 */
void refresh_prompt (CliContext *ctx);

/** @brief Updates the prompt string based on the current state.
 *  @param ctx The CLI context.
 */
void update_prompt_string (CliContext *ctx);

void execute_hint (CliContext *ctx);

#endif // CLI_COMMANDS_H