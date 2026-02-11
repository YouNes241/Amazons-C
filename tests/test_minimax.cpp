#include <gtest/gtest.h>

extern "C" {
    #include "../src/ai/minimax/minimax.c" 
}

class MinimaxParamTest : public ::testing::Test {
protected:
    AI_Interface ai;
    
    void SetUp() override {
        create_minimax_ai(&ai);
    }

    void TearDown() override {
        if (ai.context) {
            ai.vtable->clean(&ai);
        }
    }
};

// Testing default values
TEST_F(MinimaxParamTest, UseDefaultValuesWhenNoParams) {
    ConfigMap empty_params = { .list = NULL, .count = 0 };
    
    ai.vtable->init(&ai, &empty_params);

    ASSERT_NE(ai.context, nullptr) << "Init failed.";

    Minimax_Data* data = (Minimax_Data*)ai.context;

    EXPECT_EQ(data->max_depth, 3);
    EXPECT_EQ(data->time_limit, 5);
}

// Testing using parameters
TEST_F(MinimaxParamTest, UpdatesDepthFromParams) {
    Param p[] = { {"depth", "10"}, {"time_limit", "30"} };
    ConfigMap params = { .list = p, .count = 2 };

    ai.vtable->init(&ai, &params);
    Minimax_Data* data = (Minimax_Data*)ai.context;

    EXPECT_EQ(data->max_depth, 10);
    EXPECT_EQ(data->time_limit, 30);
}

// Testing using unused parameters
TEST_F(MinimaxParamTest, UpdatesDepthFromParamsWithUnused) {
    Param p[] = { {"depth", "10"}, {"time_limit", "30"}, {"a_random_parameter", "comic_sans_ms"} };
    ConfigMap params = { .list = p, .count = 2 };

    ai.vtable->init(&ai, &params);
    Minimax_Data* data = (Minimax_Data*)ai.context;

    EXPECT_EQ(data->max_depth, 10);
    EXPECT_EQ(data->time_limit, 30);
}