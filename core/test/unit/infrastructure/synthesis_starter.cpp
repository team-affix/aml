#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/synthesis_starter.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockClearConstantCandidates {
    MOCK_METHOD(void, clear, ());
};

struct MockClearFeatureCandidates {
    MOCK_METHOD(void, clear, ());
};

struct MockClearApplications {
    MOCK_METHOD(void, clear, ());
};

struct MockFindConstantCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&), (const));
};

struct MockTryInsertConstantCandidate {
    MOCK_METHOD(void, try_insert, (const candidate&));
};

struct MockFindVariableCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&, var_id), (const));
};

struct MockTryInsertFeatureCandidate {
    MOCK_METHOD(void, try_insert, (const candidate&));
};

struct SynthesisStarterTest : public ::testing::Test {
    SynthesisStarterTest();
    value truth;
    feature variable;
    feature constant;
    bin data;
    candidate variable_candidate;
    candidate constant_candidate;
    NiceMock<MockClearConstantCandidates> clear_constant_candidates;
    NiceMock<MockClearFeatureCandidates> clear_feature_candidates;
    NiceMock<MockClearApplications> clear_applications;
    NiceMock<MockFindConstantCandidates> find_constant_candidates;
    NiceMock<MockTryInsertConstantCandidate> try_insert_constant_candidate;
    NiceMock<MockFindVariableCandidates> find_variable_candidates;
    NiceMock<MockTryInsertFeatureCandidate> try_insert_feature_candidate;
    synthesis_starter<
        NiceMock<MockClearConstantCandidates>, NiceMock<MockClearFeatureCandidates>, NiceMock<MockClearApplications>,
        NiceMock<MockFindConstantCandidates>, NiceMock<MockTryInsertConstantCandidate>,
        NiceMock<MockFindVariableCandidates>, NiceMock<MockTryInsertFeatureCandidate>>
        starter{clear_constant_candidates, clear_feature_candidates, clear_applications, find_constant_candidates,
                try_insert_constant_candidate, find_variable_candidates, try_insert_feature_candidate};

    void SetUp() override {
        ON_CALL(find_constant_candidates, find(data))
            .WillByDefault(Return(std::vector<candidate>{constant_candidate}));
        ON_CALL(find_variable_candidates, find(data, 0))
            .WillByDefault(Return(std::vector<candidate>{variable_candidate}));
    }
};

SynthesisStarterTest::SynthesisStarterTest()
    : truth{.ctor = 1, .fields = {}}
    , variable{feature::var{.index = 0}}
    , constant{feature::constant{.literal = &truth}}
    , data{.scope = {typed_column{.type = 0, .cells = {&truth}}}, .labels = {&truth}}
    , variable_candidate{.expression = &variable, .cost = 1, .observed = data.scope[0]}
    , constant_candidate{.expression = &constant, .cost = 1, .observed = typed_column{.type = 0, .cells = {&truth}}}
    , clear_constant_candidates()
    , clear_feature_candidates()
    , clear_applications()
    , find_constant_candidates()
    , try_insert_constant_candidate()
    , find_variable_candidates()
    , try_insert_feature_candidate() {}

}

TEST_F(SynthesisStarterTest, TheLastSessionIsForgottenBeforeTheNewOneIsSeeded) {
    InSequence order;
    EXPECT_CALL(clear_constant_candidates, clear());
    EXPECT_CALL(clear_feature_candidates, clear());
    EXPECT_CALL(clear_applications, clear());
    EXPECT_CALL(try_insert_constant_candidate, try_insert(_));
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_));
    starter.start(data);
}

TEST_F(SynthesisStarterTest, TheBinsConstantsBecomeConstantCandidates) {
    EXPECT_CALL(try_insert_constant_candidate, try_insert(constant_candidate));
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_)).Times(AnyNumber());
    EXPECT_CALL(try_insert_feature_candidate, try_insert(constant_candidate)).Times(0);
    starter.start(data);
}

TEST_F(SynthesisStarterTest, TheBinsHeterogeneousVariablesBecomeTheFirstFeatures) {
    EXPECT_CALL(try_insert_feature_candidate, try_insert(variable_candidate));
    EXPECT_CALL(try_insert_constant_candidate, try_insert(_)).Times(AnyNumber());
    EXPECT_CALL(try_insert_constant_candidate, try_insert(variable_candidate)).Times(0);
    starter.start(data);
}
