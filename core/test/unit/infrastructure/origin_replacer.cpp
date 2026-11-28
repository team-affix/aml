#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/origin_replacer.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockSizeAncestry {
    MOCK_METHOD(size_t, size, (const std::vector<feature_id>&, const std::vector<constant_id>&), (const));
};

struct MockReplaceOrigin {
    MOCK_METHOD(void, replace_origin, (feature_id, const derivation&));
};

// The candidate applies a primitive to feature 1, constant 7 and feature 2; their ancestry has size 3, so the
// candidate costs 4 with itself.
struct OriginReplacerTest : public ::testing::Test {
    OriginReplacerTest();
    derivation candidate;
    NiceMock<MockSizeAncestry> size_ancestry;
    NiceMock<MockReplaceOrigin> replace_origin;
    origin_replacer<NiceMock<MockSizeAncestry>, NiceMock<MockReplaceOrigin>> replacer{size_ancestry, replace_origin};

    void SetUp() override {
        ON_CALL(size_ancestry, size(std::vector<feature_id>{1, 2}, std::vector<constant_id>{7}))
            .WillByDefault(Return(3));
    }
};

OriginReplacerTest::OriginReplacerTest()
    : candidate{derivation::application{
          .prim = 0,
          .args = {operand{operand::from_feature{.id = 1}}, operand{operand::from_constant{.id = 7}},
                   operand{operand::from_feature{.id = 2}}}}}
    , size_ancestry()
    , replace_origin() {}

}

TEST_F(OriginReplacerTest, ACheaperApplicationReplacesTheOrigin) {
    ON_CALL(size_ancestry, size(std::vector<feature_id>{9}, std::vector<constant_id>{})).WillByDefault(Return(5));
    EXPECT_CALL(replace_origin, replace_origin(9, candidate));
    replacer.try_replace(9, candidate);
}

TEST_F(OriginReplacerTest, AnApplicationCostingTheSameDoesNotReplaceIt) {
    ON_CALL(size_ancestry, size(std::vector<feature_id>{9}, std::vector<constant_id>{})).WillByDefault(Return(4));
    EXPECT_CALL(replace_origin, replace_origin(_, _)).Times(0);
    replacer.try_replace(9, candidate);
}

TEST_F(OriginReplacerTest, ACostlierApplicationDoesNotReplaceIt) {
    ON_CALL(size_ancestry, size(std::vector<feature_id>{9}, std::vector<constant_id>{})).WillByDefault(Return(1));
    EXPECT_CALL(replace_origin, replace_origin(_, _)).Times(0);
    replacer.try_replace(9, candidate);
}
