/** @file cli_parser.h
 * @brief Header file for the command-line interface parser for the Amazons
 * game.
 */

#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "config.h"

/**
 * @brief Parses command line options and populates the game configuration.
 * * @param argc Number of arguments.
 * @param argv Array of argument strings.
 * @param config Pointer to the game configuration struct to populate.
 * @return int EXIT_SUCCESS on success, EXIT_FAILURE on error.
 */
int parse_options (int argc, char *argv[], game_config_t *config);

#endif /* CLI_PARSER_H */