#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_builder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockDetectLeaf {
    MOCK_METHOD(std::optional<feature>, detect, (const bin_view&), (const));
};

struct MockSynthesizeFeatures {
    MOCK_METHOD(const std::vector<candidate>&, synthesize, (const bin_view&));
};

struct MockFindOutput {
    MOCK_METHOD(std::optional<feature>, find, (const std::vector<candidate>&, const column_view&), (const));
};

struct MockChooseFeature {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseBoundArgument {
    MOCK_METHOD(candidate, choose, (const bin_view&, var_id));
};

struct MockSplitBin {
    MOCK_METHOD(std::vector<bin_view>, split, (const bin_view&, const column_view&), (const));
};

using builder_t = model_builder<
    NiceMock<MockDetectLeaf>,
    NiceMock<MockSynthesizeFeatures>,
    NiceMock<MockFindOutput>,
    NiceMock<MockChooseFeature>,
    NiceMock<MockChooseBoundArgument>,
    NiceMock<MockSplitBin>>;

// The root bin has one boolean input over two rows with disagreeing labels. Its session offers two features; by
// default no output is found, the first feature is chosen, and the root splits into two leaf children.
struct ModelBuilderTest : public ::testing::Test {
    ModelBuilderTest();
    value falsity;
    value truth;
    feature first_feature;
    feature second_feature;
    feature first_bound_argument;
    feature second_argument;
    feature false_output;
    feature true_output;
    model_node empty_node;
    model_node false_leaf;
    model_node true_leaf;
    model_node output_leaf;
    bin_view root;
    bin_view false_child;
    bin_view true_child;
    bin_view no_rows_child;
    bin_view funnel_child;
    candidate first_candidate;
    candidate second_candidate;
    candidate first_bound_argument_candidate;
    candidate second_argument_candidate;
    std::vector<candidate> features;
    NiceMock<MockDetectLeaf> detect_leaf;
    NiceMock<MockSynthesizeFeatures> synthesize_features;
    NiceMock<MockFindOutput> find_output;
    NiceMock<MockChooseFeature> choose_feature;
    NiceMock<MockChooseBoundArgument> choose_bound_argument;
    NiceMock<MockSplitBin> split_bin;
    builder_t builder{detect_leaf, synthesize_features, find_output, choose_feature, choose_bound_argument, split_bin};

    void SetUp() override {
        ON_CALL(detect_leaf, detect(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(detect_leaf, detect(false_child)).WillByDefault(Return(false_output));
        ON_CALL(detect_leaf, detect(true_child)).WillByDefault(Return(true_output));
        ON_CALL(synthesize_features, synthesize(_)).WillByDefault(ReturnRef(features));
        ON_CALL(find_output, find(_, _)).WillByDefault(Return(std::nullopt));
        ON_CALL(choose_feature, choose(_)).WillByDefault(Return(0));
        ON_CALL(split_bin, split(root, first_candidate.observed))
            .WillByDefault(Return(std::vector<bin_view>{false_child, true_child}));
    }
};

ModelBuilderTest::ModelBuilderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , first_feature{feature::var{.index = 0}}
    , second_feature{feature::app{.prim = 0, .args = {first_feature}}}
    , first_bound_argument{feature::var{.index = 1}}
    , second_argument{feature::var{.index = 2}}
    , false_output{feature::constant{.literal = falsity}}
    , true_output{feature::constant{.literal = truth}}
    , empty_node{model_node::empty{}}
    , false_leaf{model_node::leaf{.output = false_output}}
    , true_leaf{model_node::leaf{.output = true_output}}
    , output_leaf{model_node::leaf{.output = second_feature}}
    , root(bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {falsity, truth}}},
           .labels = typed_column{.type = 0, .cells = {truth, falsity}}}))
    , false_child(bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {falsity}}},
                  .labels = typed_column{.type = 0, .cells = {falsity}}}))
    , true_child(bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {truth}}},
                 .labels = typed_column{.type = 0, .cells = {truth}}}))
    , no_rows_child(bin_view::of(
          bin{.inputs = {typed_column{.type = 0, .cells = {}}}, .labels = typed_column{.type = 0, .cells = {}}}))
    , funnel_child(bin_view::of(bin{
          .inputs = {typed_column{.type = 0, .cells = {falsity, truth}},
                    typed_column{.type = 0, .cells = {truth, truth}},
                    typed_column{.type = 0, .cells = {truth, falsity}}},
          .labels = typed_column{.type = 0, .cells = {truth, falsity}}}))
    , first_candidate{.expression = first_feature, .cost = 1, .observed = root.inputs[0]}
    , second_candidate{
          .expression = second_feature,
          .cost = 2,
          .observed = column_view::of(typed_column{.type = 0, .cells = {truth, falsity}})}
    , first_bound_argument_candidate{.expression = first_bound_argument, .cost = 1, .observed = funnel_child.inputs[1]}
    , second_argument_candidate{.expression = second_argument, .cost = 1, .observed = funnel_child.inputs[2]}
    , features{first_candidate, second_candidate} {}

}

