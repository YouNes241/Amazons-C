#include <gtest/gtest.h>

extern "C"
{
#include "ai_factory.h"
#include "minimax.h"
#include "random.h"
}

class MinimaxParamTest : public ::testing::Test
{
protected:
  AI_Interface ai;

  void
  SetUp () override
  {
    create_minimax_ai (&ai);
  }

  void
  TearDown () override
  {
    if (ai.context && ai.vtable)
      ai.vtable->clean (&ai);
  }
};

// Testing default values
TEST_F (MinimaxParamTest, UseDefaultValuesWhenNoParams)
{
  ConfigMap empty_params = { 0, NULL };

  ai.vtable->init (&ai, &empty_params);

  ASSERT_NE (ai.context, nullptr) << "Init failed.";

  Minimax_Data *data = (Minimax_Data *)ai.context;

  EXPECT_EQ (data->max_depth, 3);
  EXPECT_EQ (data->time_limit, 5000); // ms
}

// Testing using parameters
TEST_F (MinimaxParamTest, UpdatesDepthFromParams)
{
  Param p[] = { { "depth", "10" }, { "time_limit", "30" } };
  ConfigMap params = { 2, p };

  ai.vtable->init (&ai, &params);
  Minimax_Data *data = (Minimax_Data *)ai.context;

  EXPECT_EQ (data->max_depth, 10);
  EXPECT_EQ (data->time_limit, 30);
}

// Testing using unused parameters
TEST_F (MinimaxParamTest, UpdatesDepthFromParamsWithUnused)
{
  Param p[] = { { "depth", "10" },
                { "time_limit", "30" },
                { "a_random_parameter", "comic_sans_ms" } };
  ConfigMap params = { 2, p };

  ai.vtable->init (&ai, &params);
  Minimax_Data *data = (Minimax_Data *)ai.context;

  EXPECT_EQ (data->max_depth, 10);
  EXPECT_EQ (data->time_limit, 30);
}

// Testing creating a random AI using the factory
TEST (AIFactoryTest, CreateRandomReturnsCorrectImplementation)
{
  ConfigMap empty_params = { 0, NULL };
  AI_Interface ai = create_ai ("random", empty_params);

  ASSERT_NE (ai.vtable, nullptr);
  ASSERT_EQ (ai.context, nullptr);

  EXPECT_EQ (ai.vtable, &random_vtable)
      << "La Factory n'a pas renvoyé la bonne implémentation pour 'random'";

  ai.vtable->clean (&ai);
}

// Testing creating a minimax AI using the factory
TEST (AIFactoryTest, CreateMinimaxReturnsCorrectImplementation)
{
  ConfigMap empty_params = { 0, NULL };
  AI_Interface ai = create_ai ("minimax", empty_params);

  ASSERT_NE (ai.vtable, nullptr);
  ASSERT_NE (ai.context, nullptr);

  EXPECT_EQ (ai.vtable, &minimax_vtable)
      << "La Factory n'a pas renvoyé la bonne implémentation pour 'minimax'";

  ai.vtable->clean (&ai);
}

// Testing creating an unknown AI using the factory
TEST (AIFactoryTest, CreateUnknownReturnsNull)
{
  ConfigMap empty_params = { 0, NULL };

  // On demande un truc qui n'existe pas
  AI_Interface ai = create_ai ("skynet", empty_params);

  // On s'attend à ce que la vtable soit NULL (selon ton implémentation de
  // factory)
  EXPECT_EQ (ai.vtable, nullptr);
  EXPECT_EQ (ai.context, nullptr);
}