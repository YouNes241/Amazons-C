#ifndef I_SELECTOR_H
#define I_SELECTOR_H

typedef struct MCTS_Node MCTS_Node;
typedef struct ISelector ISelector;

/**
 * @brief Virtual table for the ISelector interface, defining the function
 * pointers for selecting a child node and cleaning up resources.
 */
typedef struct
{
  MCTS_Node *(*select_child) (ISelector *self, MCTS_Node *parent);
  void (*clean) (ISelector *self);
} ISelector_VTable;

/**
 * @brief Interface for selection strategies used in the MCTS algorithm. Each
 * selector must implement the select_child function to choose a child node
 * based on its strategy, and a clean function to free any allocated resources.
 */
struct ISelector
{
  const ISelector_VTable *vtable;
  void *context;
};

#endif