TEST_F(ModelBuilderTest, ABinWithoutRowsBecomesEmptyWithoutDetection) {
    EXPECT_CALL(detect_leaf, detect(_)).Times(0);
    EXPECT_EQ(builder.build(no_rows_child), empty_node);
}

TEST_F(ModelBuilderTest, ADetectedOutputBecomesALeafWithoutSynthesis) {
    EXPECT_CALL(synthesize_features, synthesize(_)).Times(0);
    EXPECT_EQ(builder.build(true_child), true_leaf);
}

TEST_F(ModelBuilderTest, ABinWithNoDetectedOutputIsSynthesized) {
    EXPECT_CALL(synthesize_features, synthesize(_)).Times(AnyNumber());
    EXPECT_CALL(synthesize_features, synthesize(root)).WillOnce(ReturnRef(features));
    builder.build(root);
}

TEST_F(ModelBuilderTest, ASynthesizedOutputBecomesALeafWithoutAMatch) {
    EXPECT_CALL(find_output, find(_, _)).Times(AnyNumber());
    EXPECT_CALL(find_output, find(features, root.labels)).WillOnce(Return(second_feature));
    EXPECT_CALL(choose_feature, choose(_)).Times(0);
    EXPECT_CALL(split_bin, split(_, _)).Times(0);
    EXPECT_EQ(builder.build(root), output_leaf);
}

