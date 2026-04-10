
#include "tf_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TF_IS_USABLE

// TensorFlow requires a deallocator function for tensors created from C
// arrays. Since we pass a locally scoped array from ml.c, we tell TF to do
// nothing.
static void
dummy_deallocator (void *data, size_t len, void *arg)
{
  (void)data;
  (void)len;
  (void)arg;
}

AmazonsAI *
load_amazons_ai (const char *model_path)
{
  AmazonsAI *ai = (AmazonsAI *)malloc (sizeof (AmazonsAI));
  if (!ai)
    return NULL;

  ai->status = TF_NewStatus ();
  ai->graph = TF_NewGraph ();

  TF_SessionOptions *sess_opts = TF_NewSessionOptions ();
  const char *tags[] = { "serve" };

  ai->session = TF_LoadSessionFromSavedModel (
      sess_opts, NULL, model_path, tags, 1, ai->graph, NULL, ai->status);

  TF_DeleteSessionOptions (sess_opts);

  if (TF_GetCode (ai->status) != TF_OK)
    {
      fprintf (stderr, "[TF Error] Failed to load model: %s\n",
               TF_Message (ai->status));
      TF_DeleteGraph (ai->graph);
      TF_DeleteStatus (ai->status);
      free (ai);
      return NULL;
    }

  printf ("[TF] Successfully loaded model from: %s\n", model_path);
  return ai;
}

float
predict_board_value (AmazonsAI *ai, float *board_data)
{
  if (!ai || !ai->session)
    return 0.5f; // Fallback to neutral if model isn't loaded

  // 1. Prepare Input Node (Name extracted from saved_model_cli)
  TF_Output input_op
      = { TF_GraphOperationByName (ai->graph, "serving_default_keras_tensor"),
          0 };
  if (input_op.oper == NULL)
    {
      fprintf (stderr, "[TF Error] Input node not found in graph!\n");
      return 0.5f;
    }

  // 2. Wrap C array in a TF_Tensor
  int64_t dims[] = { 1, 11, 11, 4 }; // Batch=1, Rows=11, Cols=11, Channels=4
  size_t data_size = 11 * 11 * 4 * sizeof (float);
  TF_Tensor *input_tensor = TF_NewTensor (TF_FLOAT, dims, 4, board_data,
                                          data_size, &dummy_deallocator, NULL);

  // 3. Prepare Output Node (Name extracted from saved_model_cli)
  TF_Output output_op
      = { TF_GraphOperationByName (ai->graph, "StatefulPartitionedCall_1"),
          0 };
  if (output_op.oper == NULL)
    {
      fprintf (stderr, "[TF Error] Output node not found in graph!\n");
      TF_DeleteTensor (input_tensor);
      return 0.5f;
    }

  // 4. Run Inference
  TF_Tensor *output_tensor = NULL;
  TF_SessionRun (ai->session, NULL, &input_op, &input_tensor, 1, &output_op,
                 &output_tensor, 1, NULL, 0, NULL, ai->status);

  if (TF_GetCode (ai->status) != TF_OK)
    {
      fprintf (stderr, "[TF Error] Inference failed: %s\n",
               TF_Message (ai->status));
      TF_DeleteTensor (input_tensor);
      return 0.5f;
    }

  // 5. Extract Result
  float *out_data = (float *)TF_TensorData (output_tensor);
  float win_prob = out_data[0];

  // 6. Cleanup Memory
  TF_DeleteTensor (input_tensor);
  TF_DeleteTensor (output_tensor);

  return win_prob;
}

void
free_amazons_ai (AmazonsAI *ai)
{
  if (ai)
    {
      if (ai->session)
        {
          TF_CloseSession (ai->session, ai->status);
          TF_DeleteSession (ai->session, ai->status);
        }
      if (ai->graph)
        TF_DeleteGraph (ai->graph);
      if (ai->status)
        TF_DeleteStatus (ai->status);
      free (ai);
      printf ("[TF] Model cleaned up safely.\n");
    }
}
#else

// Should not be called, meant for fallback.

AmazonsAI *
load_amazons_ai (const char *model_path)
{
  (void)model_path;
  fprintf (stderr,
           "[Warning] TF_IS_USABLE is false. Cannot load TensorFlow model.\n");
  return NULL;
}

float
predict_board_value (AmazonsAI *ai, float *board_data)
{
  (void)ai;
  (void)board_data;
  return 0.5f;
}

void
free_amazons_ai (AmazonsAI *ai)
{
  (void)ai;
}

#endif