#ifndef I_SELECTOR_H
#define I_SELECTOR_H

typedef struct MCTS_Node MCTS_Node;
typedef struct ISelector ISelector;

/**
 * @struct ISelector_VTable
 * @brief Virtual table for the ISelector interface, defining the function
 * pointers for selecting a child node and cleaning up resources.
 * @var ISelector_VTable::select_child
 * Selects a child node using this selector
 * @var ISelector_VTable::clean
 * Cleans the data of this selector
 */
typedef struct
{
  MCTS_Node *(*select_child) (const ISelector *self, const MCTS_Node *parent);
  void (*clean) (ISelector *self);
} ISelector_VTable;

/**
 * @struct ISelector
 * @brief Interface for selection strategies used in the MCTS algorithm. Each
 * selector must implement the select_child function to choose a child node
 * based on its strategy, and a clean function to free any allocated resources.
 * @var ISelector::vtable
 * Functions usable by this selector
 * @var ISelector::context
 * Context of the selector (parameters)
 */
struct ISelector
{
  const ISelector_VTable *vtable;
  void *context;
};

#endif
