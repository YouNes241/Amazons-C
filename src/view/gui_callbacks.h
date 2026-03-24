#ifndef GUI_CALLBACKS_H
#define GUI_CALLBACKS_H

#include "gui_utils.h" // Contient la définition de AppContext
#include <gtk/gtk.h>

void navigationClick (GtkButton *button, gpointer user_data);
void quitButtonClick (GtkButton *button, gpointer user_data);
void infoPopupButtonClick (GtkButton *button, gpointer user_data);
void configPopupButtonClick (GtkButton *button, gpointer user_data);
void undoButtonClick (GtkButton *button, gpointer user_data);
void redoButtonClick (GtkButton *button, gpointer user_data);

void newGameButtonClick (GtkButton *button, gpointer user_data);

GdkContentProvider *onDrag (GtkDragSource *source, double x, double y,
                            gpointer user_data);
gboolean onDrop (GtkDropTarget *target, const GValue *value, double x,
                 double y, gpointer user_data);
void onDragEnd (GtkDragSource *source, GdkDrag *drag, gboolean delete_data,
                gpointer user_data);

void fireArrow (GtkWidget *button, gpointer user_data);
void onRightClick (GtkGestureClick *gesture, int n_press, double x, double y,
                   gpointer user_data);
void onCellClicked (GtkGestureClick *gesture, int n_press, double x, double y,
                    gpointer user_data);

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
#if GTK_CHECK_VERSION(4, 10, 0)
void updateConfigLanguage (GObject *object, GParamSpec *pspec,
                           gpointer user_data);
#else
void updateConfigLanguage (GtkComboBoxText *combo_box, gpointer user_data);
#endif
gboolean shortcutNewGame (GtkWidget *widget, GVariant *args,
                          gpointer user_data);
gboolean shortcutInfo (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutConfig (GtkWidget *widget, GVariant *args,
                         gpointer user_data);
gboolean shortcutQuit (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutUndo (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutRedo (GtkWidget *widget, GVariant *args, gpointer user_data);
gboolean shortcutDummy (GtkWidget *widget, GVariant *args, gpointer user_data);

#endif // GUI_CALLBACKS_H