#include "../../include/game_engine.h"
#include <stddef.h>

uint8_t
coords_to_index (uint8_t x, uint8_t y, uint8_t board_size)
{
  return (y * board_size) + x;
}

void
index_to_coords (uint8_t index, uint8_t board_size, uint8_t *out_x,
                 uint8_t *out_y)
{
  // Check if output pointers are valid and board_size is positive
  if (out_x != NULL && out_y != NULL && board_size > 0)
    {
      // Extract x-coordinate using modulo
      *out_x = index % board_size;
      // Extract y-coordinate using integer division
      *out_y = index / board_size;
    }
}

bool
is_valid_coord (int8_t x, int8_t y, uint8_t board_size)
{
  // Check if both x and y are within [0, board_size)
  return (x >= 0 && x < board_size && y >= 0 && y < board_size);
}

void
game_init (GameState *state)
{
  if (state == NULL)
    return;

  // Default size
  if (state->size == 0)
    {
      state->size = 10;
    }
  uint8_t size = state->size;

  // Memory allocation for the board
  state->board = malloc (size * sizeof (CellContent *));
  if (state->board == NULL)
    {
      return;
    }

  // Lines allocation and initialization
  for (uint8_t y = 0; y < size; y++)
    {
      state->board[y] = malloc (size * sizeof (CellContent));
      if (state->board[y] == NULL)
        {
          return;
        }

      // Initialization of the line to empty cells
      for (uint8_t x = 0; x < size; x++)
        {
          state->board[y][x] = CONTENT_EMPTY;
        }
    }

  // Initializing other state fields
  state->current_player = PLAYER_WHITE;
  state->status = GAME_IN_PROGRESS;
  state->turn_count = 0;

  state->board[0][3] = CONTENT_WHITE_QUEEN;
  state->board[0][size - 4] = CONTENT_WHITE_QUEEN;
  state->board[3][0] = CONTENT_WHITE_QUEEN;
  state->board[3][size - 1] = CONTENT_WHITE_QUEEN;

  state->board[size - 1][3] = CONTENT_BLACK_QUEEN;
  state->board[size - 1][size - 4] = CONTENT_BLACK_QUEEN;
  state->board[size - 4][0] = CONTENT_BLACK_QUEEN;
  state->board[size - 4][size - 1] = CONTENT_BLACK_QUEEN;
}

void
game_cleanup (GameState *state)
{
  if (state != NULL && state->board != NULL)
    {

      // Free each line of the board
      for (uint8_t y = 0; y < state->size; y++)
        {
          if (state->board[y] != NULL)
            {
              free (state->board[y]);
              state->board[y] = NULL;
            }
        }

      // Free the array of line pointers
      free (state->board);
      state->board = NULL;
      state->size = 0;
    }
}

void
apply_move (GameState *state, Move move)
{
  if (state == NULL || state->board == NULL)
    return;

  // Get the moving queen's color
  CellContent moving_queen
      = state->board[move.queen_src.row][move.queen_src.col];

  // Empty the source cell of the queen
  state->board[move.queen_src.row][move.queen_src.col]
      = CONTENT_EMPTY; // Adapt according to your Enum

  // Place the queen in its new cell
  state->board[move.queen_dst.row][move.queen_dst.col] = moving_queen;

  // Place the arrow in its cell
  state->board[move.arrow_dst.row][move.arrow_dst.col] = CONTENT_ARROW;

  // Change the turn to the next player and increment the turn count
  state->turn_count++;

  if (state->current_player == PLAYER_WHITE)
    {
      state->current_player = PLAYER_BLACK;
    }
  else
    {
      state->current_player = PLAYER_WHITE;
    }
}