TEST_F(ModelBuilderTest, OtherwiseTheBinMatchesOnTheChosenFeatureAndBuildsEachChild) {
    EXPECT_CALL(choose_feature, choose(_)).Times(AnyNumber());
    EXPECT_CALL(split_bin, split(_, _)).Times(AnyNumber());
    EXPECT_CALL(choose_feature, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(split_bin, split(root, second_candidate.observed))
        .WillOnce(Return(std::vector<bin_view>{true_child, false_child}));
    EXPECT_EQ(builder.build(root),
              (model_node{model_node::match{.scrutinee = second_feature, .branches = {true_leaf, false_leaf}}}));
}

TEST_F(ModelBuilderTest, TheScrutineeOutlivesTheSessionItCameFrom) {
    EXPECT_CALL(choose_feature, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_feature, choose(2)).WillOnce(Return(1));
    ON_CALL(split_bin, split(root, second_candidate.observed))
        .WillByDefault(Return(std::vector<bin_view>{true_child, false_child}));
    EXPECT_CALL(detect_leaf, detect(_)).Times(AnyNumber());
    EXPECT_CALL(detect_leaf, detect(true_child)).WillOnce(::testing::DoAll(
        ::testing::Assign(&features, std::vector<candidate>{}), Return(true_output)));
    EXPECT_EQ(builder.build(root),
              (model_node{model_node::match{.scrutinee = second_feature, .branches = {true_leaf, false_leaf}}}));
}

TEST_F(ModelBuilderTest, EmptyChildrenBecomeEmptyBranches) {
    ON_CALL(split_bin, split(root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin_view>{false_child, no_rows_child, true_child}));
    EXPECT_EQ(builder.build(root),
              (model_node{model_node::match{
                  .scrutinee = first_feature, .branches = {false_leaf, empty_node, true_leaf}}}));
}

TEST_F(ModelBuilderTest, AMatchWhoseRowsShareOneConstructorFunnelsIntoAChosenBoundArgument) {
    EXPECT_CALL(split_bin, split(_, _)).Times(AnyNumber());
    EXPECT_CALL(choose_bound_argument, choose(_, _)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(split_bin, split(root, first_candidate.observed))
        .WillOnce(Return(std::vector<bin_view>{no_rows_child, funnel_child}));
    EXPECT_CALL(choose_bound_argument, choose(funnel_child, 1)).WillOnce(Return(second_argument_candidate));
    EXPECT_CALL(split_bin, split(funnel_child, second_argument_candidate.observed))
        .WillOnce(Return(std::vector<bin_view>{false_child, true_child}));
    const model_node inner{model_node::match{.scrutinee = second_argument, .branches = {false_leaf, true_leaf}}};
    EXPECT_EQ(builder.build(root),
              (model_node{model_node::match{.scrutinee = first_feature, .branches = {empty_node, inner}}}));
}

TEST_F(ModelBuilderTest, AFunnelledBinIsNeitherSynthesizedNorRechosen) {
    ON_CALL(split_bin, split(root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin_view>{no_rows_child, funnel_child}));
    ON_CALL(choose_bound_argument, choose(funnel_child, 1)).WillByDefault(Return(first_bound_argument_candidate));
    ON_CALL(split_bin, split(funnel_child, first_bound_argument_candidate.observed))
        .WillByDefault(Return(std::vector<bin_view>{false_child, true_child}));
    EXPECT_CALL(synthesize_features, synthesize(_)).Times(AnyNumber());
    EXPECT_CALL(synthesize_features, synthesize(funnel_child)).Times(0);
    EXPECT_CALL(choose_feature, choose(_)).Times(1);
    builder.build(root);
}

TEST_F(ModelBuilderTest, AFunnelledBinWithADetectedOutputBecomesALeafInsteadOfMatchingAArgument) {
    ON_CALL(split_bin, split(root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin_view>{no_rows_child, funnel_child}));
    ON_CALL(detect_leaf, detect(funnel_child)).WillByDefault(Return(second_argument));
    EXPECT_CALL(choose_bound_argument, choose(_, _)).Times(0);
    const model_node argument_leaf{model_node::leaf{.output = second_argument}};
    EXPECT_EQ(builder.build(root),
              (model_node{model_node::match{.scrutinee = first_feature, .branches = {empty_node, argument_leaf}}}));
}

TEST_F(ModelBuilderTest, ArgumentsAreChosenFromThoseTheMatchJustBound) {
    const bin_view wide_root = bin_view::of(bin{
        .inputs = {typed_column{.type = 0, .cells = {falsity, truth}},
                  typed_column{.type = 0, .cells = {truth, falsity}}},
        .labels = typed_column{.type = 0, .cells = {truth, falsity}}});
    ON_CALL(split_bin, split(wide_root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin_view>{no_rows_child, funnel_child}));
    ON_CALL(split_bin, split(funnel_child, _)).WillByDefault(Return(std::vector<bin_view>{false_child, true_child}));
    EXPECT_CALL(choose_bound_argument, choose(_, _)).Times(AnyNumber());
    EXPECT_CALL(choose_bound_argument, choose(funnel_child, 2)).WillOnce(Return(second_argument_candidate));
    builder.build(wide_root);
}
