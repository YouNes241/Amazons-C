#include <gtest/gtest.h>
#include <gtk/gtk.h>
#include <unistd.h>

extern "C"
{
#include "../../src/view/gui_pages.h"
#include "gui_application.h"
#include "gui_utils.h"
}

class GuiApplicationTest : public ::testing::Test
{
protected:
  GtkApplication *app;
  game_config_t *config;

  static void
  SetUpTestSuite ()
  {
    g_setenv ("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null", TRUE);
    g_setenv ("GTK_A11Y", "none", TRUE);
    g_setenv ("RUNNING_IN_TESTS", "1", TRUE);

    ASSERT_EQ (chdir (".."), 0) << "Failed to change directory to ..";

    gtk_init ();
  }

  void
  SetUp () override
  {
    config = (game_config_t *)malloc (sizeof (game_config_t));
    init_default_config (config);
    app = createApplication (config);

    g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
    g_application_register (G_APPLICATION (app), nullptr, nullptr);
  }

  void
  TearDown () override
  {
    if (app)
      {
        g_object_unref (app);
      }
    if (config)
      {
        free (config);
      }
  }
};

TEST_F (GuiApplicationTest, ApplicationCreationIsSuccessful)
{
  ASSERT_NE (app, nullptr);
  EXPECT_TRUE (G_IS_APPLICATION (app));
}

TEST_F (GuiApplicationTest, InitWindowDoesNotCrash)
{
  g_application_activate (G_APPLICATION (app));

  GtkWindow *window = gtk_application_get_active_window (app);

  ASSERT_NE (window, nullptr) << "Window should be initialized";

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