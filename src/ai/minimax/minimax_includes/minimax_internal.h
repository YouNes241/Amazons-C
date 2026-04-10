/**
 * @file minimax_internal.h
 * @brief Internal function declarations for the Minimax AI module.
 *
 * This header exposes internal minimax functions that are used for move
 * scoring, pruning decisions, and search control. These functions are
 * primarily intended for unit testing but are kept separate from the public
 * API (include/minimax.h) to maintain encapsulation.
 *
 * This header should only be included by test files and internal minimax
 * implementation files, not by external code.
 *
 * Design note — coverage
 * ----------------------
 * Function bodies are NOT defined here. They live exclusively in minimax.c,
 * tagged with MINIMAX_STATIC (= static in production, empty in test mode).
 * This ensures gcovr instruments a single copy and reports correct coverage.
 *
 * In production  : functions are `static` → inlined by the compiler,
 *                  invisible outside minimax.c, no declaration needed here.
 * In test mode   : MINIMAX_STATIC is empty → functions are external symbols.
 *                  The declarations below allow the test TU to call them.
 */

#ifndef MINIMAX_INTERNAL_H
#define MINIMAX_INTERNAL_H

/** @brief Number of best moves kept for the maximizing player (forward
 * pruning). */
extern const int NB_BEST_MOVES_PLAYER;

/** @brief Number of best moves kept for the minimizing player (forward
 * pruning). */
extern const int NB_BEST_MOVES_OPPONENT;

/** @brief Score indicating worst possible outcome (used for initialization).
 */
extern const float BOTTOM_SCORE;

/** @brief Score indicating best possible outcome (used for initialization). */
extern const float TOP_SCORE;

#include "minimax.h"
#include "utils.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef MINIMAX_TEST_MODE

/**
 * @brief Gets the optimal beam width for forward pruning based on depth.
 * @param maximizing_player Indicates if the current player is the maximizing
 * player.
 * @param current_depth The current depth in the search tree.
 * @param max_depth The maximum depth of the search tree.
 * @return The calculated beam width for the current search depth and player.
 */
int get_beam_width (int maximizing_player, int current_depth, int max_depth);

/**
 * @brief Evaluates a move based on the reachable squares from the target.
 * @param state The current board state before the move.
 * @param move The move to score.
 * @return Score based on mobility from the destination square.
 */
float score_move (const BitboardState *state, Move move);

/**
 * @brief Determines if the alpha-beta search should stop at this node.
 * @param data The Minimax_Data containing time limits and state.
 * @param state The current board state.
 * @param depth The current depth in the search tree.
 * @param start_time The starting time of the algorithm.
 * @return True if search should terminate, false otherwise.
 */
bool alpha_beta_conditions (const Minimax_Data *data,
                            const BitboardState *state, int depth,
                            unsigned long start_time);

/**
 * @brief Updates the best move and alpha-beta bounds.
 * @param is_maximizing Indicates if the current player is the maximizing
 * player.
 * @param score The score of the candidate move.
 * @param candidate The move being evaluated.
 * @param best_move Pointer to update with the best move found.
 * @param best_score Pointer to update with the best score.
 * @param alpha Pointer to the alpha value (lower bound).
 * @param beta Pointer to the beta value (upper bound).
 */
void update_best_move (bool is_maximizing, float score, const Move candidate,
                       Move *best_move, float *best_score, float *alpha,
                       float *beta);

/**
 * @brief Get the top moves for a given board state, applying forward pruning
 * to limit the number of moves considered at each level of the Minimax
 * search. The moves are scored and sorted based on their proximity to the
 * center + a tiny of randomness.
 *
 * @param state The current board state to evaluate.
 * @param beam_limit The maximum number of top moves to return.
 * @param maximizing Indicates if the current player is the maximizing
 * player.
 * @param top_moves Output array to store the top moves with their scores.
 * Should be at least of size beam_limit.
 * @return int The actual number of top moves returned, which may be less
 * than or equal to beam_limit depending on the number of legal moves
 * available.
 */
int get_top_moves (const BitboardState *state, int beam_limit,
                   ScoredMove *top_moves);

