#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/feature_synthesizer.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockClearCandidates {
    MOCK_METHOD(void, clear, ());
};

struct MockTryInsertCandidate {
    MOCK_METHOD(void, try_insert, (const candidate&));
};

struct MockGetCandidates {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockFindVariableCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&, var_id), (const));
};

struct MockFindConstantCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&), (const));
};

struct MockGetPrimitiveCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockGetPrimitiveSignature {
    MOCK_METHOD(primitive_signature, signature, (prim_id), (const));
};

struct MockEnumerateArgumentTuples {
    MOCK_METHOD(
        std::vector<std::vector<size_t>>,
        enumerate,
        (const std::vector<type_id>&, const std::vector<candidate>&, size_t, size_t),
        (const));
};

struct MockFindAppCandidate {
    MOCK_METHOD(std::optional<candidate>, find, (prim_id, type_id, const std::vector<const candidate*>&));
};

using synthesizer_t = feature_synthesizer<
    NiceMock<MockClearCandidates>,
    NiceMock<MockTryInsertCandidate>,
    NiceMock<MockGetCandidates>,
    NiceMock<MockFindVariableCandidates>,
    NiceMock<MockFindConstantCandidates>,
    NiceMock<MockGetPrimitiveCount>,
    NiceMock<MockGetPrimitiveSignature>,
    NiceMock<MockEnumerateArgumentTuples>,
    NiceMock<MockFindAppCandidate>>;

// One boolean input x0 over two rows, the constant true, and one unary boolean primitive of result type 4.
struct FeatureSynthesizerTest : public ::testing::Test {
    FeatureSynthesizerTest();
    synthesizer_t make(size_t max_depth);
    value falsity;
    value truth;
    feature variable;
    feature true_constant;
    feature application;
    bin data;
    candidate variable_candidate;
    candidate constant_candidate;
    candidate application_candidate;
    std::vector<candidate> found;
    primitive_signature signature;
    NiceMock<MockClearCandidates> clear_candidates;
    NiceMock<MockTryInsertCandidate> try_insert_candidate;
    NiceMock<MockGetCandidates> get_candidates;
    NiceMock<MockFindVariableCandidates> find_variable_candidates;
    NiceMock<MockFindConstantCandidates> find_constant_candidates;
    NiceMock<MockGetPrimitiveCount> get_primitive_count;
    NiceMock<MockGetPrimitiveSignature> get_primitive_signature;
    NiceMock<MockEnumerateArgumentTuples> enumerate_argument_tuples;
    NiceMock<MockFindAppCandidate> find_app_candidate;

    void SetUp() override {
        ON_CALL(get_candidates, candidates()).WillByDefault(ReturnRef(found));
        ON_CALL(find_variable_candidates, find(data, 0))
            .WillByDefault(Return(std::vector<candidate>{variable_candidate}));
        ON_CALL(find_constant_candidates, find(data))
            .WillByDefault(Return(std::vector<candidate>{constant_candidate}));
        ON_CALL(get_primitive_count, count()).WillByDefault(Return(1));
        ON_CALL(get_primitive_signature, signature(0)).WillByDefault(Return(signature));
        ON_CALL(enumerate_argument_tuples, enumerate(_, _, _, _))
            .WillByDefault(Return(std::vector<std::vector<size_t>>{{1}}));
        ON_CALL(find_app_candidate, find(0, 4, _)).WillByDefault(Return(application_candidate));
    }
};

FeatureSynthesizerTest::FeatureSynthesizerTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , variable{feature::var{.index = 0}}
    , true_constant{feature::constant{.literal = &truth}}
    , application{feature::app{.prim = 0, .args = {&variable}}}
    , data{.scope = {typed_column{.type = 0, .cells = {&falsity, &truth}}}, .labels = {&falsity, &truth}}
    , variable_candidate{.expression = &variable, .cost = 1, .observed = data.scope[0]}
    , constant_candidate{
          .expression = &true_constant, .cost = 1, .observed = typed_column{.type = 0, .cells = {&truth, &truth}}}
    , application_candidate{
          .expression = &application, .cost = 2, .observed = typed_column{.type = 4, .cells = {&truth, &falsity}}}
    , found{variable_candidate}
    , signature{.arg_types = {0}, .result_type = 4} {}

synthesizer_t FeatureSynthesizerTest::make(size_t max_depth) {
    return synthesizer_t{
        clear_candidates, try_insert_candidate, get_candidates, find_variable_candidates, find_constant_candidates,
        get_primitive_count, get_primitive_signature, enumerate_argument_tuples, find_app_candidate, max_depth};
}

}

TEST_F(FeatureSynthesizerTest, TheIndexIsClearedThenSeededWithTheHeterogeneousVariables) {
    synthesizer_t synthesizer = make(0);
    InSequence order;
    EXPECT_CALL(clear_candidates, clear());
    EXPECT_CALL(try_insert_candidate, try_insert(variable_candidate));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, AZeroDepthAppliesNoPrimitive) {
    synthesizer_t synthesizer = make(0);
    EXPECT_CALL(find_app_candidate, find(_, _, _)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, ConstantsPrecedeTheFoundFeaturesAndOnlyTheLastLayerIsTheFrontier) {
    synthesizer_t synthesizer = make(1);
    EXPECT_CALL(enumerate_argument_tuples,
                enumerate(signature.arg_types, (std::vector<candidate>{constant_candidate, variable_candidate}), 1, 2));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, EachTuplesOperandsAreOfferedWithThePrimitiveAndItsResultType) {
    synthesizer_t synthesizer = make(1);
    EXPECT_CALL(find_app_candidate, find(0, 4, ElementsAre(Pointee(variable_candidate))))
        .WillOnce(Return(application_candidate));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, AFoundApplicationIsInserted) {
    synthesizer_t synthesizer = make(1);
    EXPECT_CALL(try_insert_candidate, try_insert(variable_candidate));
    EXPECT_CALL(try_insert_candidate, try_insert(application_candidate));
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, AnApplicationThatIsNotFoundIsNotInserted) {
    synthesizer_t synthesizer = make(1);
    ON_CALL(find_app_candidate, find(_, _, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(try_insert_candidate, try_insert(_)).Times(AnyNumber());
    EXPECT_CALL(try_insert_candidate, try_insert(application_candidate)).Times(0);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, SynthesisStopsOnceALayerFindsNothingNew) {
    synthesizer_t synthesizer = make(3);
    EXPECT_CALL(enumerate_argument_tuples, enumerate(_, _, _, _)).Times(1);
    synthesizer.synthesize(data);
}

TEST_F(FeatureSynthesizerTest, TheIndexsCandidatesAreReturned) {
    synthesizer_t synthesizer = make(1);
    found = {variable_candidate, application_candidate};
    EXPECT_EQ(synthesizer.synthesize(data), found);
}
