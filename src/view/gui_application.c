/**
 * @file gui_application.c
 * @brief Implementation of the main GTK application lifecycle.
 *
 * @details This file handles the bootstrapping of the GTK4 application. It is
 * responsible for loading binary resources (CSS, UI templates), initializing
 * the main graphical context, configuring the root window, and assembling the
 * primary navigation stack containing all the screens/pages of the game.
 */

#include "gui_application.h"
#include "config.h"
#include "gui_pages.h"
#include "gui_utils.h"
#include <assert.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

/* Forward declaration for the GTK activate signal callback */
static void initWindow (GtkApplication *app, gpointer userData);

/**
 * @brief Creates and configures the main GTK application instance.
 *
 * @details This function performs critical startup operations:
 * 1. Loads the compiled GTK resources (`resources.gresource`) into memory.
 * 2. Initializes the global `GuiContext` using the provided game
 * configuration.
 * 3. Instantiates the `GtkApplication` and binds the context to it.
 * 4. Connects the `activate` signal to trigger the window creation.
 *
 * @param config Pointer to the game configuration structure.
 * @return A pointer to the newly created GtkApplication instance.
 * * @note If the `resources.gresource` file is missing or corrupted, the
 * application will print an error to standard error and exit immediately.
 */
GtkApplication *
createApplication (game_config_t *config)
{
  assert (config != NULL);

  GError *error = NULL;

  // Load the compiled binary UI resources (XML layouts, CSS, images)
  GResource *resource = g_resource_load ("resources.gresource", &error);

  if (error != NULL)
    {
      g_printerr ("Error while loading GUI resources: %s\n", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }
  assert (resource != NULL);

  g_resources_register (resource);
  g_resource_unref (resource);

  // Initialize the main GUI state/context wrapper
  GuiContext *ctx = initGuiContext (config);
  assert (ctx != NULL);

  // Create the GTK Application (Non-unique allows multiple instances for
  // testing)
  GtkApplication *app
      = gtk_application_new ("fr.bordeaux.amazons", G_APPLICATION_NON_UNIQUE);
  assert (app != NULL);

  // Bind the initialization callback to the application's activation phase
  g_signal_connect (app, "activate", G_CALLBACK (initWindow), ctx);

  // Attach the context to the application so it is automatically freed upon
  // destruction
  g_object_set_data_full (G_OBJECT (app), "GuiContext", ctx,
                          (GDestroyNotify)freeGuiContext);

  return app;
}

/**
 * @brief Runs the main event loop of the GTK application.
 *
 * @details Blocks the calling thread and transfers control to the GTK main
 * loop. The loop continues running until the application window is closed.
 *
 * @param app Pointer to the GtkApplication instance to run.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return The exit status code of the application.
 */
int
runApplication (GtkApplication *app, int argc, char **argv)
{
  assert (app != NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app); // Clean up the application object

  return status;
}

/**
 * @brief Initializes the main application window and its view stack.
 *
 * @details This callback is triggered by GTK when the application emits the
 * "activate" signal. It sets up the main window, applies the global CSS
 * stylesheet, creates an overlay for notifications, and initializes every
 * individual page (main menu, game board, lobby, config) into a central
 * `GtkStack` for easy navigation.
 *
 * @param app Pointer to the activating GtkApplication.
 * @param userData Pointer to the GuiContext, passed during signal connection.
 */
static void
initWindow (GtkApplication *app, gpointer userData)
{
  assert (app != NULL);

  GuiContext *ctx = (GuiContext *)userData;
  assert (ctx != NULL);

  // Create the main window
  GtkWidget *window = gtk_application_window_new (app);
  assert (window != NULL);

  gtk_window_set_title (GTK_WINDOW (window), "AMAZONS-C");
  gtk_window_set_default_size (GTK_WINDOW (window), 1664, 936);
  gtk_window_fullscreen (GTK_WINDOW (window));

  // Load and apply the global CSS stylesheet
  GtkCssProvider *cssProvider = gtk_css_provider_new ();
  assert (cssProvider != NULL);

  gtk_css_provider_load_from_resource (cssProvider, "/amazons/ui/style.css");
  gtk_style_context_add_provider_for_display (
      gdk_display_get_default (), GTK_STYLE_PROVIDER (cssProvider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Setup the global overlay (used for displaying floating notifications)
  GtkWidget *globalOverlay = gtk_overlay_new ();
  assert (globalOverlay != NULL);
  gtk_window_set_child (GTK_WINDOW (window), globalOverlay);

  // Setup the central navigation stack
  GtkWidget *stack = gtk_stack_new ();
  assert (stack != NULL);

  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 250);
  gtk_overlay_set_child (GTK_OVERLAY (globalOverlay), stack);

  ctx->mainWindow = GTK_WINDOW (window);
  ctx->mainStack = GTK_STACK (stack);

  // Construct the floating notification box
  GtkWidget *notifBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  assert (notifBox != NULL);

  gtk_widget_add_css_class (notifBox, "notif-box");
  gtk_widget_set_valign (notifBox, GTK_ALIGN_END);
  gtk_widget_set_halign (notifBox, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_bottom (notifBox, 20);

  GtkWidget *notifLabel = gtk_label_new ("");
  assert (notifLabel != NULL);

  g_object_set_data (G_OBJECT (stack), "notif_label", notifLabel);
  g_object_set_data (G_OBJECT (stack), "notif_box", notifBox);

  gtk_box_append (GTK_BOX (notifBox), notifLabel);
  gtk_widget_set_halign (notifLabel, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (notifLabel, GTK_ALIGN_CENTER);

  gtk_widget_set_visible (notifBox, FALSE);
  gtk_overlay_add_overlay (GTK_OVERLAY (globalOverlay), notifBox);

  // Initialize and load all individual application pages into the stack
  GtkWidget *mainMenuPage = createMainMenuPage (ctx);
  assert (mainMenuPage != NULL);

  GtkWidget *singleplayerMenuPage = createSingleplayerMenuPage (ctx);
  assert (singleplayerMenuPage != NULL);

  GtkWidget *multiplayerMenuPage = createMultiplayerMenuPage (ctx);
  assert (multiplayerMenuPage != NULL);

  GtkWidget *configPage = createConfigPage (ctx);
  assert (configPage != NULL);

  GtkWidget *infoPage = createInfoPage (ctx);
  assert (infoPage != NULL);

  GtkWidget *gamePage = createGamePage (ctx);
  assert (gamePage != NULL);

  GtkWidget *joinServerPage = createJoinServerPage (ctx);
  assert (joinServerPage != NULL);

  GtkWidget *hostPanelPage = createHostPanelPage (ctx);
  assert (hostPanelPage != NULL);

  GtkWidget *clientPanelPage = createClientPanelPage (ctx);
  assert (clientPanelPage != NULL);

  // Register the pages with specific string identifiers for navigation
  gtk_stack_add_named (GTK_STACK (stack), mainMenuPage, "main_menu_page");
  gtk_stack_add_named (GTK_STACK (stack), singleplayerMenuPage,
                       "singleplayer_page");
  gtk_stack_add_named (GTK_STACK (stack), multiplayerMenuPage,
                       "multiplayer_page");
  gtk_stack_add_named (GTK_STACK (stack), configPage, "config_page");
  gtk_stack_add_named (GTK_STACK (stack), infoPage, "info_page");
  gtk_stack_add_named (GTK_STACK (stack), gamePage, "game_page");
  gtk_stack_add_named (GTK_STACK (stack), joinServerPage, "join_server_page");
  gtk_stack_add_named (GTK_STACK (stack), hostPanelPage, "host_panel_page");
  gtk_stack_add_named (GTK_STACK (stack), clientPanelPage,
                       "client_panel_page");

  // Set the default visible page
  gtk_stack_set_visible_child_name (GTK_STACK (stack), "main_menu_page");

  // Render the window to the screen
  gtk_window_present (GTK_WINDOW (window));
}