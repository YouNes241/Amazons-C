/**
 * @file gui_utils.h
 * @brief Utility functions and core data structures for the GTK GUI.
 *
 * @details This header defines the main graphical context (`GuiContext`),
 * enumerations for the UI interaction phases, data structures for thread
 * workers, and various helper functions to manipulate the GTK interface
 * (drawing the board, handling highlights, updating displays, etc.).
 */

#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include "ai_interface.h"
#include "config.h"
#include "game_types.h"
#include "network_common.h"
#include <gtk/gtk.h>
#include <stdarg.h>

/** @brief Maximum string length for a GTK keyboard shortcut accelerator. */
#define SHORTCUTS_SIZE 16

/** @brief Total number of configurable keyboard shortcuts in the application.
 */
#define SHORTCUTS_COUNT 10

/**
 * @enum GuiPhase
 * @brief Represents the current interaction phase of the local player.
 */
typedef enum
{
  PHASE_MOVE_QUEEN, /**< The player is selecting or moving an Amazon (Queen).
                     */
  PHASE_SHOOT_ARROW /**< The player has moved a Queen and must now shoot an
                       arrow. */
} GuiPhase;

/**
 * @struct KeyBindings
 * @brief Holds the string representations of all keyboard shortcuts.
 */
typedef struct
{
  char key_new_game[SHORTCUTS_SIZE];  /**< Shortcut to start a new game. */
  char key_load_game[SHORTCUTS_SIZE]; /**< Shortcut to load a saved game. */
  char
      key_save_game[SHORTCUTS_SIZE]; /**< Shortcut to save the current game. */
  char key_config[SHORTCUTS_SIZE];   /**< Shortcut to open settings. */
  char key_info[SHORTCUTS_SIZE]; /**< Shortcut to open the info/rules page. */
  char key_quit[SHORTCUTS_SIZE]; /**< Shortcut to quit the application. */
  char key_undo[SHORTCUTS_SIZE]; /**< Shortcut to undo the last move. */
  char key_redo[SHORTCUTS_SIZE]; /**< Shortcut to redo the last undone move. */
  char key_pause[SHORTCUTS_SIZE]; /**< Shortcut to pause the game. */
  char key_hint[SHORTCUTS_SIZE];  /**< Shortcut to ask the AI for a hint. */
} KeyBindings;

/**
 * @struct GuiContext
 * @brief The main graphical context holding the state of the GUI.
 *
 * @details This overarching structure connects the GTK UI layer, the core
 * game engine, the network manager, and local UI states (like interaction
 * phases and temporary drag-and-drop targets).
 */
typedef struct
{
  game_config_t *gameConfig; /**< Global game configuration settings. */
  GameState *gameState;      /**< Core logic state of the game. */
  KeyBindings keyBindings;   /**< User-defined keyboard shortcuts. */

  GtkWindow *mainWindow; /**< The main GTK application window. */
  GtkStack *mainStack;   /**< The GTK Stack managing page navigation. */
  GtkWidget *
      *gameBoard; /**< Array of GTK widgets representing the board cells. */

  GuiPhase currentPhase; /**< Current interaction phase (Move Queen vs Shoot
                            Arrow). */
  Position tmpQueenSrc;  /**< Temporary storage for the origin of the moving
                            Queen. */
  Position tmpQueenDst;  /**< Temporary storage for the destination of the
                            moving Queen. */

  NetworkManager *networkManager; /**< Pointer to the networking subsystem. */
  bool isNetworkMatch; /**< True if the current game is played over the
                          network. */
  PlayerColor
      selfPlayerColor; /**< The local player's color in a network match. */

  bool isHintActive; /**< True if an AI hint is currently displayed on the
                        board. */

  AI_Interface
      ai_white; /**< The AI engine instance assigned to the White player. */
  AI_Interface
      ai_black; /**< The AI engine instance assigned to the Black player. */
  bool is_ai_white_initialized; /**< True if the White AI is actively
                                   configured and loaded. */
  bool is_ai_black_initialized; /**< True if the Black AI is actively
                                   configured and loaded. */
  bool is_ai_thinking; /**< Locks the UI when an AI is currently calculating a
                          move. */
} GuiContext;

