#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_synthesizer.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockStartSession {
    MOCK_METHOD(void, start, (const bin&));
};

struct MockCountApplications {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockFindPrimitiveOptions {
    MOCK_METHOD(std::vector<prim_id>, find, (), (const));
};

struct MockChooseStep {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockApplyChosenPrimitive {
    MOCK_METHOD(void, apply, (prim_id, size_t));
};

struct MockGetFeatures {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockFindOutput {
    MOCK_METHOD(std::optional<const feature*>, find, (const std::vector<candidate>&, const column&), (const));
};

using synthesizer_t = feature_synthesizer<
    NiceMock<MockStartSession>,
    NiceMock<MockCountApplications>,
    NiceMock<MockFindPrimitiveOptions>,
    NiceMock<MockChooseStep>,
    NiceMock<MockApplyChosenPrimitive>,
    NiceMock<MockGetFeatures>,
    NiceMock<MockFindOutput>>;

// A bin of two rows; primitives 2 and 5 have witnesses. By default nothing has been applied yet and the step chooses
// primitive 5 (option 2) once, then stops.
struct FeatureSynthesizerTest : public ::testing::Test {
    FeatureSynthesizerTest();
    synthesizer_t make(size_t max_applications);
    value falsity;
    value truth;
    bin data;
    std::vector<candidate> features;
    feature truth_output;
    NiceMock<MockStartSession> start_session;
    NiceMock<MockCountApplications> count_applications;
    NiceMock<MockFindPrimitiveOptions> find_primitive_options;
    NiceMock<MockChooseStep> choose_step;
    NiceMock<MockApplyChosenPrimitive> apply_chosen_primitive;
    NiceMock<MockGetFeatures> get_features;
    NiceMock<MockFindOutput> find_output;

    void SetUp() override {
        ON_CALL(count_applications, count()).WillByDefault(Return(0));
        ON_CALL(find_primitive_options, find()).WillByDefault(Return(std::vector<prim_id>{2, 5}));
        EXPECT_CALL(choose_step, choose(_)).Times(AnyNumber()).WillOnce(Return(2)).WillRepeatedly(Return(0));
        ON_CALL(get_features, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(find_output, find(_, _)).WillByDefault(Return(std::nullopt));
    }
};

FeatureSynthesizerTest::FeatureSynthesizerTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , data{.scope = {typed_column{.type = 0, .cells = {&falsity, &truth}}}, .labels = {&falsity, &truth}}
    , features()
    , truth_output{feature::constant{.literal = &truth}} {}

synthesizer_t FeatureSynthesizerTest::make(size_t max_applications) {
    return synthesizer_t{
        start_session, count_applications, find_primitive_options, choose_step, apply_chosen_primitive, get_features,
        find_output, max_applications};
}

}

TEST_F(FeatureSynthesizerTest, TheSessionIsStartedOverTheBinFirst) {
    synthesizer_t synthesizer = make(3);
    InSequence order;
    EXPECT_CALL(start_session, start(data));
    EXPECT_CALL(find_primitive_options, find()).Times(AtLeast(1));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheStepChoosesAmongStopAndTheWitnessedPrimitives) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(choose_step, choose(3)).Times(2).WillOnce(Return(2)).WillOnce(Return(0));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheChosenPrimitiveIsAppliedOverTheBinsRows) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(apply_chosen_primitive, apply(5, 2));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, ChoosingStopFirstAppliesNothing) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(choose_step, choose(_)).WillOnce(Return(0));
    EXPECT_CALL(apply_chosen_primitive, apply(_, _)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheSessionStopsAtTheCapWithoutAskingTheStep) {
    synthesizer_t synthesizer = make(2);
    EXPECT_CALL(count_applications, count()).WillOnce(Return(0)).WillOnce(Return(1)).WillRepeatedly(Return(2));
    EXPECT_CALL(choose_step, choose(_)).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(apply_chosen_primitive, apply(2, 2)).Times(2);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, ACapOfZeroAppliesNothing) {
    synthesizer_t synthesizer = make(0);
    EXPECT_CALL(find_primitive_options, find()).Times(0);
    EXPECT_CALL(apply_chosen_primitive, apply(_, _)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheSessionStopsWithoutAskingWhenNoPrimitiveHasAWitness) {
    synthesizer_t synthesizer = make(3);
    ON_CALL(find_primitive_options, find()).WillByDefault(Return(std::vector<prim_id>{}));
    EXPECT_CALL(choose_step, choose(_)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheSessionsOwnFeaturesAreReturned) {
    synthesizer_t synthesizer = make(3);
    EXPECT_EQ(&synthesizer.synthesize(data), &features);
}

TEST_F(FeatureSynthesizerTest, AnOutputAmongTheStartingFeaturesStopsTheSessionAtOnce) {
    synthesizer_t synthesizer = make(3);
    ON_CALL(find_output, find(_, _)).WillByDefault(Return(&truth_output));
    EXPECT_CALL(choose_step, choose(_)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, DerivingAnOutputStopsTheSessionBeforeTheNextStep) {
    synthesizer_t synthesizer = make(5);
    EXPECT_CALL(find_output, find(_, data.labels))
        .WillOnce(Return(std::nullopt))
        .WillRepeatedly(Return(&truth_output));
    EXPECT_CALL(choose_step, choose(_)).WillOnce(Return(1));
    EXPECT_CALL(apply_chosen_primitive, apply(2, 2)).Times(1);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheOutputIsSoughtAmongTheSessionsFeaturesAgainstTheLabels) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(find_output, find(_, _)).Times(AnyNumber());
    EXPECT_CALL(find_output, find(features, data.labels)).Times(AtLeast(1));
    synthesizer.synthesize(data);
}
