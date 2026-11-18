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

struct MockGetPrimitiveSignature {
    MOCK_METHOD(primitive_signature, signature, (prim_id), (const));
};

struct MockChooseArguments {
    MOCK_METHOD(std::vector<size_t>, choose, (prim_id, const std::vector<type_id>&));
};

struct MockRecordApplication {
    MOCK_METHOD(void, record, (const prim_application&));
};

struct MockGetFeatureCandidates {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockFindAppCandidate {
    MOCK_METHOD(std::optional<candidate>, find, (prim_id, type_id, const std::vector<size_t>&, size_t));
};

struct MockTryInsertFeatureCandidate {
    MOCK_METHOD(void, try_insert, (candidate));
};

using synthesizer_t = feature_synthesizer<
    NiceMock<MockStartSession>,
    NiceMock<MockCountApplications>,
    NiceMock<MockFindPrimitiveOptions>,
    NiceMock<MockChooseStep>,
    NiceMock<MockGetPrimitiveSignature>,
    NiceMock<MockChooseArguments>,
    NiceMock<MockRecordApplication>,
    NiceMock<MockGetFeatureCandidates>,
    NiceMock<MockFindAppCandidate>,
    NiceMock<MockTryInsertFeatureCandidate>>;

// One feature; primitives 2 and 5 have witnesses. Primitive 5 is binary with result type 4. By
// default nothing has been applied yet, the step chooses primitive 5 (option 2) once and then stops, and the
// arguments are operands 0 and 1.
struct FeatureSynthesizerTest : public ::testing::Test {
    FeatureSynthesizerTest();
    synthesizer_t make(size_t max_applications);
    value falsity;
    value truth;
    feature variable;
    feature true_constant;
    feature application;
    bin data;
    candidate variable_candidate;
    candidate application_candidate;
    std::vector<candidate> features;
    primitive_signature signature;
    NiceMock<MockStartSession> start_session;
    NiceMock<MockCountApplications> count_applications;
    NiceMock<MockFindPrimitiveOptions> find_primitive_options;
    NiceMock<MockChooseStep> choose_step;
    NiceMock<MockGetPrimitiveSignature> get_primitive_signature;
    NiceMock<MockChooseArguments> choose_arguments;
    NiceMock<MockRecordApplication> record_application;
    NiceMock<MockGetFeatureCandidates> get_feature_candidates;
    NiceMock<MockFindAppCandidate> find_app_candidate;
    NiceMock<MockTryInsertFeatureCandidate> try_insert_feature_candidate;

    void SetUp() override {
        ON_CALL(count_applications, count()).WillByDefault(Return(0));
        ON_CALL(find_primitive_options, find()).WillByDefault(Return(std::vector<prim_id>{2, 5}));
        EXPECT_CALL(choose_step, choose(_)).Times(AnyNumber()).WillOnce(Return(2)).WillRepeatedly(Return(0));
        ON_CALL(get_primitive_signature, signature(5)).WillByDefault(Return(signature));
        ON_CALL(choose_arguments, choose(5, signature.arg_types)).WillByDefault(Return(std::vector<size_t>{0, 1}));
        ON_CALL(get_feature_candidates, candidates()).WillByDefault(ReturnRef(features));
        ON_CALL(find_app_candidate, find(5, 4, _, _)).WillByDefault(Return(application_candidate));
    }
};

FeatureSynthesizerTest::FeatureSynthesizerTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , variable{feature::var{.index = 0}}
    , true_constant{feature::constant{.literal = &truth}}
    , application{feature::app{.prim = 5, .args = {&true_constant, &variable}}}
    , data{.scope = {typed_column{.type = 0, .cells = {&falsity, &truth}}}, .labels = {&falsity, &truth}}
    , variable_candidate{.expression = &variable, .cost = 1, .observed = data.scope[0]}
    , application_candidate{
          .expression = &application, .cost = 3, .observed = typed_column{.type = 4, .cells = {&truth, &falsity}}}
    , features{variable_candidate}
    , signature{.arg_types = {0, 0}, .result_type = 4} {}

synthesizer_t FeatureSynthesizerTest::make(size_t max_applications) {
    return synthesizer_t{
        start_session, count_applications, find_primitive_options, choose_step, get_primitive_signature,
        choose_arguments, record_application, get_feature_candidates, find_app_candidate, try_insert_feature_candidate,
        max_applications};
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

TEST_F(FeatureSynthesizerTest, ChoosingStopFirstAppliesNothing) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(choose_step, choose(_)).WillOnce(Return(0));
    EXPECT_CALL(record_application, record(_)).Times(0);
    EXPECT_CALL(find_app_candidate, find(_, _, _, _)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheChosenPrimitivesArgumentsAreChosenForItsSignatureAndLogged) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(choose_arguments, choose(5, signature.arg_types)).WillOnce(Return(std::vector<size_t>{0, 1}));
    EXPECT_CALL(record_application, record(prim_application{.prim = 5, .args = {0, 1}}));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheChosenArgumentsAreAppliedOverTheBinsRows) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(find_app_candidate, find(5, 4, std::vector<size_t>{0, 1}, 2)).WillOnce(Return(application_candidate));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, AFoundApplicationJoinsTheFeatures) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(try_insert_feature_candidate, try_insert(application_candidate));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, AnApplicationThatIsNotFoundDoesNotJoinTheFeatures) {
    synthesizer_t synthesizer = make(3);
    ON_CALL(find_app_candidate, find(_, _, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(try_insert_feature_candidate, try_insert(_)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheSessionStopsAtTheCapWithoutAskingTheStep) {
    synthesizer_t synthesizer = make(2);
    EXPECT_CALL(choose_step, choose(_)).Times(2).WillRepeatedly(Return(1));
    ON_CALL(get_primitive_signature, signature(2)).WillByDefault(Return(signature));
    ON_CALL(choose_arguments, choose(2, _)).WillByDefault(Return(std::vector<size_t>{1, 1}));
    EXPECT_CALL(count_applications, count()).WillOnce(Return(0)).WillOnce(Return(1)).WillRepeatedly(Return(2));
    EXPECT_CALL(record_application, record(_)).Times(2);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, ACapOfZeroAppliesNothing) {
    synthesizer_t synthesizer = make(0);
    EXPECT_CALL(find_primitive_options, find()).Times(0);
    EXPECT_CALL(record_application, record(_)).Times(0);
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
