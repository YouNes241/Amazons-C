/**
 * @file mcts_internal.h
 * @brief Internal function declarations and helpers for the MCTS AI module.
 *
 * This header exposes internal MCTS functions that are used for tree
 * selection, expansion, simulation, and backpropagation. These functions are
 * primarily intended for unit testing but are kept separate from the public
 * API (include/mcts.h) to maintain encapsulation.
 *
 * This header should only be included by test files and internal MCTS
 * implementation files, not by external code.
 *
 * Design note — coverage
 * ----------------------
 * Function bodies are NOT defined here. They live exclusively in mcts.c,
 * tagged with MCTS_STATIC (= static in production, empty in test mode).
 * This ensures gcovr instruments a single copy and reports correct coverage.
 *
 * In production  : functions are `static` → inlined by the compiler,
 *                  invisible outside mcts.c, no declaration needed here.
 * In test mode   : MCTS_STATIC is empty → functions are external symbols.
 *                  The declarations below allow the test TU to call them.
 */

#ifndef MCTS_INTERNAL_H
#define MCTS_INTERNAL_H

#include "bitboard_state.h"
#include "game_types.h"
#include "i_selector.h"
#include "mcts.h"

/** @brief Default time constraint for MCTS (milliseconds). */
extern const uint64_t DEFAULT_TIME_CONSTRAINT;

/** @brief Default maximum iterations for MCTS search. */
extern const uint32_t DEFAULT_MAX_ITERATIONS;

/** @brief Default maximum number of worker threads. */
extern const int DEFAULT_MAX_WORKERS;

#ifdef MCTS_TEST_MODE

/**
 * @brief Checks if a move is null (queen source and destination are the same).
 * Body defined in mcts.c under MCTS_STATIC.
 *
 * @param m The move to check.
 * @return true if the move is null, false otherwise.
 */
bool is_move_null (const Move *m);

/**
 * @brief Aggregates search results found using MCTS, by simply taking the move
 * that has been explored the most between multiple threads, if any.
 *
 * @param data The MCTS config and memory
 * @param num_threads Number of threads used for the calculation
 * @return Move Best move found in that iteration
 */
Move merge_and_get_best (const MCTS_Data *data, int num_threads);

/**
 * @brief Selects a node to expand using the provided selection strategy.
 * Body defined in mcts.c under MCTS_STATIC.
 */
MCTS_Node *select_node (const ISelector *selector, MCTS_Node *root);

/**
 * @brief Backpropagates the result of a rollout up the MCTS tree, updating
 * visit counts and scores for each node along the path from the given node to
 * the root.
 * @param node The MCTS_Node from which to start backpropagation.
 * @param result The result of the rollout.
 */
void backpropagate (MCTS_Node *node, float result);

/**
 * @brief Gets the win probability of a current bitboard state configuration.
 * @param state The game state.
 * @param starting_player_was_white True if the starting player is white, False
 * otherwise.
 * @param result The probability of win in that configuration.
 */
float get_mcts_win_probability (const BitboardState *state,
                                bool starting_player_was_white);

/**
 * @brief Performs a random rollout (simulation) from the given state until the
 * game ends, and returns the result from the perspective of the player to move
 * at the start of the rollout.
 * @param state The initial BitboardState from which to start the rollout.
 * @return A float value representing the result of the rollout: 1.0 if the
 * starting player wins, 0.0 if the starting player loses.
 */
float rollout (BitboardState state);

/**
 * @brief Allocates a node in MCTS. It is a bit costly (calloc) : avoid calling
 * it more than necessary.
 *
 * @param move The move to be played. (Can be nulled out)
 * @param current_state The state of the board before the move was played.
 * @param parent The parent node.
 * @return MCTS_Node* The MCTS node.
 */
MCTS_Node *alloc_mcts_node (const Move *move,
                            const BitboardState *current_state,
                            MCTS_Node *parent);

/**
 * @brief Expands the given MCTS node by adding a new child node for an untried
 * move. If there aren't any, it will free the untried moves.
 * @param parent The MCTS node to expand.
 * @return The newly created child node corresponding to an untried move, or
 * the parent node if all moves have already been tried (fully expanded).
 */
MCTS_Node *expand (MCTS_Node *parent);

/**
 * @brief Recursively frees the memory allocated for the MCTS tree starting
 * from the given node.
 * @param node The root node of the subtree to free. This function will free
 * all descendants of this node as well.
 * @note Should not be called twice on the same node, as it will lead to
 * double-free errors. Please ensure that the caller sets the pointer to NULL
 * after freeing if there's a possibility of it being freed again.
 */
void free_mcts_tree (MCTS_Node *node);

#endif /* MCTS_TEST_MODE */

#endif /* MCTS_INTERNAL_H */
