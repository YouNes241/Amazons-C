#include "input.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Transforme une chaîne "d4-d7(g10)" en structure Move.
 * @return 0 si succès, -1 si le format est invalide.
 */
int
parse_coordinates (const char *input, Move *move)
{
  char c_src, c_dst, c_arr; // Pour stocker les colonnes
  int r_src, r_dst, r_arr;  // Pour stocker lignes

  /* sscanf lit la chaîne selon un format.*/
  int result = sscanf (input, " %c%d - %c%d ( %c%d )", &c_src, &r_src, &c_dst,
                       &r_dst, &c_arr, &r_arr);

  // Entrée invalide si on a pas exactement 6 elts
  if (result != 6)
    {
      return -1;
    }

  /* Conversion des lettres en indices de colonnes */
  int col_src = (c_src >= 'a' && c_src <= 'z')   ? c_src - 'a'
                : (c_src >= 'A' && c_src <= 'Z') ? c_src - 'A'
                                                 : -1;

  int col_dst = (c_dst >= 'a' && c_dst <= 'z')   ? c_dst - 'a'
                : (c_dst >= 'A' && c_dst <= 'Z') ? c_dst - 'A'
                                                 : -1;

  int col_arr = (c_arr >= 'a' && c_arr <= 'z')   ? c_arr - 'a'
                : (c_arr >= 'A' && c_arr <= 'Z') ? c_arr - 'A'
                                                 : -1;

  if (col_src == -1 || col_dst == -1 || col_arr == -1)
    {
      return -1;
    }

  /* Remplissage de la structure Move avec les coordonnées converties.
   * Note : les lignes sont indexées à partir de 0 dans la structure, d'où le
   * -1.
   */
  move->queen_src.col = (uint8_t)col_src;
  move->queen_src.row = (uint8_t)(r_src - 1);

  move->queen_dst.col = (uint8_t)col_dst;
  move->queen_dst.row = (uint8_t)(r_dst - 1);

  move->arrow_dst.col = (uint8_t)col_arr;
  move->arrow_dst.row = (uint8_t)(r_arr - 1);

  return 0;
}