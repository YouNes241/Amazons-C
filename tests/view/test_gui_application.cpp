/**
 * @file test_gui_application.cpp
 * @brief Unit tests for the main GUI application lifecycle.
 *
 * @details This test suite verifies the behavior of the `gui_application.c`
 * module. It tests the creation, initialization, and basic lifecycle of the
 * `GtkApplication`, ensuring that the context is properly attached and that
 * window initialization does not cause crashes in a headless environment.
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

/**
 * @class GuiApplicationTest
 * @brief Test fixture for the GTK Application tests.
 *
 * @details Manages the setup and teardown of the GTK environment,
 * simulating a headless execution to prevent CI pipeline failures.
 * It also handles working directory adjustments so that GTK resources
 * can be loaded correctly during tests.
 */
class GuiApplicationTest : public ::testing::Test
{
protected:
  GtkApplication *app;   /**< Pointer to the GTK application instance. */
  game_config_t *config; /**< Pointer to the game configuration. */
  static char original_dir[PATH_MAX]; /**< Buffer to store the original working
                                         directory. */

  /**
   * @brief Sets up the test suite before any tests run.
   * * @details Initializes GTK in a safe, headless mode by disabling
   * accessibility and overriding the DBUS session (preventing hangs in CI
   * environments). It also navigates up one directory so that relative paths
   * for UI resources remain valid.
   */
  static void
  SetUpTestSuite ()
  {
    // Prevent GTK from attempting to connect to a non-existent DBus session in
    // CI
    g_setenv ("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null", TRUE);
    // Disable accessibility features to prevent warnings/crashes in headless
    // mode
    g_setenv ("GTK_A11Y", "none", TRUE);
    // Custom flag to notify the application that it is running in a test
    // context
    g_setenv ("RUNNING_IN_TESTS", "1", TRUE);

    // Save the current directory and move up to properly locate GTK resources
    if (getcwd (original_dir, sizeof (original_dir)) != nullptr)
      {
        int ret = chdir ("..");
        (void)ret; // Suppress unused variable warning
      }

    gtk_init ();
  }

  /**
   * @brief Cleans up the test suite after all tests have finished.
   * Restores the original working directory.
   */
  static void
  TearDownTestSuite ()
  {
    int ret = chdir (original_dir);
    (void)ret; // Suppress unused variable warning
  }

  /**
   * @brief Prepares the environment before each test.
   * Allocates the configuration, creates the application, and registers it.
   */
  void
  SetUp () override
  {
    config = (game_config_t *)calloc (1, sizeof (game_config_t));
    init_default_config (config);

    app = createApplication (config);
    ASSERT_NE (app, nullptr);

    // Set as non-unique so multiple tests can instantiate the app without DBus
    // conflicts
    g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);

    GError *error = nullptr;
    g_application_register (G_APPLICATION (app), nullptr, &error);
    ASSERT_EQ (error, nullptr)
        << "App registration failed: " << (error ? error->message : "");
  }

  /**
   * @brief Cleans up the environment after each test.
   * Frees the application and configuration memory.
   */
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

// Define the static member variable
char GuiApplicationTest::original_dir[PATH_MAX];

/* ==========================================================================
 * APPLICATION LIFECYCLE TESTS
 * ==========================================================================
 */

/**
 * @brief Verifies that the GTK Application is created successfully.
 * Checks for proper object casting and ensures the application ID is assigned.
 */
TEST_F (GuiApplicationTest, ApplicationCreationIsSuccessful)
{
  EXPECT_TRUE (G_IS_APPLICATION (app));
  EXPECT_TRUE (GTK_IS_APPLICATION (app));

  const gchar *app_id = g_application_get_application_id (G_APPLICATION (app));
  EXPECT_NE (app_id, nullptr);
}

/**
 * @brief Verifies that the internal GUI context is properly attached to the
 * application. The context is typically stored using `g_object_set_data`.
 */
TEST_F (GuiApplicationTest, GuiContextIsAttachedToApplication)
{
  GuiContext *ctx
      = (GuiContext *)g_object_get_data (G_OBJECT (app), "GuiContext");
  ASSERT_NE (ctx, nullptr) << "GuiContext was not attached to GtkApplication";

  EXPECT_EQ (ctx->gameConfig, config);
}

/**
 * @brief Verifies that activating the application and initializing the window
 * does not crash.
 * * @details This test forces the GTK main loop to process pending events to
 * ensure all widgets are allocated and rendered in memory safely, then cleanly
 * destroys them.
 */
TEST_F (GuiApplicationTest, InitWindowDoesNotCrash)
{
  // Trigger the application "activate" signal to build the UI
  g_application_activate (G_APPLICATION (app));

  GtkWindow *window = gtk_application_get_active_window (app);
  ASSERT_NE (window, nullptr) << "Window should be initialized and active";

  // Spin the GTK main loop to process all pending UI rendering events
  while (g_main_context_pending (nullptr))
    {
      g_main_context_iteration (nullptr, FALSE);
    }

  // Cleanly destroy the window
  if (window != nullptr)
    {
      gtk_window_destroy (window);
    }

  // Spin the loop one more time to process the destruction events
  while (g_main_context_pending (nullptr))
    {
      g_main_context_iteration (nullptr, FALSE);
    }

  SUCCEED ();
}