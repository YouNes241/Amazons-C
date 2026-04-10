/**
 * @file gui_application.h
 * @brief Declarations for the main GTK application lifecycle.
 *
 * @details This header provides the entry points for creating and running
 * the GTK4 application. It encapsulates the application instance creation,
 * signal binding, and the execution of the main event loop.
 */

#ifndef GUI_APPLICATION_H
#define GUI_APPLICATION_H

#include "config.h"
#include <gtk/gtk.h>

/**
 * @brief Creates and configures the main GTK application instance.
 *
 * @details Initializes a new `GtkApplication`, setting up necessary internal
 * state, connecting core lifecycle signals (such as 'activate' to build the
 * UI), and linking the provided game configuration to the application context.
 *
 * @param config Pointer to the game configuration structure (`game_config_t`).
 * @return A pointer to the newly created `GtkApplication` instance.
 */
GtkApplication *createApplication (game_config_t *config);

/**
 * @brief Runs the main event loop of the GTK application.
 *
 * @details This function takes the initialized `GtkApplication` and starts its
 * execution, handing over control to the GTK main loop. It blocks the current
 * thread until the application window is closed or the app is explicitly quit.
 *
 * @param app Pointer to the `GtkApplication` instance to run.
 * @param argc The number of command-line arguments (from `main`).
 * @param argv The array of command-line arguments (from `main`).
 * @return The exit status code of the application (typically 0 on success).
 */
int runApplication (GtkApplication *app, int argc, char **argv);

#endif // GUI_APPLICATION_H