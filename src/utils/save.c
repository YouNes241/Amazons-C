#include "save.h"
#include <ctype.h>
#include <stdbool.h> // Pour avoir le droit d'écrire "true" et "false"
#include <stdio.h>
#include <stdlib.h> // Pour convertir du texte en nombres entiers (atoi)
#include <string.h> // Pour comparer et mesurer du texte (strcmp, strncmp, strlen)

#define BUFFER_SIZE 512
#define KEY_SIZE 64
#define VAL_SIZE 64

static void
write_settings (FILE *file, game_config_t *config)
{
  fprintf (file, "[settings]\n");

  fprintf (file, "board_size=%d\n", config->board_size);
  fprintf (file, "time_per_player=%d\n", config->time_per_player);

  fprintf (file, "blitz_mode=%s\n", config->blitz_mode ? "true" : "false");
  fprintf (file, "ai_white=%s\n", config->ai_white ? "true" : "false");
  fprintf (file, "ai_black=%s\n", config->ai_black ? "true" : "false");
  fprintf (file, "verbose=%s\n", config->verbose ? "true" : "false");

  const char *lang_str = (config->language == LANG_FR) ? "fr" : "en";
  fprintf (file, "language=%s\n", lang_str);

  const char *mode_str = "interactive";
  if (config->mode == MODE_GUI)
    {
      mode_str = "gui";
    }
  else if (config->mode == MODE_CONTEST)
    {
      mode_str = "contest";
    }
  fprintf (file, "mode=%s\n", mode_str);

  fprintf (file, "\n");
}

static void
write_game_state (FILE *file, GameState *state)
{
  fprintf (file, "[game]\n");

  char turn_char = (state->current_player == PLAYER_WHITE) ? 'W' : 'B';
  fprintf (file, "%c\n", turn_char);

  for (int row = state->size - 1; row >= 0; row--)
    {
      for (int col = 0; col < state->size; col++)
        {
          char cell_char = '_';

          switch (state->board[row][col])
            {
            case CONTENT_WHITE_QUEEN:
              cell_char = 'Q';
              break;
            case CONTENT_BLACK_QUEEN:
              cell_char = 'q';
              break;
            case CONTENT_ARROW:
              cell_char = 'x';
              break;
            case CONTENT_EMPTY:
              cell_char = '_';
              break;
            }
          fprintf (file, "%c ", cell_char);
        }
      fprintf (file, "\n");
    }
  fprintf (file, "\n");
}

static void
write_history (FILE *file, GameState *state)
{
  fprintf (file, "[history]\n");

  if (state->undo_stack.top < 0)
    {
      fprintf (file, "\n");
      return;
    }

  for (int i = 0; i <= state->undo_stack.top; i++)
    {
      Move m = state->undo_stack.data[i];

      char q_src_col = m.queen_src.col + 'a';
      char q_dst_col = m.queen_dst.col + 'a';
      char a_dst_col = m.arrow_dst.col + 'a';

      int q_src_row = m.queen_src.row + 1;
      int q_dst_row = m.queen_dst.row + 1;
      int a_dst_row = m.arrow_dst.row + 1;

      fprintf (file, "%c%d-%c%d(%c%d)\n", q_src_col, q_src_row, q_dst_col,
               q_dst_row, a_dst_col, a_dst_row);
    }

  fprintf (file, "\n");
}

int
save_game (const char *filepath, game_config_t *config, GameState *state)
{
  FILE *file = fopen (filepath, "w");

  if (file == NULL)
    {
      fprintf (stderr, "Error: Cannot create save file. %s\n", filepath);
      return -1;
    }

  write_settings (file, config);
  write_game_state (file, state);
  write_history (file, state);

  fclose (file);

  return 0;
}

/* ==========================================================================
   LECTURE DE FICHIER (LOAD)
   ==========================================================================
 */

/**
 * @brief Nettoie une ligne en supprimant les commentaires # et { }
 */
static void
clean_line (const char *input, char *output, bool *in_block_comment)
{
  int j = 0; // Index pour la ligne de sortie, il n'avance que lorsque l'on
             // ajoute un caractère utile
  for (int i = 0; input[i] != '\0' && input[i] != '\n' && input[i] != '\r';
       i++) // i parcourt la ligne d'entrée
    {
      if (*in_block_comment)
        {
          if (input[i] == '}')
            *in_block_comment = false; // on a fini le bloc de commentaire
          continue;
        }

      if (input[i] == '{')
        {
          *in_block_comment = true;
          continue; // on ignore le caractère d'ouverture et tout ce qui suit
                    // jusqu'à la fermeture
        }

      if (input[i] == '#')
        break; // Fin de la ligne utile

      output[j++] = input[i];
    }
  output[j] = '\0';
}

