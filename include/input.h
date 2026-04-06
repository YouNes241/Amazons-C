#ifndef INPUT_H
#define INPUT_H
#include "game_types.h"

// Traduit une chaîne type "e2" en structure coord_t.
// Retourne 0 si succès, -1 si le format est invalide.
int parse_coordinates (const char *input, Move *move);
#endif