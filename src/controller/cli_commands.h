#ifndef CLI_COMMANDS_H
#define CLI_COMMANDS_H

#include "cli_context.h"

// Declaration so that Readline can use it
char **amazons_completion (const char *text, int start, int end);

// The main order processing function
void handle_command (char *input_buffer, CliContext *ctx);

// Utility function for redrawing the interface (used by commands
// and the network)
void refresh_prompt (CliContext *ctx);

void update_prompt_string (CliContext *ctx);

#endif // CLI_COMMANDS_H