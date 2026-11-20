#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/synthesis_starter.hpp"

using namespace AML;

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockClearFeatureCandidates {
    MOCK_METHOD(void, clear, ());
};

struct MockClearApplications {
    MOCK_METHOD(void, clear, ());
};

struct MockFindVariableCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&, var_id), (const));
};

struct MockTryInsertFeatureCandidate {
    MOCK_METHOD(void, try_insert, (candidate));
};

struct SynthesisStarterTest : public ::testing::Test {
    SynthesisStarterTest();
    value truth;
    feature variable;
    bin data;
    candidate variable_candidate;
    NiceMock<MockClearFeatureCandidates> clear_feature_candidates;
    NiceMock<MockClearApplications> clear_applications;
    NiceMock<MockFindVariableCandidates> find_variable_candidates;
    NiceMock<MockTryInsertFeatureCandidate> try_insert_feature_candidate;
    synthesis_starter<
        NiceMock<MockClearFeatureCandidates>, NiceMock<MockClearApplications>, NiceMock<MockFindVariableCandidates>,
        NiceMock<MockTryInsertFeatureCandidate>>
        starter{clear_feature_candidates, clear_applications, find_variable_candidates, try_insert_feature_candidate};

    void SetUp() override {
        ON_CALL(find_variable_candidates, find(data, 0))
            .WillByDefault(Return(std::vector<candidate>{variable_candidate}));
    }
};

SynthesisStarterTest::SynthesisStarterTest()
    : truth{.tag = 1, .args = {}}
    , variable{feature::var{.index = 0}}
    , data{.inputs = {typed_column{.type = 0, .cells = {&truth}}}, .labels = typed_column{.type = 0, .cells = {&truth}}}
    , variable_candidate{.expression = variable, .cost = 1, .observed = data.inputs[0]}
    , clear_feature_candidates()
    , clear_applications()
    , find_variable_candidates()
    , try_insert_feature_candidate() {}

}

TEST_F(SynthesisStarterTest, TheLastSessionIsForgottenBeforeTheNewOneIsSeeded) {
    InSequence order;
    EXPECT_CALL(clear_feature_candidates, clear());
    EXPECT_CALL(clear_applications, clear());
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_));
    starter.start(data);
}

TEST_F(SynthesisStarterTest, TheBinsHeterogeneousVariablesBecomeTheFirstFeatures) {
    EXPECT_CALL(try_insert_feature_candidate, try_insert(variable_candidate));
    starter.start(data);
}

TEST_F(SynthesisStarterTest, VariablesAreSoughtFromTheFirstVariable) {
    EXPECT_CALL(find_variable_candidates, find(data, 0)).WillOnce(Return(std::vector<candidate>{}));
    starter.start(data);
}

TEST_F(SynthesisStarterTest, EveryHeterogeneousVariableIsInsertedInOrder) {
    const candidate second{.expression = variable, .cost = 2, .observed = data.inputs[0]};
    ON_CALL(find_variable_candidates, find(data, 0))
        .WillByDefault(Return(std::vector<candidate>{variable_candidate, second}));
    InSequence order;
    EXPECT_CALL(try_insert_feature_candidate, try_insert(variable_candidate));
    EXPECT_CALL(try_insert_feature_candidate, try_insert(second));
    starter.start(data);
}

TEST_F(SynthesisStarterTest, NoHeterogeneousVariablesStillClearsTheSession) {
    ON_CALL(find_variable_candidates, find(data, 0)).WillByDefault(Return(std::vector<candidate>{}));
    EXPECT_CALL(clear_feature_candidates, clear());
    EXPECT_CALL(clear_applications, clear());
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_)).Times(0);
    starter.start(data);
}