/**
 * @brief Updates (inline) a given array of moves by moving the best move found
 * to the front of the array, and shifting other moves accordingly.
 * @note This is used to improve move ordering in iterative deepening, to have
 * more chance of pruning.
 *
 * @param count The number of moves in the available_moves array.
 * @param best_move_overall The best move found in the previous iteration.
 * @param available_moves The array of available moves to reorder. Mind that it
 * should be of size at least count.
 */
void sort_moves (int count, const Move best_move_overall,
                 Move *available_moves);

/**
 * @brief Searches for the best move from the root of the Minimax search
 * tree, applying forward pruning and alpha-beta pruning to efficiently
 * evaluate the best move within the given time constraints. Called when
 * initializing a move search.
 *
 * @param data The Minimax_Data containing configuration and evaluation
 * function.
 * @param state The current board state to evaluate and find the best move
 * for.
 * @param depth The maximum depth to search in the Minimax tree.
 * @param start_time The starting time of the search, used to enforce time
 * limits.
 * @return Move The best move found for the current player based on the
 * Minimax evaluation. If no moves are available, returns a default move with
 * all fields set to zero.
 */
Move run_root_search (const Minimax_Data *data, const BitboardState *state,
                      int depth, unsigned long start_time,
                      Move *available_moves, int count);

/**
 * @brief Get the possible moves for a queen at a given index, based on the
 * occupied squares.
 *
 * @param queen_index The linear index of the queen.
 * @param occupied Bitboard representing all occupied squares (queens and
 * arrows).
 * @param size The dimension of the board.
 * @param moves_array Output array to store the generated moves. Mind that it
 * should be large enough to hold all possible moves.
 *
 * @return int The number of moves generated and stored in moves_array.
 */
int get_queen_moves (uint8_t queen_index, Bitboard occupied, uint8_t size,
                     Move *moves_array);

/**
 * @brief Get the possible moves for the arrow based on the queen moves and the
 * occupied squares.
 *
 * @param queen_moves Array of moves representing the queen's potential moves.
 * @param queen_move_count The number of queen moves in the queen_moves array.
 * @param occupied Bitboard representing all occupied squares (queens and
 * arrows).
 * @param size The dimension of the board.
 * @param final_moves Output array to store the final moves with arrow
 * destinations.
 *
 * @return int The number of moves generated and stored in final_moves.
 */
int get_arrow_moves (const Move *queen_moves, int queen_move_count,
                     Bitboard occupied, uint8_t size, Move *final_moves);

/**
 * @brief Picks the top moves from a list of raw moves based on their scores
 * and whether the current player is maximizing or minimizing.
 * @param state The current board state to evaluate the moves against.
 * @param raw_moves An array of potential moves to evaluate.
 * @param count The total number of raw moves available.
 * @param limit The maximum number of top moves to select.
 * @param maximizing A boolean indicating if the current player is the
 * maximizing player.
 * @param best_moves An array to store the top moves that are selected based
 * on their scores.
 * @return The actual number of moves selected as the best moves.
 */
int pick_top_moves (const BitboardState *state, const Move *raw_moves,
                    int count, int limit, ScoredMove *best_moves);

/**
 * @brief Implements the Minimax algorithm with alpha-beta pruning and
 * forward pruning to evaluate the best move for the current player.
 * @param data The Minimax_Data containing configuration and evaluation
 * function.
 * @param state The current board state to evaluate.
 * @param depth The current depth in the search tree.
 * @param alpha The best score that the maximizing player can guarantee at
 * this level or above.
 * @param beta The best score that the minimizing player can guarantee at
 * this level or above.
 * @param maximizing A boolean indicating if the current player is the
 * maximizing player.
 * @param start_time The starting time of the algorithm
 * @return The best score for the current player at this level of the search
 * tree.
 */
float alpha_beta_forward (const Minimax_Data *data, const BitboardState *state,
                          int depth, float alpha, float beta, bool maximizing,
                          unsigned long start_time);

#endif /* MINIMAX_TEST_MODE */

#endif /* MINIMAX_INTERNAL_H */
