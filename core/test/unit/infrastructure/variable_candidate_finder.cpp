#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/variable_candidate_finder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature&), (const));
};

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column_view&), (const));
};

struct VariableCandidateFinderTest : public ::testing::Test {
    VariableCandidateFinderTest();
    value falsity;
    value truth;
    feature first;
    feature third;
    bin_view data;
    NiceMock<MockSizeFeature> size_feature;
    NiceMock<MockFindUniformValue> find_uniform_value;
    variable_candidate_finder<NiceMock<MockSizeFeature>, NiceMock<MockFindUniformValue>> finder{
        size_feature, find_uniform_value};

    void SetUp() override {
        ON_CALL(size_feature, size(_)).WillByDefault(Return(1));
        ON_CALL(find_uniform_value, find(column_view::of(typed_column{.type = 0, .cells = {falsity, truth}})))
            .WillByDefault(Return(std::nullopt));
        ON_CALL(find_uniform_value, find(column_view::of(typed_column{.type = 3, .cells = {truth, falsity}})))
            .WillByDefault(Return(std::nullopt));
        ON_CALL(find_uniform_value, find(column_view::of(typed_column{.type = 0, .cells = {truth, truth}})))
            .WillByDefault(Return(&truth));
    }
};

VariableCandidateFinderTest::VariableCandidateFinderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , first{feature::var{.index = 0}}
    , third{feature::var{.index = 2}}
    , data(bin_view::of(bin{
          .inputs = {
              typed_column{.type = 0, .cells = {falsity, truth}},
              typed_column{.type = 0, .cells = {truth, truth}},
              typed_column{.type = 3, .cells = {truth, falsity}}},
          .labels = typed_column{.type = 0, .cells = {falsity, truth}}}))
    , size_feature()
    , find_uniform_value() {}

}

TEST_F(VariableCandidateFinderTest, HeterogeneousVariablesBecomeCandidatesAndHomogeneousOnesAreSkipped) {
    EXPECT_EQ(finder.find(data, 0),
              (std::vector<candidate>{
                  candidate{.expression = first, .cost = 1, .observed = data.inputs[0]},
                  candidate{.expression = third, .cost = 1, .observed = data.inputs[2]}}));
}

TEST_F(VariableCandidateFinderTest, VariablesBeforeTheFirstAreSkipped) {
    EXPECT_EQ(finder.find(data, 1),
              (std::vector<candidate>{candidate{.expression = third, .cost = 1, .observed = data.inputs[2]}}));
}

TEST_F(VariableCandidateFinderTest, ACandidateCostsItsFeaturesSize) {
    EXPECT_CALL(size_feature, size(third)).WillOnce(Return(7));
    EXPECT_EQ(finder.find(data, 2).front().cost, 7u);
}

TEST_F(VariableCandidateFinderTest, AFirstVariablePastTheScopeFindsNothing) {
    EXPECT_TRUE(finder.find(data, 3).empty());
}

TEST_F(VariableCandidateFinderTest, ACandidateKeepsItsVariablesType) {
    EXPECT_EQ(finder.find(data, 2).front().observed.type, 3u);
}

TEST_F(VariableCandidateFinderTest, AnEmptyScopeFindsNothing) {
    const bin_view no_variables =
        bin_view::of(bin{.inputs = {}, .labels = typed_column{.type = 0, .cells = {falsity, truth}}});
    EXPECT_TRUE(finder.find(no_variables, 0).empty());
}

TEST_F(VariableCandidateFinderTest, ACandidateSharesItsVariablesColumnInsteadOfCopyingIt) {
    const std::vector<candidate> found = finder.find(data, 0);
    ASSERT_EQ(found.size(), 2u);
    EXPECT_EQ(found[0].observed.cells, data.inputs[0].cells);
    EXPECT_EQ(found[0].observed.rows, data.inputs[0].rows);
    EXPECT_EQ(found[1].observed.cells, data.inputs[2].cells);
}
