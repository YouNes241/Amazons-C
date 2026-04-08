#ifndef TF_WRAPPER_H
#define TF_WRAPPER_H

#ifdef TF_IS_USABLE
#include <tensorflow/c/c_api.h>

/**
 * @struct AmazonsAI
 * @brief Holds the TensorFlow session, graph, and status for inference.
 */
typedef struct
{
  TF_Graph *graph;
  TF_Session *session;
  TF_Status *status;
} AmazonsAI;

#else

/**
 * @struct AmazonsAI
 * @brief Dummy struct used when TensorFlow is disabled to allow safe
 * compilation.
 */
typedef struct
{
  int dummy_variable_to_prevent_empty_struct;
} AmazonsAI;

#endif // TF_IS_USABLE

/**
 * @brief Loads the SavedModel from the given directory.
 * @param model_path Path to the exported model directory (e.g.,
 * "amazons_model_v2_hidden")
 * @return A pointer to the initialized AmazonsAI struct, or NULL on failure.
 */
AmazonsAI *load_amazons_ai (const char *model_path);

/**
 * @brief Runs the TensorFlow model on a 484-float array representing the
 * board.
 * @param ai The initialized AmazonsAI struct.
 * @param board_data Array of 484 floats (11x11x4).
 * @return The win probability (0.0 to 1.0) from the perspective of the current
 * player.
 */
float predict_board_value (AmazonsAI *ai, float *board_data);

/**
 * @brief Safely frees the TensorFlow session, graph, and memory.
 * @param ai The AmazonsAI struct to clean up.
 */
void free_amazons_ai (AmazonsAI *ai);

#endif /* AI_TF_WRAPPER_H */