/**
 * @file gui_callbacks.h
 * @brief GTK signal handlers and UI callbacks for the application.
 *
 * @details This header defines all the callback functions triggered by user
 * interactions within the GTK interface. It includes handlers for button
 * clicks, board interactions (drag-and-drop, mouse clicks), configuration
 * changes, keyboard shortcuts, and network-related actions.
 */

#ifndef GUI_CALLBACKS_H
#define GUI_CALLBACKS_H

#include "gui_utils.h"
#include <gtk/gtk.h>

/**
 * @name Navigation and General UI Callbacks
 * @{
 */

/**
 * @brief Handles general page navigation within the UI stack.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void navigationClick (GtkButton *button, gpointer user_data);

/**
 * @brief Handles the application quit button click.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void quitButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Opens the Information/Rules popup.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void infoPopupButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Opens the Configuration/Settings popup.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void configPopupButtonClick (GtkButton *button, gpointer user_data);

/** @} */

/**
 * @name Game Control Callbacks
 * @{
 */

/**
 * @brief Undoes the last move.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void undoButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Redoes the previously undone move.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void redoButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Triggers the AI to calculate and highlight a recommended move.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void hintButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Exits the current game and returns to the main menu.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void quitGameButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Opens the popup to create a new local game.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void createGameButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Starts a new local game immediately.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void newGameButtonClick (GtkButton *button, gpointer user_data);

/** @} */

/**
 * @name Networking Callbacks
 * @{
 */

/**
 * @brief Connects the client to a specified multiplayer server.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void joinServerButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Refreshes the list of locally discovered UDP servers.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void refreshServerListButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Disconnects the client from the current server.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void disconnectFromServerButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Sends a match invitation to a specific player ID in the lobby.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void sendInviteButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Opens the popup to configure and host a new server.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void hostServerPopupButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Starts the background server process using the provided settings.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void startServerButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Stops the active server process.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void stopServerButtonClick (GtkButton *button, gpointer user_data);

/** @} */

/**
 * @name Board Interaction Callbacks (Drag & Drop, Clicks)
 * @{
 */

/**
 * @brief Called when a drag operation begins on a board cell (moving a Queen).
 * @param source The drag source object.
 * @param x The X coordinate of the drag start.
 * @param y The Y coordinate of the drag start.
 * @param user_data Pointer to the GuiContext.
 * @return A GdkContentProvider representing the dragged data, or NULL if
 * invalid.
 */
GdkContentProvider *onDrag (GtkDragSource *source, double x, double y,
                            gpointer user_data);

/**
 * @brief Called when a dragged Queen is dropped onto a target cell.
 * @param target The drop target object.
 * @param value The value provided by the drag source.
 * @param x The X coordinate of the drop.
 * @param y The Y coordinate of the drop.
 * @param user_data Pointer to the GuiContext.
 * @return TRUE if the drop was successful and the move is valid; FALSE
 * otherwise.
 */
gboolean onDrop (GtkDropTarget *target, const GValue *value, double x,
                 double y, gpointer user_data);

/**
 * @brief Called when a drag operation concludes (successfully or canceled).
 * @param source The drag source object.
 * @param drag The drag context.
 * @param delete_data Whether the source data should be deleted.
 * @param user_data Pointer to the GuiContext.
 */
void onDragEnd (GtkDragSource *source, GdkDrag *drag, gboolean delete_data,
                gpointer user_data);

/**
 * @brief Handles firing an arrow onto a specific cell.
 * @param button The board cell button clicked to receive the arrow.
 * @param user_data Pointer to the GuiContext.
 */
void fireArrow (GtkWidget *button, gpointer user_data);

/**
 * @brief Handles right-clicks on the board (typically used to cancel a Queen
 * move).
 * @param gesture The gesture click object.
 * @param n_press Number of presses.
 * @param x The X coordinate.
 * @param y The Y coordinate.
 * @param user_data Pointer to the GuiContext.
 */
void onRightClick (GtkGestureClick *gesture, int n_press, double x, double y,
                   gpointer user_data);

/**
 * @brief General click handler for board cells.
 * @param gesture The gesture click object.
 * @param n_press Number of presses.
 * @param x The X coordinate.
 * @param y The Y coordinate.
 * @param user_data Pointer to the GuiContext.
 */
void onCellClicked (GtkGestureClick *gesture, int n_press, double x, double y,
                    gpointer user_data);

/** @} */

/**
 * @name Configuration Update Callbacks
 * @{
 */

void updateConfigVerbose (GtkSwitch *widget, GParamSpec *pspec,
                          gpointer user_data);
void updateConfigBlitz (GtkSwitch *widget, GParamSpec *pspec,
                        gpointer user_data);
void updateConfigTimeout (GtkSpinButton *spin_button, gpointer user_data);
void updateConfigBoardSize (GtkSpinButton *spin_button, gpointer user_data);
void updateConfigAiWhite (GtkSwitch *widget, GParamSpec *pspec,
                          gpointer user_data);
void updateConfigAiBlack (GtkSwitch *widget, GParamSpec *pspec,
                          gpointer user_data);

/* GTK 4.10 introduced new dropdown widgets deprecating GtkComboBoxText */
#if GTK_CHECK_VERSION(4, 10, 0)
void updateConfigLanguage (GObject *object, GParamSpec *pspec,
                           gpointer user_data);
#else
void updateConfigLanguage (GtkComboBoxText *combo_box, gpointer user_data);
#endif

/** @} */

/**
 * @name File I/O Callbacks
 * @{
 */

/**
 * @brief Opens the file dialog to load a saved game.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void loadButtonClick (GtkButton *button, gpointer user_data);

/**
 * @brief Opens the file dialog to save the current game.
 * @param button The button that was clicked.
 * @param user_data Pointer to the GuiContext.
 */
void saveButtonClick (GtkButton *button, gpointer user_data);

/* Handle async file dialog responses depending on GTK version */
#if GTK_CHECK_VERSION(4, 10, 0)
void on_load_file_selected (GObject *source_object, GAsyncResult *res,
                            gpointer user_data);
void on_save_file_selected (GObject *source_object, GAsyncResult *res,
                            gpointer user_data);
#else
void on_load_response (GtkNativeDialog *native, int response,
                       gpointer user_data);
void on_save_response (GtkNativeDialog *native, int response,
                       gpointer user_data);
#endif

/** @} */

/**
 * @name Keyboard Shortcut Callbacks
 * @{
 */

gboolean shortcutNewGame (GtkWidget *widget, GVariant *args,
                          gpointer user_data);
gboolean shortcutInfo (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutConfig (GtkWidget *widget, GVariant *args,
                         gpointer user_data);
gboolean shortcutQuit (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutUndo (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutRedo (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutSaveGame (GtkWidget *widget, GVariant *args,
                           gpointer user_data);
gboolean shortcutLoadGame (GtkWidget *widget, GVariant *args,
                           gpointer user_data);
gboolean shortcutHint (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutDummy (GtkWidget *widget, GVariant *args, gpointer user_data);

/** @} */

/**
 * @name AI Hint Worker Callbacks
 * @{
 */

/**
 * @brief Applies the move calculated by the hint worker thread to the UI.
 * @param user_data Pointer to the AIWorkerData structure.
 * @return G_SOURCE_REMOVE to stop the idle execution.
 */
gboolean guiHintApplyMove (gpointer user_data);

/**
 * @brief Entry point for the background thread that calculates the AI hint.
 * @param user_data Pointer to the AIWorkerData structure.
 * @return NULL upon completion.
 */
gpointer guiHintWorkerThread (gpointer user_data);

/** @} */

#endif // GUI_CALLBACKS_H