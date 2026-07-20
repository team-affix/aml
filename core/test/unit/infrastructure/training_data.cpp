#include <gtest/gtest.h>
#include "infrastructure/training_data.hpp"
#include "infrastructure/lc_expr_pool.hpp"

struct TrainingDataTest : public ::testing::Test {
    lc_expr_pool  lc;
    training_data td;
};

TEST_F(TrainingDataTest, EmptyYieldsNullopt) {
    EXPECT_EQ(td.get_next_data_point(), std::nullopt);
}

TEST_F(TrainingDataTest, PushThenGetInOrder) {
    const lc_expr* a = lc.make_var(0);
    const lc_expr* b = lc.make_var(1);
    const lc_expr* c = lc.make_var(2);
    const lc_expr* d = lc.make_var(3);
    td.push_data_point({a, b});
    td.push_data_point({c, d});

    auto first = td.get_next_data_point();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->input, a);
    EXPECT_EQ(first->label, b);

    auto second = td.get_next_data_point();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->input, c);
    EXPECT_EQ(second->label, d);

    EXPECT_EQ(td.get_next_data_point(), std::nullopt);
}

TEST_F(TrainingDataTest, ExhaustedStaysExhausted) {
    td.push_data_point({lc.make_var(0), lc.make_var(1)});
    ASSERT_TRUE(td.get_next_data_point().has_value());
    EXPECT_EQ(td.get_next_data_point(), std::nullopt);
    EXPECT_EQ(td.get_next_data_point(), std::nullopt);
}
