#include "../../include/save.h"
#include <stdio.h>

static void
write_settings (FILE *file, game_config_t *config)
{
  fprintf (file, "[settings]\n");

  fprintf (file, "board_size=%d\n", config->board_size);
  fprintf (file, "time_per_player=%d\n", config->time_per_player);

  fprintf (file, "blitz_mode=%s\n", config->blitz_mode ? "true" : "false");
  fprintf (file, "ai_enabled=%s\n", config->ai_enabled ? "true" : "false");
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
  // TODO : Boucler sur la liste des coups joués quand elle sera disponible
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