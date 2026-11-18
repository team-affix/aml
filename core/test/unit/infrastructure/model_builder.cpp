#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_builder.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockDetectLeaf {
    MOCK_METHOD(std::optional<const value*>, detect, (const bin&), (const));
};

struct MockSynthesizeCandidates {
    MOCK_METHOD(std::vector<candidate>, synthesize, (const bin&));
};

struct MockChooseFeature {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockSplitBin {
    MOCK_METHOD(std::vector<bin>, split, (const bin&, const typed_column&), (const));
};

struct MockFindVariableCandidates {
    MOCK_METHOD(std::vector<candidate>, find, (const bin&, var_id), (const));
};

struct MockChooseField {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockMakeEmpty {
    MOCK_METHOD(const model_node*, make_empty, ());
};

struct MockMakeLeaf {
    MOCK_METHOD(const model_node*, make_leaf, (const value*));
};

struct MockMakeMatch {
    MOCK_METHOD(const model_node*, make_match, (const feature*, const std::vector<const model_node*>&));
};

// The root bin has one boolean input over two rows with disagreeing labels.
struct ModelBuilderTest : public ::testing::Test {
    ModelBuilderTest();
    value falsity;
    value truth;
    feature first_feature;
    feature second_feature;
    feature first_field;
    feature second_field;
    model_node empty_node;
    model_node false_leaf;
    model_node true_leaf;
    model_node inner_match;
    model_node outer_match;
    bin root;
    bin false_child;
    bin true_child;
    bin no_rows_child;
    bin funnel_child;
    candidate first_candidate;
    candidate second_candidate;
    candidate first_field_candidate;
    candidate second_field_candidate;
    NiceMock<MockDetectLeaf> detect_leaf;
    NiceMock<MockSynthesizeCandidates> synthesize_candidates;
    NiceMock<MockChooseFeature> choose_feature;
    NiceMock<MockSplitBin> split_bin;
    NiceMock<MockFindVariableCandidates> find_variable_candidates;
    NiceMock<MockChooseField> choose_field;
    NiceMock<MockMakeEmpty> make_empty;
    NiceMock<MockMakeLeaf> make_leaf;
    NiceMock<MockMakeMatch> make_match;
    model_builder<
        NiceMock<MockDetectLeaf>, NiceMock<MockSynthesizeCandidates>, NiceMock<MockChooseFeature>,
        NiceMock<MockSplitBin>, NiceMock<MockFindVariableCandidates>, NiceMock<MockChooseField>,
        NiceMock<MockMakeEmpty>, NiceMock<MockMakeLeaf>, NiceMock<MockMakeMatch>>
        builder{detect_leaf, synthesize_candidates, choose_feature, split_bin, find_variable_candidates,
                choose_field, make_empty, make_leaf, make_match};

    void SetUp() override {
        ON_CALL(detect_leaf, detect(root)).WillByDefault(Return(std::nullopt));
        ON_CALL(detect_leaf, detect(funnel_child)).WillByDefault(Return(std::nullopt));
        ON_CALL(detect_leaf, detect(false_child)).WillByDefault(Return(&falsity));
        ON_CALL(detect_leaf, detect(true_child)).WillByDefault(Return(&truth));
        ON_CALL(synthesize_candidates, synthesize(root))
            .WillByDefault(Return(std::vector<candidate>{first_candidate, second_candidate}));
        ON_CALL(make_empty, make_empty()).WillByDefault(Return(&empty_node));
        ON_CALL(make_leaf, make_leaf(&falsity)).WillByDefault(Return(&false_leaf));
        ON_CALL(make_leaf, make_leaf(&truth)).WillByDefault(Return(&true_leaf));
    }
};

ModelBuilderTest::ModelBuilderTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , first_feature{feature::var{.index = 0}}
    , second_feature{feature::app{.prim = 0, .args = {&first_feature}}}
    , first_field{feature::var{.index = 1}}
    , second_field{feature::var{.index = 2}}
    , empty_node{model_node::empty{}}
    , false_leaf{model_node::leaf{.label = &falsity}}
    , true_leaf{model_node::leaf{.label = &truth}}
    , inner_match{model_node::match{.scrutinee = &second_field, .branches = {}}}
    , outer_match{model_node::match{.scrutinee = &first_feature, .branches = {}}}
    , root{.scope = {typed_column{.type = 0, .cells = {&falsity, &truth}}}, .labels = {&falsity, &truth}}
    , false_child{.scope = {typed_column{.type = 0, .cells = {&falsity}}}, .labels = {&falsity}}
    , true_child{.scope = {typed_column{.type = 0, .cells = {&truth}}}, .labels = {&truth}}
    , no_rows_child{.scope = {typed_column{.type = 0, .cells = {}}}, .labels = {}}
    , funnel_child{
          .scope = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                    typed_column{.type = 0, .cells = {&truth, &truth}},
                    typed_column{.type = 0, .cells = {&truth, &falsity}}},
          .labels = {&falsity, &truth}}
    , first_candidate{.expression = &first_feature, .cost = 1, .observed = root.scope[0]}
    , second_candidate{
          .expression = &second_feature, .cost = 2, .observed = typed_column{.type = 0, .cells = {&truth, &falsity}}}
    , first_field_candidate{.expression = &first_field, .cost = 1, .observed = funnel_child.scope[1]}
    , second_field_candidate{.expression = &second_field, .cost = 1, .observed = funnel_child.scope[2]} {}

}

