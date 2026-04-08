#include "save.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

  // NOUVELLE LOGIQUE D'ECRITURE DES IA
  fprintf (file, "ai_white=%s\n",
           config->ai_white_cfg.enabled ? config->ai_white_cfg.name : "false");
  fprintf (file, "ai_black=%s\n",
           config->ai_black_cfg.enabled ? config->ai_black_cfg.name : "false");

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
      fprintf (stderr, "ERROR: Cannot create save file %s\n", filepath);
      return -1;
    }

  write_settings (file, config);
  write_game_state (file, state);
  write_history (file, state);

  fclose (file);

  return 0;
}

/**
 * @brief Cleans a line from the save file by removing comments and trimming
 * whitespace. It handles both line comments (starting with #) and block
 * comments (enclosed in { }).
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

/**
 * @brief Parses a line from the [settings] section and fills the game_config_t
 * structure accordingly.
 */
static void
parse_settings_line (const char *trimmed, game_config_t *config,
                     GameState *state, int *row_index)
{
  char key[KEY_SIZE], val_str[VAL_SIZE];

  if (sscanf (trimmed, "%63[^=]=%63s", key, val_str) == 2)
    {
      if (strcmp (key, "board_size") == 0)
        {
          int new_size = atoi (val_str);
          if (new_size >= MIN_BOARD_SIZE && new_size <= MAX_BOARD_SIZE)
            {
              config->board_size = new_size;
            }
          else
            {
              config->board_size = DEFAULT_BOARD_SIZE;
            }
          state->size = config->board_size;
          *row_index = state->size - 1;
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
          if (strcmp (val_str, "false") == 0)
            {
              config->ai_white_cfg.enabled = false;
            }
          else
            {
              config->ai_white_cfg.enabled = true;
              if (strcmp (val_str, "true") == 0)
                {
                  strncpy (config->ai_white_cfg.name, "minimax",
                           MAX_AI_STR_LEN - 1);
                }
              else
                {
                  strncpy (config->ai_white_cfg.name, val_str,
                           MAX_AI_STR_LEN - 1);
                }
            }
        }
      else if (strcmp (key, "ai_black") == 0)
        {
          if (strcmp (val_str, "false") == 0)
            {
              config->ai_black_cfg.enabled = false;
            }
          else
            {
              config->ai_black_cfg.enabled = true;
              if (strcmp (val_str, "true") == 0)
                {
                  strncpy (config->ai_black_cfg.name, "minimax",
                           MAX_AI_STR_LEN - 1);
                }
              else
                {
                  strncpy (config->ai_black_cfg.name, val_str,
                           MAX_AI_STR_LEN - 1);
                }
            }
        }
      else if (strcmp (key, "verbose") == 0)
        {
          config->verbose = (strcmp (val_str, "true") == 0);
        }
      else if (strcmp (key, "language") == 0)
        {
          config->language = (strcmp (val_str, "fr") == 0) ? LANG_FR : LANG_EN;
        }
      else if (strcmp (key, "mode") == 0)
        {
          if (strcmp (val_str, "gui") == 0)
            config->mode = MODE_GUI;
          else if (strcmp (val_str, "contest") == 0)
            config->mode = MODE_CONTEST;
          else
            config->mode = MODE_INTERACTIVE;
        }
    }
}

/**
 * @brief Parses a line from the [game] section and fills the GameState's
 * board or current player accordingly. The row_index pointer is used to keep
 * track of which row of the board we are filling, and it is updated as we
 * read lines of the board.
 */
static void
parse_board_line (const char *trimmed, GameState *state, int *row_index)
{
  if (strcmp (trimmed, "W") == 0)
    {
      state->current_player = PLAYER_WHITE;
      return;
    }
  else if (strcmp (trimmed, "B") == 0)
    {
      state->current_player = PLAYER_BLACK;
      return;
    }

  if (*row_index >= 0 && *row_index < state->size)
    {
      int col = 0;
      for (int i = 0; trimmed[i] != '\0' && col < state->size; i++)
        {
          char c = trimmed[i];
          if (c == ' ' || c == '\t')
            continue;

          switch (c)
            {
            case 'Q':
              state->board[*row_index][col] = CONTENT_WHITE_QUEEN;
              break;
            case 'q':
              state->board[*row_index][col] = CONTENT_BLACK_QUEEN;
              break;
            case 'x':
            case 'X':
              state->board[*row_index][col] = CONTENT_ARROW;
              break;
            case '_':
            default:
              state->board[*row_index][col] = CONTENT_EMPTY;
              break;
            }
          col++;
        }

      while (col < state->size)
        {
          state->board[*row_index][col] = CONTENT_EMPTY;
          col++;
        }

      (*row_index)--;
    }
}

/**
 * @brief Parses a line from the [history] section and adds the move to the
 * undo stack.
 */
static void
parse_history_line (const char *trimmed, GameState *state)
{
  char qs_c, qd_c, ad_c;
  int qs_r, qd_r, ad_r;

  if (sscanf (trimmed, "%c%d-%c%d(%c%d)", &qs_c, &qs_r, &qd_c, &qd_r, &ad_c,
              &ad_r)
      == 6)
    {
      Move m;
      m.queen_src.col = qs_c - 'a';
      m.queen_src.row = qs_r - 1;
      m.queen_dst.col = qd_c - 'a';
      m.queen_dst.row = qd_r - 1;
      m.arrow_dst.col = ad_c - 'a';
      m.arrow_dst.row = ad_r - 1;

      if (m.queen_src.col < state->size && m.queen_src.row < state->size
          && m.queen_dst.col < state->size && m.queen_dst.row < state->size
          && m.arrow_dst.col < state->size && m.arrow_dst.row < state->size)
        {
          if (state->undo_stack.top < state->undo_stack.capacity - 1)
            {
              state->undo_stack.top++;
              state->undo_stack.data[state->undo_stack.top] = m;
            }
        }
    }
}

/**
 * @brief Main function to load a game from a file. It reads the file line by
 * line, determines which section it is in, and calls the appropriate parsing
 * function for each line. It also initializes the undo and redo stacks.
 */
int
load_game (const char *filepath, game_config_t *config, GameState *state)
{
  FILE *file = fopen (filepath, "r");
  if (file == NULL)
    {
      fprintf (stderr, "ERROR : Cannot open file %s\n", filepath);
      return -1;
    }

  char raw_line[BUFFER_SIZE];
  char line[BUFFER_SIZE];
  bool in_block_comment = false;

  int current_section = 0; // 1=settings, 2=game, 3=history
  int row_index = -1;

  state->undo_stack.top = -1;
  state->redo_stack.top = -1;

  while (fgets (raw_line, sizeof (raw_line), file))
    {
      clean_line (raw_line, line, &in_block_comment);
      char *trimmed = line;
      while (isspace ((unsigned char)*trimmed))
        trimmed++;

      if (strlen (trimmed) == 0)
        continue;

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

      if (current_section == 1)
        {
          parse_settings_line (trimmed, config, state, &row_index);
        }
      else if (current_section == 2)
        {
          parse_board_line (trimmed, state, &row_index);
        }
      else if (current_section == 3)
        {
          parse_history_line (trimmed, state);
        }
    }

  fclose (file);
  return 0;
}