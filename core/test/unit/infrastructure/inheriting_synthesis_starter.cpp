#include <cstddef>
#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/inheriting_synthesis_starter.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockStartSession {
    MOCK_METHOD(void, start, (const bin_view&));
};

struct MockGetInheritedFeatures {
    MOCK_METHOD(const std::vector<feature>&, inherited, (const std::vector<size_t>*), (const));
};

struct MockEvaluateFeatureColumn {
    MOCK_METHOD(column_view, evaluate, (const feature&, const bin_view&), (const));
};

struct MockFindUniformValue {
    MOCK_METHOD(std::optional<const value*>, find, (const column_view&), (const));
};

struct MockSizeFeature {
    MOCK_METHOD(size_t, size, (const feature&), (const));
};

struct MockTryInsertFeatureCandidate {
    MOCK_METHOD(void, try_insert, (candidate));
};

struct InheritingSynthesisStarterTest : public ::testing::Test {
    InheritingSynthesisStarterTest();
    value falsity;
    value truth;
    feature sum;
    feature product;
    bin_view data;
    column_view sum_column;
    column_view product_column;
    std::vector<feature> inherited;
    NiceMock<MockStartSession> start_session;
    NiceMock<MockGetInheritedFeatures> get_inherited_features;
    NiceMock<MockEvaluateFeatureColumn> evaluate_feature_column;
    NiceMock<MockFindUniformValue> find_uniform_value;
    NiceMock<MockSizeFeature> size_feature;
    NiceMock<MockTryInsertFeatureCandidate> try_insert_feature_candidate;
    inheriting_synthesis_starter<
        NiceMock<MockStartSession>, NiceMock<MockGetInheritedFeatures>, NiceMock<MockEvaluateFeatureColumn>,
        NiceMock<MockFindUniformValue>, NiceMock<MockSizeFeature>, NiceMock<MockTryInsertFeatureCandidate>>
        starter{start_session, get_inherited_features, evaluate_feature_column, find_uniform_value, size_feature,
                try_insert_feature_candidate};

    void SetUp() override {
        ON_CALL(get_inherited_features, inherited(_)).WillByDefault(ReturnRef(inherited));
        ON_CALL(evaluate_feature_column, evaluate(sum, _)).WillByDefault(Return(sum_column));
        ON_CALL(evaluate_feature_column, evaluate(product, _)).WillByDefault(Return(product_column));
        ON_CALL(find_uniform_value, find(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(size_feature, size(sum)).WillByDefault(Return(3));
        ON_CALL(size_feature, size(product)).WillByDefault(Return(5));
    }
};

InheritingSynthesisStarterTest::InheritingSynthesisStarterTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , sum{feature::app{.prim = 0, .args = {feature{feature::var{.index = 0}}}}}
    , product{feature::app{.prim = 1, .args = {feature{feature::var{.index = 0}}}}}
    , data(bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {falsity, truth}}},
                            .labels = typed_column{.type = 0, .cells = {truth, falsity}}}))
    , sum_column(column_view::of(typed_column{.type = 0, .cells = {truth, falsity}}))
    , product_column(column_view::of(typed_column{.type = 0, .cells = {falsity, truth}}))
    , inherited{sum, product}
    , start_session()
    , get_inherited_features()
    , evaluate_feature_column()
    , find_uniform_value()
    , size_feature()
    , try_insert_feature_candidate() {}

}

TEST_F(InheritingSynthesisStarterTest, TheWrappedStarterStartsTheSessionFirst) {
    InSequence order;
    EXPECT_CALL(start_session, start(data));
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_)).Times(2);
    starter.start(data);
}

TEST_F(InheritingSynthesisStarterTest, InheritedFeaturesAreLookedUpByTheBinsRows) {
    EXPECT_CALL(get_inherited_features, inherited(_)).Times(AnyNumber());
    EXPECT_CALL(get_inherited_features, inherited(data.labels.rows.get())).WillOnce(ReturnRef(inherited));
    starter.start(data);
}

TEST_F(InheritingSynthesisStarterTest, EachInheritedFeatureIsInsertedInOrderWithItsColumnAndSize) {
    InSequence order;
    EXPECT_CALL(try_insert_feature_candidate,
                try_insert(candidate{.expression = sum, .cost = 3, .observed = sum_column}));
    EXPECT_CALL(try_insert_feature_candidate,
                try_insert(candidate{.expression = product, .cost = 5, .observed = product_column}));
    starter.start(data);
}

TEST_F(InheritingSynthesisStarterTest, EachInheritedFeatureIsEvaluatedOverTheBin) {
    EXPECT_CALL(evaluate_feature_column, evaluate(_, _)).Times(AnyNumber());
    EXPECT_CALL(evaluate_feature_column, evaluate(sum, data)).WillOnce(Return(sum_column));
    EXPECT_CALL(evaluate_feature_column, evaluate(product, data)).WillOnce(Return(product_column));
    starter.start(data);
}

TEST_F(InheritingSynthesisStarterTest, AnInheritedFeatureUniformOverTheBinIsLeftOut) {
    ON_CALL(find_uniform_value, find(sum_column)).WillByDefault(Return(std::optional<const value*>(&truth)));
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_)).Times(AnyNumber());
    EXPECT_CALL(try_insert_feature_candidate,
                try_insert(candidate{.expression = sum, .cost = 3, .observed = sum_column}))
        .Times(0);
    EXPECT_CALL(try_insert_feature_candidate,
                try_insert(candidate{.expression = product, .cost = 5, .observed = product_column}));
    starter.start(data);
}

TEST_F(InheritingSynthesisStarterTest, ABinInheritingNothingOnlyStartsTheSession) {
    inherited.clear();
    EXPECT_CALL(start_session, start(data));
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_)).Times(0);
    starter.start(data);
}