int
load_game (const char *filepath, game_config_t *config, GameState *state)
{
  FILE *file = fopen (filepath, "r");
  if (file == NULL)
    {
      fprintf (stderr, "Erreur : Impossible d'ouvrir le fichier %s\n",
               filepath);
      return -1;
    }

  char raw_line[BUFFER_SIZE]; // Ligne brute lue depuis le fichier
  char line[BUFFER_SIZE];     // Ligne nettoyée (sans commentaires ni espaces
                              // inutiles)
  bool in_block_comment = false;

  int current_section = 0; // 1=settings, 2=game, 3=history
  int row_index = -1;      // Pour suivre la ligne du plateau que l'on remplit
                           // spécifiquement dans la section [game]
  // on le met à -1 pour indiquer que l'on n'a pas encore commencé à remplir le
  // plateau, et on le réinitialisera à size-1 dès que l'on saura la taille du
  // plateau (lors de la lecture des settings)

  state->undo_stack.top = -1;
  state->redo_stack.top = -1;

  while (fgets (raw_line, sizeof (raw_line), file)) // on prend ligne par ligne
    {
      // Nettoyage des commentaires
      clean_line (raw_line, line, &in_block_comment);

      // Suppression des espaces de début de ligne
      char *trimmed = line; // trimmed pointe vers le premier caractère de la
                            // partie utile de la ligne
      while (isspace (
          (unsigned char)*trimmed)) // on avance tant que ce sont des espaces
        trimmed++;

      if (strlen (trimmed) == 0) // Ligne vide ou uniquement des commentaires
        continue;

      // Ici on détermine la section du fichier dans laquelle on se trouve
      if (strncmp (trimmed, "[settings]", 10) == 0)
        {
          current_section = 1;
          continue;
        }
      if (strncmp (trimmed, "[game]", 6) == 0)
        {
          current_section = 2;
          continue;
        }
      if (strncmp (trimmed, "[history]", 9) == 0)
        {
          current_section = 3;
          continue;
        }

      // On est dans la première section qui est settings, on lit les
      // paramètres de configuration
      if (current_section == 1)
        {
          char key[KEY_SIZE], val_str[VAL_SIZE];
          if (sscanf (trimmed, "%63[^=]=%63s", key, val_str)
              == 2) // On lit une ligne au format "clé=valeur"
            {
              if (strcmp (key, "board_size")
                  == 0) // on re-remplit la structure de config à partir du
                        // fichier
                {
                  config->board_size = atoi (val_str);
                  state->size = config->board_size;
                  row_index = state->size - 1;
                }
              else if (strcmp (key, "time_per_player") == 0)
                {
                  config->time_per_player = atoi (val_str);
                }
              else if (strcmp (key, "blitz_mode") == 0)
                {
                  config->blitz_mode = (strcmp (val_str, "true") == 0);
                }
              else if (strcmp (key, "ai_white") == 0)
                {
                  config->ai_white = (strcmp (val_str, "true") == 0);
                }
              else if (strcmp (key, "ai_black") == 0)
                {
                  config->ai_black = (strcmp (val_str, "true") == 0);
                }
              else if (strcmp (key, "verbose") == 0)
                {
                  config->verbose = (strcmp (val_str, "true") == 0);
                }
              else if (strcmp (key, "language") == 0)
                {
                  config->language
                      = (strcmp (val_str, "fr") == 0) ? LANG_FR : LANG_EN;
                }
              else if (strcmp (key, "mode") == 0)
                {
                  if (strcmp (val_str, "gui") == 0)
                    config->mode = MODE_GUI;
                  else if (strcmp (val_str, "contest") == 0)
                    config->mode = MODE_CONTEST;
                  else
                    config->mode = 0; // Mode interactif par défaut
                }
            }
        }

      // On passe à la section game
      else if (current_section == 2)
        {
          if (strcmp (trimmed, "W") == 0)
            state->current_player = PLAYER_WHITE;
          else if (strcmp (trimmed, "B") == 0)
            state->current_player = PLAYER_BLACK;

          else if (row_index
                   >= 0) // si c'est pas la ligne du tour, alors c'est une
                         // ligne du plateau que l'on doit remplir
            {
              int col = 0;
              for (int i = 0; trimmed[i] != '\0' && col < state->size; i++)
                {
                  char c = trimmed[i];
                  if (c == ' ' || c == '\t')
                    continue; // On ignore les espaces

                  switch (c) // on remplit la matrice du plateau en fonction
                             // des caractères lus
                    {
                    case 'Q':
                      state->board[row_index][col] = CONTENT_WHITE_QUEEN;
                      break;
                    case 'q':
                      state->board[row_index][col] = CONTENT_BLACK_QUEEN;
                      break;
                    case 'x':
                    case 'X':
                      state->board[row_index][col] = CONTENT_ARROW;
                      break;
                    case '_':
                      state->board[row_index][col] = CONTENT_EMPTY;
                      break;
                    }
                  col++;
                }
              row_index--;
            }
        }

      // On passe à la section history, on lit les coups joués
      else if (current_section == 3)
        {
          char qs_c, qd_c, ad_c;
          int qs_r, qd_r, ad_r;

          // On cherche le format exact qui est d10-d9(d8)
          if (sscanf (trimmed, "%c%d-%c%d(%c%d)", &qs_c, &qs_r, &qd_c, &qd_r,
                      &ad_c, &ad_r)
              == 6)
            {
              Move m; // on remplit une structure Move à partir de la ligne lue
              m.queen_src.col
                  = qs_c - 'a'; // Convertir la lettre en index de colonne (0
                                // pour 'a', 1 pour 'b', ...)
              m.queen_src.row = qs_r - 1;
              m.queen_dst.col = qd_c - 'a';
              m.queen_dst.row = qd_r - 1;
              m.arrow_dst.col = ad_c - 'a';
              m.arrow_dst.row = ad_r - 1;

              // Ajout manuel dans la pile d'historique des coups joués
              if (state->undo_stack.top
                  < state->undo_stack.capacity
                        - 1) // on vérifie qu'il y a de la place dans la pile
                             // pour ajouter ce coup
                {
                  state->undo_stack.top++; // on avance le sommet de la pile
                  state->undo_stack.data[state->undo_stack.top]
                      = m; // on ajoute le coup au sommet de la pile
                  // dans le tableau data, à l'indice top, on range le
                  // mouvement m que qu'on vient de lire dans le fichier
                }
            }
        }
    }

  fclose (file);
  return 0;
}