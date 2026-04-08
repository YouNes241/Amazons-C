#include "ai_factory.h"
#include "ai_interface.h"
#include "bitboard_state.h"
#include "game_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/**
 * @brief Puts all occupied squares (queens and arrows) into a single bitboard.
 * @param state A game state with pos of queens and arrows.
 * @return Bitboard with all occupied squares.
 */
static Bitboard
get_obstacles (const BitboardState *state)
{
  Bitboard occ = bitboard_or (state->white_queens, state->black_queens);
  return bitboard_or (occ, state->arrows);
}

GameState
setup_standard_start (int board_size)
{
  if (board_size < 4)
    board_size = 4;
  if (board_size > 11)
    board_size = 11;

  CellContent **cc
      = (CellContent **)malloc (sizeof (CellContent *) * board_size);
  for (int i = 0; i < board_size; i++)
    {
      cc[i] = (CellContent *)malloc (sizeof (CellContent) * board_size);
      for (int j = 0; j < board_size; j++)
        {
          cc[i][j] = CONTENT_EMPTY;
        }
    }

  // Standard starting positions for Amazons
  if (board_size == 10)
    {
      cc[0][3] = CONTENT_WHITE_QUEEN;
      cc[0][6] = CONTENT_WHITE_QUEEN;
      cc[3][0] = CONTENT_WHITE_QUEEN;
      cc[3][9] = CONTENT_WHITE_QUEEN;

      cc[6][0] = CONTENT_BLACK_QUEEN;
      cc[6][9] = CONTENT_BLACK_QUEEN;
      cc[9][3] = CONTENT_BLACK_QUEEN;
      cc[9][6] = CONTENT_BLACK_QUEEN;
    }
  else if (board_size == 6)
    {
      cc[0][1] = CONTENT_WHITE_QUEEN;
      cc[0][4] = CONTENT_WHITE_QUEEN;
      cc[1][0] = CONTENT_WHITE_QUEEN;
      cc[1][5] = CONTENT_WHITE_QUEEN;

      cc[4][0] = CONTENT_BLACK_QUEEN;
      cc[4][5] = CONTENT_BLACK_QUEEN;
      cc[5][1] = CONTENT_BLACK_QUEEN;
      cc[5][4] = CONTENT_BLACK_QUEEN;
    }
  else
    {
      // For other sizes, just place queens in corners
      cc[0][0] = CONTENT_BLACK_QUEEN;
      cc[0][board_size - 1] = CONTENT_BLACK_QUEEN;
      cc[board_size - 1][0] = CONTENT_WHITE_QUEEN;
      cc[board_size - 1][board_size - 1] = CONTENT_WHITE_QUEEN;
    }

  GameState new_game = {
    (uint8_t)board_size, // size
    false,               // blitz_mode
    0,                   // turn_count
    PLAYER_WHITE,        // current_player
    GAME_IN_PROGRESS,    // status
    cc,                  // board
    0.0,                 // blitz_turn_start
    0.0,                 // white_time_left
    0.0,                 // black_time_left
    { 0 },               // undo_stack
    { 0 }                // redo_stack
  };

  return new_game;
}

// 1. AI Initialization Helpers
AI_Interface *
api_create_mcts (int time_limit, int max_workers)
{
  AI_Interface *ai = (AI_Interface *)malloc (sizeof (AI_Interface));
  char time_str[20], workers_str[20];
  sprintf (time_str, "%d", time_limit);
  sprintf (workers_str, "%d", max_workers);

  Param p[] = { { "time_limit", time_str }, { "max_workers", workers_str } };
  ConfigMap cm = { 2, p };
  *ai = create_ai ("mcts", cm);
  return ai;
}

AI_Interface *
api_create_minimax (int depth)
{
  AI_Interface *ai = (AI_Interface *)malloc (sizeof (AI_Interface));
  char depth_str[20];
  sprintf (depth_str, "%d", depth);
  Param p[] = { { "depth", depth_str }, { "evaluator", "qd&kd" } };
  ConfigMap cm = { 2, p };
  *ai = create_ai ("minimax", cm);
  return ai;
}

// 2. Game State Management
GameState *
api_init_game (int size)
{
  GameState *gs = (GameState *)malloc (sizeof (GameState));
  *gs = setup_standard_start (size);
  return gs;
}

void
api_free_game (GameState *gs)
{
  game_cleanup (gs);
  free (gs);
}

void
api_free_ai (AI_Interface *ai)
{
  ai->vtable->clean (ai);
  free (ai);
}

// 3. Gameplay Loop
Move
api_get_ai_move (AI_Interface *ai, GameState *gs)
{
  return ai->vtable->get_best_move (ai, *gs);
}

void
api_apply_move (GameState *gs, Move m)
{
  apply_move (gs, m);
}

int
api_check_winner (GameState *gs)
{
  if (is_game_over (gs, gs->current_player))
    {
      if (gs->current_player == PLAYER_WHITE)
        return 2;
      else
        return 1;
    }
  return 0; // Game still going
}

// 4. ML Tensor Exporter
void
api_export_tensor (GameState *gs, float *out_tensor)
{
  BitboardState bb = sync_from_logic (gs);
  int max_size = 11;

  // Clear array
  for (int i = 0; i < max_size * max_size * 4; i++)
    out_tensor[i] = 0.0f;

  for (int r = 0; r < bb.size; r++)
    {
      for (int c = 0; c < bb.size; c++)
        {
          int idx = r * bb.size + c;
          int tensor_idx = r * max_size + c;

          // Channel 3: Valid Board Mask (1.0 for squares inside the board)
          out_tensor[tensor_idx + 3 * max_size * max_size] = 1.0f;

          // Channel 0: White, Channel 1: Black, Channel 2: Arrows
          if (bitboard_get_bit (bb.white_queens, idx))
            out_tensor[tensor_idx + 0 * max_size * max_size] = 1.0f;
          else if (bitboard_get_bit (bb.black_queens, idx))
            out_tensor[tensor_idx + 1 * max_size * max_size] = 1.0f;
          else if (bitboard_get_bit (
                       get_obstacles (&bb),
                       idx)) // Adjust if arrows are stored differently
            out_tensor[tensor_idx + 2 * max_size * max_size] = 1.0f;
        }
    }
}