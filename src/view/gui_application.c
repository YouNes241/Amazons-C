#include "gui_application.h"
#include "config.h"
#include "gui_pages.h"
#include "gui_utils.h"
#include <assert.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

static void initWindow (GtkApplication *app, gpointer userData);

GtkApplication *
createApplication (game_config_t *config)
{
  assert (config != NULL);

  GError *error = NULL;
  GResource *resource = g_resource_load ("resources.gresource", &error);

  if (error != NULL)
    {
      g_printerr ("Error while loading GUI resources : %s\n", error->message);
      g_error_free (error);
      exit (EXIT_FAILURE);
    }
  assert (resource != NULL);

  g_resources_register (resource);
  g_resource_unref (resource);

  GuiContext *ctx = initGuiContext (config);
  assert (ctx != NULL);

  GtkApplication *app
      = gtk_application_new ("fr.bordeaux.amazons", G_APPLICATION_NON_UNIQUE);
  assert (app != NULL);

  g_signal_connect (app, "activate", G_CALLBACK (initWindow), ctx);
  g_object_set_data_full (G_OBJECT (app), "GuiContext", ctx,
                          (GDestroyNotify)freeGuiContext);

  return app;
}

int
runApplication (GtkApplication *app, int argc, char **argv)
{
  assert (app != NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

static void
initWindow (GtkApplication *app, gpointer userData)
{
  assert (app != NULL);

  GuiContext *ctx = (GuiContext *)userData;
  assert (ctx != NULL);

  GtkWidget *window = gtk_application_window_new (app);
  assert (window != NULL);

  gtk_window_set_title (GTK_WINDOW (window), "AMAZONS-C");
  gtk_window_set_default_size (GTK_WINDOW (window), 1664, 936);
  gtk_window_fullscreen (GTK_WINDOW (window));

  GtkCssProvider *cssProvider = gtk_css_provider_new ();
  assert (cssProvider != NULL);

  gtk_css_provider_load_from_resource (cssProvider, "/amazons/ui/style.css");
  gtk_style_context_add_provider_for_display (
      gdk_display_get_default (), GTK_STYLE_PROVIDER (cssProvider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  GtkWidget *globalOverlay = gtk_overlay_new ();
  assert (globalOverlay != NULL);

  gtk_window_set_child (GTK_WINDOW (window), globalOverlay);

  GtkWidget *stack = gtk_stack_new ();
  assert (stack != NULL);

  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration (GTK_STACK (stack), 250);
  gtk_overlay_set_child (GTK_OVERLAY (globalOverlay), stack);

  ctx->mainWindow = GTK_WINDOW (window);
  ctx->mainStack = GTK_STACK (stack);

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

  gtk_stack_set_visible_child_name (GTK_STACK (stack), "main_menu_page");

  gtk_window_present (GTK_WINDOW (window));
}
