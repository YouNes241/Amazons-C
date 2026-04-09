/**
 * @file test_gui_application.cpp
 * @brief Unit tests for gui_application.c
 */

#include <gtest/gtest.h>
#include <gtk/gtk.h>
#include <limits.h>
#include <unistd.h>

extern "C"
{
#include "../../src/view/gui_pages.h"
#include "config.h"
#include "gui_application.h"
#include "gui_utils.h"
}

class GuiApplicationTest : public ::testing::Test
{
protected:
  GtkApplication *app;
  game_config_t *config;
  static char original_dir[PATH_MAX];

  static void
  SetUpTestSuite ()
  {
    g_setenv ("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null", TRUE);
    g_setenv ("GTK_A11Y", "none", TRUE);
    g_setenv ("RUNNING_IN_TESTS", "1", TRUE);

    if (getcwd (original_dir, sizeof (original_dir)) != nullptr)
      {
        int ret = chdir ("..");
        (void)ret;
      }

    gtk_init ();
  }

  static void
  TearDownTestSuite ()
  {
    int ret = chdir (original_dir);
    (void)ret;
  }

  void
  SetUp () override
  {
    config = (game_config_t *)calloc (1, sizeof (game_config_t));
    init_default_config (config);

    app = createApplication (config);
    ASSERT_NE (app, nullptr);

    g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);

    GError *error = nullptr;
    g_application_register (G_APPLICATION (app), nullptr, &error);
    ASSERT_EQ (error, nullptr)
        << "App registration failed: " << (error ? error->message : "");
  }

  void
  TearDown () override
  {
    if (app)
      {
        g_object_unref (app);
        app = nullptr;
      }
    if (config)
      {
        free (config);
        config = nullptr;
      }
  }
};

char GuiApplicationTest::original_dir[PATH_MAX];

/* ==========================================================================
 * TESTS
 * ==========================================================================
 */

TEST_F (GuiApplicationTest, ApplicationCreationIsSuccessful)
{
  EXPECT_TRUE (G_IS_APPLICATION (app));
  EXPECT_TRUE (GTK_IS_APPLICATION (app));

  const gchar *app_id = g_application_get_application_id (G_APPLICATION (app));
  EXPECT_NE (app_id, nullptr);
}

TEST_F (GuiApplicationTest, GuiContextIsAttachedToApplication)
{
  GuiContext *ctx
      = (GuiContext *)g_object_get_data (G_OBJECT (app), "GuiContext");
  ASSERT_NE (ctx, nullptr) << "GuiContext was not attached to GtkApplication";

  EXPECT_EQ (ctx->gameConfig, config);
}

TEST_F (GuiApplicationTest, InitWindowDoesNotCrash)
{
  g_application_activate (G_APPLICATION (app));

  GtkWindow *window = gtk_application_get_active_window (app);
  ASSERT_NE (window, nullptr) << "Window should be initialized and active";

  while (g_main_context_pending (nullptr))
    {
      g_main_context_iteration (nullptr, FALSE);
    }

  if (window != nullptr)
    {
      gtk_window_destroy (window);
    }

  while (g_main_context_pending (nullptr))
    {
      g_main_context_iteration (nullptr, FALSE);
    }

  SUCCEED ();
}