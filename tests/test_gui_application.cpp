#include <gtest/gtest.h>
#include <gtk/gtk.h>

extern "C"
{
#include "../src/view/gui_application.c"
#include "../src/view/gui_pages.c"
}

class GuiApplicationTest : public ::testing::Test
{
protected:
  GtkApplication *app;

  static void
  SetUpTestSuite ()
  {
    g_setenv ("DBUS_SESSION_BUS_ADDRESS", "unix:path=/dev/null", TRUE);
    g_setenv ("GTK_A11Y", "none", TRUE);
    gtk_init (); // needed to avoid a SegFault
  }

  void
  SetUp () override
  {
    app = createApplication ();

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
  }
};

TEST_F (GuiApplicationTest, ApplicationCreationIsSuccessful)
{
  ASSERT_NE (app, nullptr);
  EXPECT_TRUE (G_IS_APPLICATION (app));
}

TEST_F (GuiApplicationTest, InitWindowDoesNotCrash)
{
  g_signal_emit_by_name (app, "startup");
  initWindow (app, nullptr);

  GtkWindow *window = gtk_application_get_active_window (app);

  if (window != nullptr)
    {
      gtk_window_destroy (window);
    }

  SUCCEED ();
}

// TEST_F (GuiApplicationTest, ChangePageTogglesBetweenPage1AndPage2)
// {
//   GtkWidget *stack = gtk_stack_new ();
//   g_object_ref_sink (stack);

//   GtkWidget *dummyPage1 = gtk_label_new ("1");
//   GtkWidget *dummyPage2 = gtk_label_new ("2");

//   gtk_stack_add_named (GTK_STACK (stack), dummyPage1, "page_1");
//   gtk_stack_add_named (GTK_STACK (stack), dummyPage2, "page_2");

//   gtk_stack_set_visible_child_name (GTK_STACK (stack),
//                                     "page_1"); // set to page_1

//   ASSERT_STREQ (gtk_stack_get_visible_child_name (GTK_STACK (stack)),
//                 "page_1");

//   changePage (nullptr, stack);

//   EXPECT_STREQ (gtk_stack_get_visible_child_name (GTK_STACK (stack)),
//                 "page_2"); // test if it is on page 2

//   changePage (nullptr, stack);

//   EXPECT_STREQ (gtk_stack_get_visible_child_name (GTK_STACK (stack)),
//                 "page_1"); // test if it came back to page 1

//   g_object_unref (stack);
// }