TEST_F(ModelBuilderTest, ABinWithoutRowsBecomesEmpty) {
    EXPECT_EQ(builder.build(no_rows_child), &empty_node);
}

TEST_F(ModelBuilderTest, ABinWhoseLabelsAgreeBecomesALeaf) {
    EXPECT_EQ(builder.build(true_child), &true_leaf);
}

TEST_F(ModelBuilderTest, AHeterogeneousBinMatchesOnTheChosenFeatureAndBuildsEachChild) {
    EXPECT_CALL(choose_feature, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(split_bin, split(root, second_candidate.observed))
        .WillOnce(Return(std::vector<bin>{true_child, false_child}));
    EXPECT_CALL(make_match, make_match(&second_feature, std::vector<const model_node*>{&true_leaf, &false_leaf}))
        .WillOnce(Return(&outer_match));
    EXPECT_EQ(builder.build(root), &outer_match);
}

TEST_F(ModelBuilderTest, AMatchWhoseRowsShareOneConstructorFunnelsIntoAChosenBoundField) {
    ON_CALL(choose_feature, choose(_)).WillByDefault(Return(0));
    EXPECT_CALL(split_bin, split(root, first_candidate.observed))
        .WillOnce(Return(std::vector<bin>{no_rows_child, funnel_child}));
    EXPECT_CALL(find_variable_candidates, find(funnel_child, 1))
        .WillOnce(Return(std::vector<candidate>{first_field_candidate, second_field_candidate}));
    EXPECT_CALL(choose_field, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(split_bin, split(funnel_child, second_field_candidate.observed))
        .WillOnce(Return(std::vector<bin>{false_child, true_child}));
    EXPECT_CALL(make_match, make_match(&second_field, std::vector<const model_node*>{&false_leaf, &true_leaf}))
        .WillOnce(Return(&inner_match));
    EXPECT_CALL(make_match, make_match(&first_feature, std::vector<const model_node*>{&empty_node, &inner_match}))
        .WillOnce(Return(&outer_match));
    EXPECT_EQ(builder.build(root), &outer_match);
}

TEST_F(ModelBuilderTest, AFunnelledBinIsNotSynthesizedAgain) {
    ON_CALL(choose_feature, choose(_)).WillByDefault(Return(0));
    ON_CALL(split_bin, split(root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{no_rows_child, funnel_child}));
    ON_CALL(find_variable_candidates, find(funnel_child, 1))
        .WillByDefault(Return(std::vector<candidate>{first_field_candidate}));
    ON_CALL(split_bin, split(funnel_child, first_field_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{false_child, true_child}));
    EXPECT_CALL(synthesize_candidates, synthesize(_)).Times(AnyNumber());
    EXPECT_CALL(synthesize_candidates, synthesize(funnel_child)).Times(0);
    builder.build(root);
}
