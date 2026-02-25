#include "minimax.h"
#include "ai_interface.h"
#include "bitboard.h"
#include "evaluator.h"
#include "game_types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX_NB_OF_MOVES_POSSIBLE 2176
static const int DEFAULT_MAX_DEPTH = 3;
static const int DEFAULT_TIME_LIMIT = 5;
static const int NB_BEST_MOVES_PLAYER = 5;
static const int NB_BEST_MOVES_OPPONENT = 20;
static const float BOTTOM_SCORE = -1000000.0f;
static const float TOP_SCORE = 1000000.0f;
static const char *DEFAULT_EVALUATOR = "me";

typedef struct
{
  Move moves[MAX_NB_OF_MOVES_POSSIBLE]; // The "List"
  int count;                            // The "Size"
} MoveList;

typedef struct
{
  Move move;
  int score;
} ScoredMove;

BitboardState // CHECK
apply_move_bitboard (BitboardState state, Move move)
{
  // Init
  BitboardState next = state;
  Position q_src = move.queen_src;
  Position q_dst = move.queen_dst;
  Position a_dst = move.arrow_dst;
  int size = state.size;

  // Convert pos for bits
  uint8_t src_idx = q_src.row * size + q_src.col;
  uint8_t dst_idx = q_dst.row * size + q_dst.col;
  uint8_t arr_idx = a_dst.row * size + a_dst.col;

  // Who's moving ?
  Bitboard *my_queens
      = (state.white_to_move) ? &next.white_queens : &next.black_queens;

  // Remove
  bitboard_clear_bit (my_queens, src_idx);

  // Place
  bitboard_set_bit (my_queens, dst_idx);
  bitboard_set_bit (&next.arrows, arr_idx);

  next.white_to_move = !state.white_to_move;

  return next;
}

BitboardState // Check
sync_from_logic (GameState *gs)
{
  BitboardState bs = { 0 };
  bs.size = gs->size;
  bs.white_to_move = (gs->current_player == PLAYER_WHITE);
  for (uint8_t r = 0; r < gs->size; r++)
    {
      for (uint8_t c = 0; c < gs->size; c++)
        {
          uint8_t idx = r * gs->size + c;
          switch (gs->board[r][c])
            {
            case CONTENT_WHITE_QUEEN:
              bitboard_set_bit (&bs.white_queens, idx);
              break;
            case CONTENT_BLACK_QUEEN:
              bitboard_set_bit (&bs.black_queens, idx);
              break;
            case CONTENT_ARROW:
              bitboard_set_bit (&bs.arrows, idx);
              break;
            default:
              break;
            }
        }
    }
  return bs;
}

static void // Check
check_param (Param p, const char *key, int *target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = atoi (p.value);
    }
}

static void // Check
check_str_param (Param p, const char *key, char **target)
{
  if (strcmp (p.key, key) == 0)
    {
      *target = strdup (p.value);
    }
}

static void // TODO : run a thread (or more) on the side to compute moves
minimax_init (AI_Interface *self, ConfigMap *params)
{
  Minimax_Data *data = (Minimax_Data *)malloc (sizeof (Minimax_Data));
  if (!data)
    return;

  data->max_depth = DEFAULT_MAX_DEPTH;
  data->time_limit = DEFAULT_TIME_LIMIT;
  char *evaluator = (char *)DEFAULT_EVALUATOR;

  if (params != NULL)
    {
      for (int i = 0; i < params->count; i++)
        {
          check_param (params->list[i], "depth", &data->max_depth);
          check_param (params->list[i], "time_limit", &data->time_limit);
          check_str_param (params->list[i], "evaluator", &evaluator);
        }
    }

  data->evaluate = get_evaluator_by_name (evaluator);

  self->context = data;
  printf ("[Minimax] Init OK\n");
}

Bitboard // CHECK
get_reachable (uint8_t start_index, Bitboard occupied, uint8_t size)
{
  Bitboard reachable = bitboard_create_empty ();
  uint8_t start_r = start_index / size;
  uint8_t start_c = start_index % size;

  for (int8_t dr = -1; dr <= 1; dr++)
    for (int8_t dc = -1; dc <= 1; dc++)
      {
        // origin
        if (dr == 0 && dc == 0)
          continue;

        // any movement
        for (int dist = 1; dist < size; dist++)
          {
            int8_t r = start_r + (dr * dist);
            int8_t c = start_c + (dc * dist);

            if (r < 0 || r >= size || c < 0 || c >= size)
              break;

            uint8_t current_idx = r * size + c;

            // If we hit an obstacle, stop.
            if (bitboard_get_bit (occupied, current_idx))
              break;

            bitboard_set_bit (&reachable, current_idx);
          }
      }
  return reachable;
}

Position
idx_to_pos (uint8_t index, uint8_t size)
{
  return (Position){ .row = index / size, .col = index % size };
}

