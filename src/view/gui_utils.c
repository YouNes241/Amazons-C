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

  GtkWidget *cellBtn = ctx->gameBoard[index];

  gtk_widget_remove_css_class (cellBtn, "piece-white");
  gtk_widget_remove_css_class (cellBtn, "piece-black");
  gtk_widget_remove_css_class (cellBtn, "piece-arrow");
  gtk_button_set_label (GTK_BUTTON (cellBtn), "");

  switch (content)
    {
    case CONTENT_WHITE_QUEEN:
      gtk_button_set_label (GTK_BUTTON (cellBtn), "♕");
      gtk_widget_add_css_class (cellBtn, "piece-white");
      break;

    case CONTENT_BLACK_QUEEN:
      gtk_button_set_label (GTK_BUTTON (cellBtn), "♛");
      gtk_widget_add_css_class (cellBtn, "piece-black");
      break;

    case CONTENT_ARROW:
      gtk_button_set_label (GTK_BUTTON (cellBtn), "🔥");
      gtk_widget_add_css_class (cellBtn, "piece-arrow");
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
          drawPiece (ctx, x, y, ctx->gameState->board[x][y]);
        }
    }
}