/**
 * @struct AIWorkerData
 * @brief Data payload passed to the background AI worker thread.
 *
 * @details This structure prevents the UI thread from freezing by passing a
 * safe copy of the game state to an asynchronous calculation thread.
 */
typedef struct
{
  GuiContext *ctx;      /**< Pointer to the main GUI context. */
  GameState state_copy; /**< A thread-safe copy of the game state. */
  AI_Interface *ai; /**< Pointer to the AI interface calculating the move. */
  Move best_move;   /**< Output parameter: The calculated optimal move. */
} AIWorkerData;

/**
 * @brief Allocates and initializes a new GUI context.
 * @param config Pointer to the game configuration structure.
 * @return A dynamically allocated pointer to the initialized `GuiContext`.
 */
GuiContext *initGuiContext (game_config_t *config);

/**
 * @brief Frees the memory allocated for the GUI context and its resources.
 * @param ctx Pointer to the `GuiContext` to destroy.
 */
void freeGuiContext (GuiContext *ctx);

/**
 * @brief Updates the visual representation of a single cell on the board.
 * @param ctx Pointer to the `GuiContext`.
 * @param x The X coordinate (column) of the cell.
 * @param y The Y coordinate (row) of the cell.
 * @param content The entity to draw (Empty, White Queen, Black Queen, or
 * Arrow).
 */
void drawPiece (GuiContext *ctx, int x, int y, CellContent content);

/**
 * @brief Completely redraws the visual board based on the internal
 * `GameState`.
 * @param ctx Pointer to the `GuiContext`.
 */
void drawBoard (GuiContext *ctx);

/**
 * @brief Clears all valid-move highlights from the board UI.
 * @param ctx Pointer to the `GuiContext`.
 */
void clearHighlights (GuiContext *ctx);

/**
 * @brief Clears AI hint highlights from the board UI.
 * @param ctx Pointer to the `GuiContext`.
 */
void clearHintHighlights (GuiContext *ctx);

/**
 * @brief Highlights all legal destination squares for a specific piece.
 * @param ctx Pointer to the `GuiContext`.
 * @param srcX The X coordinate (column) of the selected piece.
 * @param srcY The Y coordinate (row) of the selected piece.
 */
void highlightReachableSquares (GuiContext *ctx, int srcX, int srcY);

/**
 * @brief Dynamically builds the physical GTK widgets for the game board grid.
 * @param ctx Pointer to the `GuiContext`.
 * @param boardGrid Pointer to the `GtkGrid` container that will hold the
 * cells.
 */
void setupBoard (GuiContext *ctx, GtkWidget *boardGrid);

/**
 * @brief Binds a key-capture listener to a toggle button for shortcut
 * configuration.
 * @param btn The `GtkToggleButton` to convert into a shortcut recorder.
 * @param target_str The string buffer where the captured shortcut will be
 * saved.
 * @param ctx Pointer to the `GuiContext`.
 */
void setupBindingButton (GtkWidget *btn, char *target_str, GuiContext *ctx);

/**
 * @brief Refreshes the application's global shortcut controllers based on
 * current bindings.
 * @param ctx Pointer to the `GuiContext`.
 */
void updateGameShortcuts (GuiContext *ctx);

/**
 * @brief Updates the UI label indicating whose turn it is.
 * @param ctx Pointer to the `GuiContext`.
 */
void updateTurnDisplay (GuiContext *ctx);

/**
 * @brief Visually displays an AI-suggested move (Hint) on the board.
 * @param ctx Pointer to the `GuiContext`.
 * @param q_src The recommended origin position of the Queen.
 * @param q_dst The recommended destination position of the Queen.
 * @param a_dst The recommended target position for the Arrow.
 */
void showHintOnBoard (GuiContext *ctx, Position q_src, Position q_dst,
                      Position a_dst);

/**
 * @brief Logs a formatted message to the built-in server console view.
 * @param ctx Pointer to the `GuiContext`.
 * @param format Printf-style format string.
 * @param ... Additional arguments for the format string.
 */
void gui_log_to_server_console (GuiContext *ctx, const char *format, ...);

/**
 * @brief Displays a non-blocking modal popup message to the user.
 * @param ctx Pointer to the `GuiContext`.
 * @param message The text to display in the popup.
 */
void guiShowPopup (GuiContext *ctx, const char *message);

#endif // GUI_UTILS_H