int // To optimize | CHECK
get_legal_moves (BitboardState state, Move *moves_array)
{
  int count = 0;
  int size = state.size;
  Bitboard occupied = bitboard_or (
      bitboard_or (state.white_queens, state.black_queens), state.arrows);
  Bitboard my_queens
      = state.white_to_move ? state.white_queens : state.black_queens;

  for (uint8_t q_from = 0; q_from < (size * size); q_from++)
    {
      if (!bitboard_get_bit (my_queens, q_from))
        continue; // TODO : optimize

      Bitboard queen_destinations = get_reachable (q_from, occupied, size);

      for (uint8_t q_to = 0; q_to < (size * size); q_to++)
        {
          if (!bitboard_get_bit (queen_destinations, q_to))
            continue;

          Bitboard arrow_occupied = occupied;
          bitboard_clear_bit (&arrow_occupied, q_from);
          bitboard_set_bit (&arrow_occupied, q_to);

          Bitboard arrow_destinations
              = get_reachable (q_to, arrow_occupied, size);

          for (uint8_t arrow_to = 0; arrow_to < (size * size); arrow_to++)
            {
              if (!bitboard_get_bit (arrow_destinations, arrow_to))
                continue;

              // Pack it into your partner's struct format
              if (moves_array != NULL)
                {
                  moves_array[count] = (Move){ idx_to_pos (q_from, size),
                                               idx_to_pos (q_to, size),
                                               idx_to_pos (arrow_to, size) };
                }
              count++;
            }
        }
    }
  return count;
}

static void // CHECK
minimax_clean (AI_Interface *self)
{
  Minimax_Data *data = (Minimax_Data *)self->context;

  free (data);
  self->context = NULL;

  printf ("[Minimax] Clean OK\n");
}

bool // CHECK
is_game_over (BitboardState state)
{
  return get_legal_moves (state, NULL) == 0;
}

static int // Somewhat CHECK
get_beam_width (int maximizing_player, int current_depth, int max_depth)
{
  int base_width
      = maximizing_player ? NB_BEST_MOVES_PLAYER : NB_BEST_MOVES_OPPONENT;

  int depth_bonus = (max_depth - current_depth) * 5;

  return base_width + depth_bonus;
}

static int // Check
compare_moves (const void *a, const void *b)
{
  ScoredMove *sm1 = (ScoredMove *)a;
  ScoredMove *sm2 = (ScoredMove *)b;
  return sm2->score - sm1->score;
}

float
score_move_ordering (BitboardState state, Move move)
{
  // Score based on how close the queen ends up to the middle of the board
  int8_t center = state.size / 2;
  int dist_to_center
      = abs (move.queen_dst.row - center) + abs (move.queen_dst.col - center);

  // We return a higher score for a smaller distance to center
  return (float)(20 - dist_to_center);
}

static float // Check
alpha_beta_forward (Minimax_Data *data, BitboardState state, int depth,
                    float alpha, float beta, bool maximizing)
{
  // End of game
  if (depth <= 0 || is_game_over (state))
    {
      return data->evaluate (&state);
    }

  // vars
  Move raw_moves[MAX_NB_OF_MOVES_POSSIBLE];
  int count = get_legal_moves (state, raw_moves);
  float best_score = maximizing ? BOTTOM_SCORE : TOP_SCORE;

  // No moves available (end of game)
  if (count == 0)
    return best_score;

  ScoredMove sorted_moves[MAX_NB_OF_MOVES_POSSIBLE];

  // Get scores for each move after each move
  for (int i = 0; i < count; i++)
    {
      sorted_moves[i].move = raw_moves[i];
      sorted_moves[i].score = score_move_ordering (state, raw_moves[i]);
    }

  qsort (sorted_moves, count, sizeof (ScoredMove), compare_moves);

  int limit = get_beam_width (maximizing, depth, data->max_depth);
  int moves_to_scan = (count < limit) ? count : limit;

  for (int i = 0; i < moves_to_scan; i++)
    {
      Move move = sorted_moves[i].move;
      BitboardState next_state = apply_move_bitboard (state, move);

      float score = alpha_beta_forward (data, next_state, depth - 1, alpha,
                                        beta, !maximizing);

      if (maximizing)
        {
          best_score = MAX (best_score, score);
          alpha = MAX (alpha, best_score);
        }
      else
        {
          best_score = MIN (best_score, score);
          beta = MIN (beta, best_score);
        }

      if (beta <= alpha)
        break;
    }

  return best_score;
}

static Move // CHECK
minimax_get_best_move (AI_Interface *self, GameState state)
{
  Minimax_Data *data = (Minimax_Data *)self->context;

  printf ("[Minimax] Searching a move (Depth=%d)...\n", data->max_depth);

  BitboardState fast_state = sync_from_logic (&state);

  // 2. Initialize Alpha/Beta variables
  float alpha = BOTTOM_SCORE;
  float beta = TOP_SCORE;

  // 3. We need to find the actual MOVE, not just the score.
  // We can do the first level of the search here to grab the Move object.
  Move best_move = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
  float best_score = BOTTOM_SCORE;

  Move available_moves[MAX_NB_OF_MOVES_POSSIBLE];
  int count = get_legal_moves (fast_state, available_moves);

  for (int i = 0; i < count; i++)
    {
      // Create a copy of the state for this move
      BitboardState next_state
          = apply_move_bitboard (fast_state, available_moves[i]);

      // Start the recursion (notice we pass 'false' because the next layer is
      // minimizing)
      float score = alpha_beta_forward (data, next_state, data->max_depth - 1,
                                        alpha, beta, false);

      if (score > best_score)
        {
          best_score = score;
          best_move = available_moves[i];
        }
      alpha = MAX (alpha, best_score);
    }

  printf ("[Minimax] Best move found with score: %.2f\n", best_score);
  return best_move;
}

const AI_VTable minimax_vtable = { .init = minimax_init,
                                   .clean = minimax_clean,
                                   .get_best_move = minimax_get_best_move };

void
create_minimax_ai (AI_Interface *ai)
{
  ai->vtable = &minimax_vtable;
  ai->context = NULL;
}