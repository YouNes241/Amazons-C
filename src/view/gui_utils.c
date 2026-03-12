#include "gui_utils.h"
#include "config.h"
#include "game_engine.h"
#include "game_types.h"
#include <assert.h>
#include <gtk/gtk.h>

AppContext *
initAppContext (game_config_t *config)
{
  AppContext *ctx = malloc (sizeof (AppContext));
  assert (ctx);

  ctx->gameConfig = config;
  ctx->gameState = NULL;
  ctx->mainWindow = NULL;
  ctx->mainStack = NULL;
  ctx->gameBoard
      = malloc (ctx->gameConfig->board_size * ctx->gameConfig->board_size
                * sizeof (GtkWidget *));
  assert (ctx->gameBoard);

  ctx->currentPhase = PHASE_MOVE_QUEEN;

  return ctx;
}

void
freeAppContext (AppContext *ctx)
{
  assert (ctx);

  if (ctx->gameBoard)
    free (ctx->gameBoard);
  if (ctx->gameState)
    game_cleanup (ctx->gameState);
  free (ctx);
}

void
drawPiece (AppContext *ctx, int x, int y, CellContent content)
{
  int size = ctx->gameConfig->board_size;
  int index = y * size + x;

  GtkWidget *cellBox = ctx->gameBoard[index];
  GtkWidget *label
      = GTK_WIDGET (g_object_get_data (G_OBJECT (cellBox), "cell_label"));

  gtk_widget_remove_css_class (cellBox, "piece-white");
  gtk_widget_remove_css_class (cellBox, "piece-black");
  gtk_widget_remove_css_class (cellBox, "piece-arrow");
  gtk_label_set_text (GTK_LABEL (label), "");

  switch (content)
    {
    case CONTENT_WHITE_QUEEN:
      gtk_label_set_text (GTK_LABEL (label), "♛");
      gtk_widget_add_css_class (cellBox, "piece-white");
      break;

    case CONTENT_BLACK_QUEEN:
      gtk_label_set_text (GTK_LABEL (label), "♛");
      gtk_widget_add_css_class (cellBox, "piece-black");
      break;

    case CONTENT_ARROW:
      gtk_label_set_text (GTK_LABEL (label), "🔥");
      gtk_widget_add_css_class (cellBox, "piece-arrow");
      break;

    case CONTENT_EMPTY:
      break;
    }
}

void
drawBoard (AppContext *ctx)
{
  int size = ctx->gameConfig->board_size;

  for (int y = 0; y < size; y++)
    {
      for (int x = 0; x < size; x++)
        {
          drawPiece (ctx, x, y, ctx->gameState->board[y][x]);
        }
    }
}

void
clearHighlights (AppContext *ctx)
{
  int size = ctx->gameConfig->board_size;
  for (int i = 0; i < size * size; i++)
    {
      gtk_widget_remove_css_class (ctx->gameBoard[i], "highlight-valid");
      g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "is_legal_move",
                         GINT_TO_POINTER (0));
    }
}

void
highlightReachableSquares (AppContext *ctx, int srcX, int srcY)
{
  clearHighlights (ctx);

  int size = ctx->gameConfig->board_size;
  uint8_t start_index = coords_to_index (srcX, srcY, size);

  Bitboard bb_white, bb_black, bb_arrows;
  extract_bitboards_from_state (ctx->gameState, &bb_white, &bb_black,
                                &bb_arrows);

  Bitboard obstacles
      = bitboard_or (bb_white, bitboard_or (bb_black, bb_arrows));
  bitboard_clear_bit (&obstacles, start_index);
  Bitboard reachable = get_reachable_squares (obstacles, start_index, size);

  for (int i = 0; i < size * size; i++)
    {
      if (bitboard_get_bit (reachable, i))
        {
          gtk_widget_add_css_class (ctx->gameBoard[i], "highlight-valid");
          g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "is_legal_move",
                             GINT_TO_POINTER (1));
        }
      else
        {
          g_object_set_data (G_OBJECT (ctx->gameBoard[i]), "is_legal_move",
                             GINT_TO_POINTER (0));
        }
    }
}