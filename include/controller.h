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

void run_interactive_mode (game_config_t *config);
void run_gui_mode (game_config_t *config);
#endif