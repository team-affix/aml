#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/set_cost_measurer.hpp"
#include "value_objects/constant_id.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockSizeAncestry {
    MOCK_METHOD(size_t, size, (const std::vector<feature_id>&, const std::vector<constant_id>&), (const));
};

struct MockCountDistinctRows {
    MOCK_METHOD(size_t, count, (const std::vector<feature_id>&));
};

struct SetCostMeasurerTest : public ::testing::Test {
    NiceMock<MockSizeAncestry> size_ancestry;
    NiceMock<MockCountDistinctRows> count_distinct_rows;
    set_cost_measurer<NiceMock<MockSizeAncestry>, NiceMock<MockCountDistinctRows>> measurer{
        size_ancestry, count_distinct_rows};
};

}

TEST_F(SetCostMeasurerTest, TheCostIsTheAncestrySizeWithNoExtraConstantsPlusTheDistinctRows) {
    ON_CALL(size_ancestry, size(std::vector<feature_id>{2, 5}, std::vector<constant_id>{})).WillByDefault(Return(4));
    ON_CALL(count_distinct_rows, count(std::vector<feature_id>{2, 5})).WillByDefault(Return(7));
    EXPECT_EQ(measurer.measure({2, 5}), 11u);
}

TEST_F(SetCostMeasurerTest, TheEmptySetCostsItsOneDistinctRow) {
    ON_CALL(count_distinct_rows, count(std::vector<feature_id>{})).WillByDefault(Return(1));
    EXPECT_EQ(measurer.measure({}), 1u);
}
