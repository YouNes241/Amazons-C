#include "save.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 512
#define KEY_SIZE 64
#define VAL_SIZE 64

/**
 * @brief Writes the [settings] section to the save file.
 * @param file   Output file handle.
 * @param config Current game configuration.
 */
static void
write_settings (FILE *file, game_config_t *config)
{
  fprintf (file, "[settings]\n");

  fprintf (file, "board_size=%d\n", config->board_size);
  fprintf (file, "time_per_player=%d\n", config->time_per_player);

  fprintf (file, "blitz_mode=%s\n", config->blitz_mode ? "true" : "false");

  // NEW AI WRITING LOGIC
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

/**
 * @brief Writes the [game] section and board layout to the save file.
 * @param file  Output file handle.
 * @param state Current game state.
 */
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

/**
 * @brief Writes the [history] section from the undo stack to the save file.
 * @param file  Output file handle.
 * @param state Current game state.
 */
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

      fprintf (file, "%c%d-%c%d/%c%d\n", q_src_col, q_src_row, q_dst_col,
               q_dst_row, a_dst_col, a_dst_row);
    }

  fprintf (file, "\n");
}

/**
 * @brief Saves the current game configuration, board, and move history.
 * @param filepath Destination file path.
 * @param config   Current configuration.
 * @param state    Current game state.
 * @return 0 on success, -1 on error.
 */
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
  int j = 0; // Output line index; it advances only when we add a useful
             // character
  for (int i = 0; input[i] != '\0' && input[i] != '\n' && input[i] != '\r';
       i++) // i iterates over the input line
    {
      if (*in_block_comment)
        {
          if (input[i] == '}')
            *in_block_comment = false; // end of block comment
          continue;
        }

      if (input[i] == '{')
        {
          *in_block_comment = true;
          continue; // ignore the opening brace and everything that follows
                    // until the closing brace
        }

      if (input[i] == '#')
        break; // rest of the line is a comment

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
                     int *row_index)
{
  char key[KEY_SIZE], val_str[VAL_SIZE];

  if (sscanf (trimmed, "%63[^=]=%63s", key, val_str) == 2)
    {
      if (strcmp (key, "board_size") == 0)
        {
          int new_size = atoi (val_str);
          if (new_size < MIN_BOARD_SIZE || new_size > MAX_BOARD_SIZE)
            {
              new_size = DEFAULT_BOARD_SIZE;
            }

          config->board_size = new_size;
          *row_index = new_size - 1;
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
                  snprintf (config->ai_white_cfg.name, MAX_AI_STR_LEN,
                            "minimax");
                }
              else
                {
                  snprintf (config->ai_white_cfg.name, MAX_AI_STR_LEN, "%.31s",
                            val_str);
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
                  snprintf (config->ai_black_cfg.name, MAX_AI_STR_LEN,
                            "minimax");
                }
              else
                {
                  snprintf (config->ai_black_cfg.name, MAX_AI_STR_LEN, "%.31s",
                            val_str);
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
 * @param trimmed  A cleaned line from the game section.
 * @param state    The game state to modify.
 * @param row_index Pointer to the current board row being filled.
 * @return 0 on success, -1 on error.
 */
static int
parse_board_line (const char *trimmed, GameState *state, int *row_index)
{
  if (strcmp (trimmed, "W") == 0)
    {
      state->current_player = PLAYER_WHITE;
      return 0;
    }
  else if (strcmp (trimmed, "B") == 0)
    {
      state->current_player = PLAYER_BLACK;
      return 0;
    }

  if (*row_index < 0 || *row_index >= state->size)
    {
      return -1;
    }

  int col = 0;
  for (int i = 0; trimmed[i] != '\0'; i++)
    {
      char c = trimmed[i];
      if (c == ' ' || c == '\t')
        continue;

      // too many columns on this board row
      if (col >= state->size)
        {
          return -1;
        }

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
          state->board[*row_index][col] = CONTENT_EMPTY;
          break;
        default:
          return -1; // invalid character in board row
        }
      col++;
    }

  // not enough columns on this board row
  if (col < state->size)
    {
      return -1;
    }

  (*row_index)--;
  return 0;
}

/**
 * @brief Parses a line from the [history] section and adds the move to the
 * undo stack.
 * @param trimmed A cleaned line from the history section.
 * @param state   The game state to modify.
 * @return 0 on success, -1 on error.
 */
static int
parse_history_line (const char *trimmed, GameState *state)
{
  char qs_c, qd_c, ad_c;
  int qs_r, qd_r, ad_r;

  if (sscanf (trimmed, "%c%d-%c%d/%c%d", &qs_c, &qs_r, &qd_c, &qd_r, &ad_c,
              &ad_r)
      != 6)
    {
      return -1;
    }

  int qsc_idx = qs_c - 'a';
  int qsr_idx = qs_r - 1;
  int qdc_idx = qd_c - 'a';
  int qdr_idx = qd_r - 1;
  int adc_idx = ad_c - 'a';
  int adr_idx = ad_r - 1;

  if (qsc_idx < 0 || qsc_idx >= state->size || qsr_idx < 0
      || qsr_idx >= state->size || qdc_idx < 0 || qdc_idx >= state->size
      || qdr_idx < 0 || qdr_idx >= state->size || adc_idx < 0
      || adc_idx >= state->size || adr_idx < 0 || adr_idx >= state->size)
    {
      return -1;
    }

  Move m;
  m.queen_src.col = qsc_idx;
  m.queen_src.row = qsr_idx;
  m.queen_dst.col = qdc_idx;
  m.queen_dst.row = qdr_idx;
  m.arrow_dst.col = adc_idx;
  m.arrow_dst.row = adr_idx;

  if (state->undo_stack.top >= state->undo_stack.capacity - 1)
    {
      return -1;
    }

  state->undo_stack.top++;
  state->undo_stack.data[state->undo_stack.top] = m;

  return 0;
}

/**
 * @brief Loads a saved game file into the provided configuration and state.
 * @param filepath Path to the save file.
 * @param config   Configuration object to populate from the file.
 * @param state    Game state object to populate from the file.
 * @return SAVE_OK on success, or one of the SaveReturn error codes.
 */
SaveReturn
load_game (const char *filepath, game_config_t *config, GameState *state)
{
  FILE *file = fopen (filepath, "r");
  if (file == NULL)
    {
      return ERR_FILE_NOT_FOUND;
    }

  char raw_line[BUFFER_SIZE];
  char line[BUFFER_SIZE];
  bool in_block_comment = false;

  int current_section = 0; // 1=settings, 2=game, 3=history
  int row_index = -1;
  int allocated_memory = state->size;

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
          if (current_section == 2 && row_index != -1)
            {
              fclose (file);
              return ERR_BOARD_SIZE_MISMATCH;
            }
          current_section = 3;
          continue;
        }

      if (current_section == 1)
        {
          parse_settings_line (trimmed, config, &row_index);
          if (config->board_size > allocated_memory)
            {
              fclose (file);
              return ERR_BOARD_SIZE_MISMATCH;
            }
          state->size = config->board_size;
        }
      else if (current_section == 2)
        {
          if (parse_board_line (trimmed, state, &row_index) == -1)
            {
              fclose (file);
              return ERR_CORRUPT_GAME_SECTION;
            }
        }
      else if (current_section == 3)
        {
          if (parse_history_line (trimmed, state) == -1)
            {
              fclose (file);
              return ERR_CORRUPT_HISTORY_SECTION;
            }
        }
    }

  fclose (file);
  if (current_section == 2 && row_index != -1)
    {
      return ERR_PREMATURE_EOF;
    }

  int white_queens = 0;
  int black_queens = 0;

  for (int r = 0; r < state->size; r++)
    {
      for (int c = 0; c < state->size; c++)
        {
          if (state->board[r][c] == CONTENT_WHITE_QUEEN)
            {
              white_queens++;
            }
          else if (state->board[r][c] == CONTENT_BLACK_QUEEN)
            {
              black_queens++;
            }
        }
    }

  if (white_queens != 4 || black_queens != 4)
    {
      return ERR_INVALID_QUEEN_COUNT;
    }

  return SAVE_OK;
}