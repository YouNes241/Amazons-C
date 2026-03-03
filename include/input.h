#ifndef INPUT_H
#define INPUT_H

/**
 * Structure simple pour stocker une position pour l'instant 
 */
typedef struct 
{
    int x; 
    int y; 
} pos_t;


 //Traduit une chaîne type "e2" en structure coord_t.
 //Retourne 0 si succès, -1 si le format est invalide.
 
int parse_coordinates(const char *input, pos_t *out_pos);

#endif 