#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/variable_candidate_finder.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMakeVarFeature {
    MOCK_METHOD(const feature*, make_var, (var_id));
};

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature*), (const));
};

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column&), (const));
};

struct VariableCandidateFinderTest : public ::testing::Test {
    VariableCandidateFinderTest();
    value falsity;
    value truth;
    feature first;
    feature third;
    bin data;
    NiceMock<MockMakeVarFeature> make_var_feature;
    NiceMock<MockSizeFeature> size_feature;
    NiceMock<MockFindUniformValue> find_uniform_value;
    variable_candidate_finder<
        NiceMock<MockMakeVarFeature>, NiceMock<MockSizeFeature>, NiceMock<MockFindUniformValue>>
        finder{make_var_feature, size_feature, find_uniform_value};

    void SetUp() override {
        ON_CALL(make_var_feature, make_var(0)).WillByDefault(Return(&first));
        ON_CALL(make_var_feature, make_var(2)).WillByDefault(Return(&third));
        ON_CALL(size_feature, size(_)).WillByDefault(Return(1));
        ON_CALL(find_uniform_value, find(column{&falsity, &truth})).WillByDefault(Return(std::nullopt));
        ON_CALL(find_uniform_value, find(column{&truth, &falsity})).WillByDefault(Return(std::nullopt));
        ON_CALL(find_uniform_value, find(column{&truth, &truth})).WillByDefault(Return(&truth));
    }
};

VariableCandidateFinderTest::VariableCandidateFinderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , first{feature::var{.index = 0}}
    , third{feature::var{.index = 2}}
    , data{
          .scope = {
              typed_column{.type = 0, .cells = {&falsity, &truth}},
              typed_column{.type = 0, .cells = {&truth, &truth}},
              typed_column{.type = 3, .cells = {&truth, &falsity}}},
          .labels = {&falsity, &truth}}
    , make_var_feature()
    , size_feature()
    , find_uniform_value() {}

}

TEST_F(VariableCandidateFinderTest, HeterogeneousVariablesBecomeCandidatesAndHomogeneousOnesAreSkipped) {
    EXPECT_EQ(finder.find(data, 0),
              (std::vector<candidate>{
                  candidate{.expression = &first, .cost = 1, .observed = data.scope[0]},
                  candidate{.expression = &third, .cost = 1, .observed = data.scope[2]}}));
}

TEST_F(VariableCandidateFinderTest, VariablesBeforeTheFirstAreSkipped) {
    EXPECT_EQ(finder.find(data, 1),
              (std::vector<candidate>{candidate{.expression = &third, .cost = 1, .observed = data.scope[2]}}));
}

TEST_F(VariableCandidateFinderTest, ACandidateCostsItsFeaturesSize) {
    EXPECT_CALL(size_feature, size(&third)).WillOnce(Return(7));
    EXPECT_EQ(finder.find(data, 2).front().cost, 7u);
}

TEST_F(VariableCandidateFinderTest, AFirstVariablePastTheScopeFindsNothing) {
    EXPECT_TRUE(finder.find(data, 3).empty());
}

TEST_F(VariableCandidateFinderTest, ACandidateKeepsItsVariablesType) {
    EXPECT_EQ(finder.find(data, 2).front().observed.type, 3u);
}

TEST_F(VariableCandidateFinderTest, AnEmptyScopeFindsNothing) {
    const bin no_variables{.scope = {}, .labels = {&falsity, &truth}};
    EXPECT_TRUE(finder.find(no_variables, 0).empty());
}
