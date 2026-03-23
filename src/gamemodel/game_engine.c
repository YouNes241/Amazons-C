#include "../../include/game_engine.h"
#include <stddef.h>

static void
push_move (MoveStack *stack, Move m)
{
  if (stack->top < stack->capacity - 1)
    {
      stack->top++;
      stack->data[stack->top] = m;
    }
}

static Move
pop_move (MoveStack *stack, bool *success)
{
  Move popped_move = { 0 };

  if (stack->top >= 0)
    {
      *success = true;
      popped_move = stack->data[stack->top];
      stack->top--;
    }
  else
    {
      *success = false;
    }

  return popped_move;
}

static void
clear_stack (MoveStack *stack)
{
  stack->top = -1;
}

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

static void
place_initial_queens (GameState *state)
{
  if (state == NULL || state->board == NULL)
    return;

  uint8_t size = state->size;

  const uint8_t FIRST_ROW = 0;
  const uint8_t FIRST_COL = 0;
  const uint8_t LAST_ROW = size - 1;
  const uint8_t LAST_COL = size - 1;

  const uint8_t OFFSET = size / 3;
  const uint8_t OPP_OFFSET = LAST_COL - OFFSET;

  const uint8_t white_coords[4][2] = { { FIRST_ROW, OFFSET },
                                       { FIRST_ROW, OPP_OFFSET },
                                       { OFFSET, FIRST_COL },
                                       { OFFSET, LAST_COL } };

  const uint8_t black_coords[4][2] = { { LAST_ROW, OFFSET },
                                       { LAST_ROW, OPP_OFFSET },
                                       { LAST_ROW - OFFSET, FIRST_COL },
                                       { LAST_ROW - OFFSET, LAST_COL } };

  for (int i = 0; i < 4; i++)
    {
      uint8_t w_row = white_coords[i][0];
      uint8_t w_col = white_coords[i][1];
      state->board[w_row][w_col] = CONTENT_WHITE_QUEEN;

      uint8_t b_row = black_coords[i][0];
      uint8_t b_col = black_coords[i][1];
      state->board[b_row][b_col] = CONTENT_BLACK_QUEEN;
    }
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

  int max_possible_moves = 2000; // Largement suffisant pour une partie

  state->undo_stack.capacity = max_possible_moves;
  state->undo_stack.top = -1;
  state->undo_stack.data = malloc (max_possible_moves * sizeof (Move));

  state->redo_stack.capacity = max_possible_moves;
  state->redo_stack.top = -1;
  state->redo_stack.data = malloc (max_possible_moves * sizeof (Move));
  place_initial_queens (state);
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
      if (state->undo_stack.data != NULL)
        {
          free (state->undo_stack.data);
          state->undo_stack.data = NULL;
        }

      if (state->redo_stack.data != NULL)
        {
          free (state->redo_stack.data);
          state->redo_stack.data = NULL;
        }
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
  state->board[move.queen_src.row][move.queen_src.col] = CONTENT_EMPTY;

  // Place the queen in its new cell
  state->board[move.queen_dst.row][move.queen_dst.col] = moving_queen;

  // Place the arrow in its cell
  state->board[move.arrow_dst.row][move.arrow_dst.col] = CONTENT_ARROW;

  // Change the turn to the next player and increment the turn count
  state->turn_count++;
  push_move (&state->undo_stack, move);
  clear_stack (&state->redo_stack);

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

      if (bitboard_get_bit (obstacles, current_idx))
        {
          return false;
        }

      current_x += step_x;
      current_y += step_y;
    }

  // Check if the destination cell is empty
  uint8_t dst_idx = coords_to_index (dst.col, dst.row, board_size);
  if (bitboard_get_bit (obstacles, dst_idx))
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

  if (!(is_valid_coord (move.queen_src.col, move.queen_src.row, size)
        && is_valid_coord (move.queen_dst.col, move.queen_dst.row, size)
        && is_valid_coord (move.arrow_dst.col, move.arrow_dst.row, size)))
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
      if (!bitboard_get_bit (bb_white, src_idx))
        return false;
    }
  else
    {
      if (!bitboard_get_bit (bb_black, src_idx))
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
has_legal_move (Bitboard obstacles, uint8_t start_index, uint8_t board_size)
{
  uint8_t start_x = 0, start_y = 0;
  index_to_coords (start_index, board_size, &start_x, &start_y);

  // The 8 possible directions for a queen
  int8_t directions[8][2] = { { 0, -1 },  { 0, 1 },  { -1, 0 }, { 1, 0 },
                              { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };

  // Check the immediate neighbor in each of the 8 directions
  for (int i = 0; i < 8; i++)
    {
      int8_t check_x = start_x + directions[i][0];
      int8_t check_y = start_y + directions[i][1];

      if (is_valid_coord (check_x, check_y, board_size))
        {
          uint8_t current_idx = coords_to_index (check_x, check_y, board_size);

          // If it's not an obstacle
          if (!bitboard_get_bit (obstacles, current_idx))
            {
              return true;
            }
        }
    }

  // If we checked all 8 directions and couldn't return true, we are trapped.
  return false;
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
      if (bitboard_get_bit (current_queens, idx))
        {
          if (has_legal_move (obstacles, idx, size))
            {
              return false;
            }
        }
    }

  return true;
}

void
extract_bitboards_from_state (const GameState *state, Bitboard *bb_white,
                              Bitboard *bb_black, Bitboard *bb_arrows)
{
  *bb_white = bitboard_create_empty ();
  *bb_black = bitboard_create_empty ();
  *bb_arrows = bitboard_create_empty ();

  for (uint8_t r = 0; r < state->size; r++)
    {
      for (uint8_t c = 0; c < state->size; c++)
        {
          uint8_t idx = coords_to_index (c, r, state->size);

          switch (state->board[r][c])
            {
            case CONTENT_WHITE_QUEEN:
              bitboard_set_bit (bb_white, idx);
              break;
            case CONTENT_BLACK_QUEEN:
              bitboard_set_bit (bb_black, idx);
              break;
            case CONTENT_ARROW:
              bitboard_set_bit (bb_arrows, idx);
              break;
            default:
              break;
            }
        }
    }
}

Bitboard
get_reachable_squares (Bitboard obstacles, uint8_t start_index,
                       uint8_t board_size)
{
  Bitboard reachable = bitboard_create_empty ();
  uint8_t start_r = 0, start_c = 0;
  index_to_coords (start_index, board_size, &start_c, &start_r);

  // The 8 possible directions for a queen
  for (int8_t dr = -1; dr <= 1; dr++)
    {
      for (int8_t dc = -1; dc <= 1; dc++)
        {
          if (dr == 0 && dc == 0)
            continue;

          for (int dist = 1; dist < board_size; dist++)
            {
              int8_t r = start_r + (dr * dist);
              int8_t c = start_c + (dc * dist);

              if (r < 0 || r >= board_size || c < 0 || c >= board_size)
                break;
              uint8_t current_idx = coords_to_index (c, r, board_size);

              // If we hit an obstacle, we can't go further in this direction
              if (bitboard_get_bit (obstacles, current_idx))
                break;

              bitboard_set_bit (&reachable, current_idx);
            }
        }
    }
  return reachable;
}

int
generate_all_moves (Bitboard bb_white, Bitboard bb_black, Bitboard bb_arrows,
                    uint8_t board_size, PlayerColor current_player,
                    Move *out_moves)
{
  int count = 0;
  Bitboard obstacles
      = bitboard_or (bb_white, bitboard_or (bb_black, bb_arrows));
  Bitboard my_queens = (current_player == PLAYER_WHITE) ? bb_white : bb_black;

  // Copy to a temporary variable because it will be modified with pop_lsb
  Bitboard temp_queens = my_queens;

  while (!bitboard_is_zero (temp_queens))
    {
      uint8_t q_from = pop_lsb (&temp_queens);
      Bitboard queen_destinations
          = get_reachable_squares (obstacles, q_from, board_size);

      Bitboard temp_q_dests = queen_destinations;
      while (!bitboard_is_zero (temp_q_dests))
        {
          uint8_t q_to = pop_lsb (&temp_q_dests);

          // Simulate the queen's move
          Bitboard arrow_obstacles = obstacles;
          bitboard_clear_bit (&arrow_obstacles, q_from);
          bitboard_set_bit (&arrow_obstacles, q_to);

          Bitboard arrow_destinations
              = get_reachable_squares (arrow_obstacles, q_to, board_size);

          Bitboard temp_a_dests = arrow_destinations;
          while (!bitboard_is_zero (temp_a_dests))
            {
              uint8_t arrow_to = pop_lsb (&temp_a_dests);

              if (out_moves != NULL)
                {
                  uint8_t q_from_x, q_from_y, q_to_x, q_to_y, a_to_x, a_to_y;
                  q_from_x = q_from_y = q_to_x = q_to_y = a_to_x = a_to_y = 0;
                  index_to_coords (q_from, board_size, &q_from_x, &q_from_y);
                  index_to_coords (q_to, board_size, &q_to_x, &q_to_y);
                  index_to_coords (arrow_to, board_size, &a_to_x, &a_to_y);

                  out_moves[count].queen_src.col = q_from_x;
                  out_moves[count].queen_src.row = q_from_y;
                  out_moves[count].queen_dst.col = q_to_x;
                  out_moves[count].queen_dst.row = q_to_y;
                  out_moves[count].arrow_dst.col = a_to_x;
                  out_moves[count].arrow_dst.row = a_to_y;
                }
              count++;
            }
        }
    }
  return count;
}

PlayerColor
get_winner (const GameState *state)
{
  if (!is_game_over (state, state->current_player))
    {
      return NO_PLAYER;
    }

  if (state->current_player == PLAYER_WHITE)
    {
      return PLAYER_BLACK;
    }
  else
    {
      return PLAYER_WHITE;
    }
}

bool
undo_move (GameState *state)
{
  bool success;
  Move last_move = pop_move (&state->undo_stack, &success);

  if (!success)
    return false;

  state->board[last_move.arrow_dst.row][last_move.arrow_dst.col]
      = CONTENT_EMPTY;

  CellContent queen_color
      = state->board[last_move.queen_dst.row][last_move.queen_dst.col];
  state->board[last_move.queen_dst.row][last_move.queen_dst.col]
      = CONTENT_EMPTY;
  state->board[last_move.queen_src.row][last_move.queen_src.col] = queen_color;

  state->current_player
      = (state->current_player == PLAYER_WHITE) ? PLAYER_BLACK : PLAYER_WHITE;
  state->turn_count--;
  state->status = GAME_IN_PROGRESS;

  push_move (&state->redo_stack, last_move);
  return true;
}

bool
redo_move (GameState *state)
{
  bool success;
  Move next_move = pop_move (&state->redo_stack, &success);

  if (!success)
    return false;
  apply_move (state, next_move);

  return true;
}