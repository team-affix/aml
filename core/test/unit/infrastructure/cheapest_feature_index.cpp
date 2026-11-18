#include <gtest/gtest.h>
#include "infrastructure/cheapest_feature_index.hpp"

namespace {

struct CheapestFeatureIndexTest : public ::testing::Test {
    CheapestFeatureIndexTest();
    candidate make(const feature* expression, size_t cost, const column& cells);
    value falsity;
    value truth;
    feature first;
    feature second;
    cheapest_feature_index index;
};

CheapestFeatureIndexTest::CheapestFeatureIndexTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , first{feature::var{.index = 0}}
    , second{feature::var{.index = 1}}
    , index() {}

candidate CheapestFeatureIndexTest::make(const feature* expression, size_t cost, const column& cells) {
    return candidate{.expression = expression, .cost = cost, .observed = typed_column{.type = 0, .cells = cells}};
}

}

TEST_F(CheapestFeatureIndexTest, CandidatesWithDifferentColumnsAreAllKeptInOrder) {
    index.try_insert(make(&first, 1, {&falsity, &truth}));
    index.try_insert(make(&second, 1, {&truth, &falsity}));
    EXPECT_EQ(index.candidates(),
              (std::vector<candidate>{make(&first, 1, {&falsity, &truth}), make(&second, 1, {&truth, &falsity})}));
}

TEST_F(CheapestFeatureIndexTest, ANoCheaperCandidateForAHeldColumnIsDropped) {
    index.try_insert(make(&first, 2, {&falsity, &truth}));
    index.try_insert(make(&second, 2, {&falsity, &truth}));
    EXPECT_EQ(index.candidates(), (std::vector<candidate>{make(&first, 2, {&falsity, &truth})}));
}

TEST_F(CheapestFeatureIndexTest, ACheaperCandidateForAHeldColumnReplacesItInPlace) {
    index.try_insert(make(&first, 3, {&falsity, &truth}));
    index.try_insert(make(&first, 3, {&truth, &truth}));
    index.try_insert(make(&second, 1, {&falsity, &truth}));
    EXPECT_EQ(index.candidates(),
              (std::vector<candidate>{make(&second, 1, {&falsity, &truth}), make(&first, 3, {&truth, &truth})}));
}

TEST_F(CheapestFeatureIndexTest, ClearForgetsEveryCandidate) {
    index.try_insert(make(&first, 3, {&falsity, &truth}));
    index.clear();
    index.try_insert(make(&second, 5, {&falsity, &truth}));
    EXPECT_EQ(index.candidates(), (std::vector<candidate>{make(&second, 5, {&falsity, &truth})}));
}
