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

/** @brief Maximum length of a position string.
 * @details This value represents the maximum number of characters that a
 * position string can contain.
 */
#define POS_STRING_MAX_LEN 6

/** @brief Maximum length of a key string.
 * @details This value represents the maximum number of characters that a key
 * string can contain.
 */
#define MAX_KEY_LEN 64

/** @brief Maximum length of a value string.
 * @details This value represents the maximum number of characters that a value
 * string can contain.
 */
#define MAX_VAL_LEN 64

/** @brief Length of the "help" command.
 * @details This value represents the number of characters in the "help"
 * command.
 */
#define CMD_LEN_HELP 4

/** @brief Length of the "details" command.
 * @details This value represents the number of characters in the "details"
 * command.
 */
#define CMD_LEN_DETAILS 7

/** @brief Length of the "server start" command.
 * @details This value represents the number of characters in the "server
 * start" command.
 */
#define CMD_LEN_SERVER_START 12

/** @brief Length of the "join" command.
 * @details This value represents the number of characters in the "join"
 * command.
 */
#define CMD_LEN_JOIN 4

/** @brief Length of the "new" command.
 * @details This value represents the number of characters in the "new"
 * command.
 */
#define CMD_LEN_NEW 3

/** @brief Length of the "undo" command.
 * @details This value represents the number of characters in the "undo"
 * command.
 */
#define CMD_LEN_UNDO 4

/** @brief Length of the "redo" command.
 * @details This value represents the number of characters in the "redo"
 * command.
 */
#define CMD_LEN_REDO 4

/** @brief Length of the "save" command.
 * @details This value represents the number of characters in the "save"
 * command.
 */
#define CMD_LEN_SAVE 4

/** @brief Length of the "load" command.
 * @details This value represents the number of characters in the "load"
 * command.
 */
#define CMD_LEN_LOAD 4

/** @brief Length of the "show" command.
 * @details This value represents the number of characters in the "show"
 * command.
 */
#define CMD_LEN_SHOW 4

/** @brief Length of the "invite" command.
 * @details This value represents the number of characters in the "invite"
 * command.
 */
#define CMD_LEN_INVITE 7

/** @brief Length of the "accept" command.
 * @details This value represents the number of characters in the "accept"
 * command.
 */
#define CMD_LEN_ACCEPT 7

/** @brief Length of the "decline" command.
 * @details This value represents the number of characters in the "decline"
 * command.
 */
#define CMD_LEN_DECLINE 8

/** @brief Length of the "quit" command.
 * @details This value represents the number of characters in the "quit"
 * command.
 */
#define CMD_LEN_QUIT 4

/** @brief Length of the "set" command.
 * @details This value represents the number of characters in the "set"
 * command.
 */
#define CMD_LEN_SET 3

/** @brief Offset for space after a command.
 * @details This value represents the number of characters to skip after a
 * command to ignore the space that follows it (e.g., "join").
 */
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

/**
 * @brief Reinitializes the global CLI context reference.
 * @details This function is primarily used to clean up the state between unit
 * tests. It sets the global CLI context pointer to NULL, ensuring that no
 * stale references persist across tests, which could lead to unintended side
 * effects or memory issues. This is especially important in a testing
 * environment where multiple tests may create and manipulate their own CLI
 * contexts, and we want to ensure a clean slate for each test case.
 */
void clear_global_cli_context (void);

/** @brief Executes the hint command.
 *  @param ctx The CLI context.
 */
void execute_hint (CliContext *ctx);

#endif // CLI_COMMANDS_H