// Helper function to check if a path is clear between two positions
static bool
is_path_clear (Bitboard obstacles, Position src, Position dst,
               uint8_t board_size)
{
  int8_t dx = dst.col - src.col;
  int8_t dy = dst.row - src.row;

  // Check if the move is strictly horizontal, vertical, or diagonal
  if (dx != 0 && dy != 0 && abs (dx) != abs (dy))
    {
      return false;
    }

  // If there is no movement, it's not a valid path
  if (dx == 0 && dy == 0)
    return false;

  // Determine the step direction for x and y
  int8_t step_x = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
  int8_t step_y = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

  int8_t current_x = src.col + step_x;
  int8_t current_y = src.row + step_y;

  while (current_x != dst.col || current_y != dst.row)
    {
      uint8_t current_idx = coords_to_index (current_x, current_y, board_size);

      if (bitboard_is_set (obstacles, current_idx))
        {
          return false;
        }

      current_x += step_x;
      current_y += step_y;
    }

  // Check if the destination cell is empty
  uint8_t dst_idx = coords_to_index (dst.col, dst.row, board_size);
  if (bitboard_is_set (obstacles, dst_idx))
    {
      return false;
    }

  return true;
}

bool
is_move_legal (const GameState *state, Move move)
{
  if (state == NULL)
    return false;
  uint8_t size = state->size;

  // Check if all coordinates are on the board
  if (!is_valid_coord (move.queen_src.col, move.queen_src.row, size)
      || !is_valid_coord (move.queen_dst.col, move.queen_dst.row, size)
      || !is_valid_coord (move.arrow_dst.col, move.arrow_dst.row, size))
    {
      return false;
    }

  // Matrix to bitboards
  Bitboard bb_white, bb_black, bb_arrows;
  extract_bitboards_from_state (state, &bb_white, &bb_black, &bb_arrows);

  Bitboard obstacles
      = bitboard_or (bb_white, bitboard_or (bb_black, bb_arrows));

  // Check that the starting square contains a queen of the current player
  uint8_t src_idx
      = coords_to_index (move.queen_src.col, move.queen_src.row, size);
  if (state->current_player == PLAYER_WHITE)
    {
      if (!bitboard_is_set (bb_white, src_idx))
        return false;
    }
  else
    {
      if (!bitboard_is_set (bb_black, src_idx))
        return false;
    }

  // Check the Queen's movement
  if (!is_path_clear (obstacles, move.queen_src, move.queen_dst, size))
    {
      return false;
    }

  // The queen just moved. Its starting pos is now empty
  // and its destination pos has become an obstacle for the arrow
  // We update the obstacles Bitboard
  uint8_t dst_idx
      = coords_to_index (move.queen_dst.col, move.queen_dst.row, size);
  bitboard_clear_bit (&obstacles, src_idx); // The queen is no longer there
  bitboard_set_bit (&obstacles, dst_idx);   // The queen is here

  // Check the arrow shot
  if (!is_path_clear (obstacles, move.queen_dst, move.arrow_dst, size))
    {
      return false;
    }

  return true;
}

bool
is_game_over (const GameState *state, PlayerColor current_player)
{
  if (state == NULL)
    return true;

  uint8_t size = state->size;
  uint8_t max_index = size * size;

  Bitboard bb_white, bb_black, bb_arrows;
  extract_bitboards_from_state (state, &bb_white, &bb_black, &bb_arrows);

  // Merge all obstacles
  Bitboard obstacles
      = bitboard_or (bb_white, bitboard_or (bb_black, bb_arrows));

  // Determine which queens to check
  Bitboard current_queens
      = (current_player == PLAYER_WHITE) ? bb_white : bb_black;

  // Traverse all squares on the board (from 0 to size*size - 1)
  for (uint8_t idx = 0; idx < max_index; idx++)
    {

      // If we find one of our queens on this square
      if (bitboard_is_set (current_queens, idx))
        {

          // We calculate all squares it can move to
          Bitboard reachable
              = engine_get_reachable_squares (obstacles, idx, size);

          // If the bitboard reachable is not empty, the queen can move
          // and the game is not over yet
          if (!bitboard_is_empty (reachable))
            {
              return false;
            }
        }
    }

  return true;
}