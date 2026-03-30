#include "gui_application.h"
#include "config.h"
#include "gui_pages.h"
#include "gui_utils.h"
#include <gio/gio.h>
#include <gtk/gtk.h>

static void initWindow (GtkApplication *app, gpointer userData);

GtkApplication *
createApplication (game_config_t *config)
{
  GError *error = NULL;
  GResource *resource = g_resource_load ("resources.gresource", &error);

  if (error != NULL)
    {
      g_printerr ("Error while loading GUI resources : %s\n", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }

  g_resources_register (resource);
  g_resource_unref (resource);

  GuiContext *ctx = initGuiContext (config);
  GtkApplication *app
      = gtk_application_new ("fr.bordeaux.amazons", G_APPLICATION_NON_UNIQUE);

  g_signal_connect (app, "activate", G_CALLBACK (initWindow), ctx);
  g_object_set_data_full (G_OBJECT (app), "GuiContext", ctx,
                          (GDestroyNotify)freeGuiContext);

  return app;
}

int
runApplication (GtkApplication *app, int argc, char **argv)
{
  int status;

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

static void
initWindow (GtkApplication *app, gpointer userData)
{
  GuiContext *ctx = (GuiContext *)userData;

  GtkCssProvider *cssProvider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (cssProvider, "/amazons/ui/style.css");

  gtk_style_context_add_provider_for_display (
      gdk_display_get_default (), GTK_STYLE_PROVIDER (cssProvider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_object_unref (cssProvider);

  GtkWidget *window;
  GtkWidget *globalOverlay;
  GtkWidget *stack;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "PLACEHOLDER_GAME_NAME");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
  gtk_window_fullscreen (GTK_WINDOW (window));

  globalOverlay = gtk_overlay_new ();
  stack = gtk_stack_new ();

  ctx->mainWindow = GTK_WINDOW (window);
  ctx->mainStack = GTK_STACK (stack);

  // Page transition animation (WIP)
  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 300);

  gtk_overlay_set_child (GTK_OVERLAY (globalOverlay), stack);

  GtkWidget *notifBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  GtkWidget *notifLabel = gtk_label_new ("");

  gtk_box_append (GTK_BOX (notifBox), notifLabel);

  gtk_widget_set_valign (notifBox, GTK_ALIGN_START);
  gtk_widget_set_halign (notifBox, GTK_ALIGN_CENTER);

  gtk_widget_set_visible (notifBox, FALSE);
  gtk_overlay_add_overlay (GTK_OVERLAY (globalOverlay), notifBox);

  GtkWidget *mainMenuPage = createMainMenuPage (ctx);
  GtkWidget *singleplayerMenuPage = createSingleplayerMenuPage (ctx);
  GtkWidget *multiplayerMenuPage = createMultiplayerMenuPage (ctx);
  GtkWidget *configPage = createConfigPage (ctx);
  GtkWidget *infoPage = createInfoPage (ctx);
  GtkWidget *gamePage = createGamePage (ctx);
  GtkWidget *joinServerPage = createJoinServerPage (ctx);
  GtkWidget *clientPanelPage = createClientPanelPage (ctx);

  gtk_stack_add_named (GTK_STACK (stack), mainMenuPage, "main_menu_page");
  gtk_stack_add_named (GTK_STACK (stack), singleplayerMenuPage,
                       "singleplayer_page");
  gtk_stack_add_named (GTK_STACK (stack), multiplayerMenuPage,
                       "multiplayer_page");
  gtk_stack_add_named (GTK_STACK (stack), configPage, "config_page");
  gtk_stack_add_named (GTK_STACK (stack), infoPage, "info_page");
  gtk_stack_add_named (GTK_STACK (stack), gamePage, "game_page");
  gtk_stack_add_named (GTK_STACK (stack), joinServerPage, "join_server_page");
  gtk_stack_add_named (GTK_STACK (stack), clientPanelPage,
                       "client_panel_page");

  gtk_window_set_child (GTK_WINDOW (window), globalOverlay);
  gtk_window_present (GTK_WINDOW (window));
}
