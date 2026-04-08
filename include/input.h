#ifndef INPUT_H
#define INPUT_H
#include "game_types.h"

// Translates a string like "e2" into a coord_t structure.
// Returns 0 on success, -1 if the format is invalid.
int parse_coordinates (const char *input, Move *move);
#endif