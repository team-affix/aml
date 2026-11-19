#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/model_builder.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockDetectLeaf {
    MOCK_METHOD(std::optional<const feature*>, detect, (const bin&), (const));
};

struct MockSynthesizeFeatures {
    MOCK_METHOD(const std::vector<candidate>&, synthesize, (const bin&));
};

struct MockFindOutput {
    MOCK_METHOD(std::optional<const feature*>, find, (const std::vector<candidate>&, const column&), (const));
};

struct MockChooseFeature {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseBoundArgument {
    MOCK_METHOD(candidate, choose, (const bin&, var_id));
};

struct MockSplitBin {
    MOCK_METHOD(std::vector<bin>, split, (const bin&, const typed_column&), (const));
};

struct MockMakeEmpty {
    MOCK_METHOD(const model_node*, make_empty, ());
};

struct MockMakeLeaf {
    MOCK_METHOD(const model_node*, make_leaf, (const feature*));
};

struct MockMakeMatch {
    MOCK_METHOD(const model_node*, make_match, (const feature*, const std::vector<const model_node*>&));
};

using builder_t = model_builder<
    NiceMock<MockDetectLeaf>,
    NiceMock<MockSynthesizeFeatures>,
    NiceMock<MockFindOutput>,
    NiceMock<MockChooseFeature>,
    NiceMock<MockChooseBoundArgument>,
    NiceMock<MockSplitBin>,
    NiceMock<MockMakeEmpty>,
    NiceMock<MockMakeLeaf>,
    NiceMock<MockMakeMatch>>;

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
    model_node inner_match;
    model_node outer_match;
    bin root;
    bin false_child;
    bin true_child;
    bin no_rows_child;
    bin funnel_child;
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
    NiceMock<MockMakeEmpty> make_empty;
    NiceMock<MockMakeLeaf> make_leaf;
    NiceMock<MockMakeMatch> make_match;
    builder_t builder{
        detect_leaf, synthesize_features, find_output, choose_feature, choose_bound_argument, split_bin, make_empty,
        make_leaf, make_match};

    void SetUp() override {
        ON_CALL(detect_leaf, detect(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(detect_leaf, detect(false_child)).WillByDefault(Return(&false_output));
        ON_CALL(detect_leaf, detect(true_child)).WillByDefault(Return(&true_output));
        ON_CALL(synthesize_features, synthesize(_)).WillByDefault(ReturnRef(features));
        ON_CALL(find_output, find(_, _)).WillByDefault(Return(std::nullopt));
        ON_CALL(choose_feature, choose(_)).WillByDefault(Return(0));
        ON_CALL(split_bin, split(root, first_candidate.observed))
            .WillByDefault(Return(std::vector<bin>{false_child, true_child}));
        ON_CALL(make_empty, make_empty()).WillByDefault(Return(&empty_node));
        ON_CALL(make_leaf, make_leaf(&false_output)).WillByDefault(Return(&false_leaf));
        ON_CALL(make_leaf, make_leaf(&true_output)).WillByDefault(Return(&true_leaf));
        ON_CALL(make_leaf, make_leaf(&second_feature)).WillByDefault(Return(&output_leaf));
        ON_CALL(make_match, make_match(_, _)).WillByDefault(Return(&outer_match));
    }
};

ModelBuilderTest::ModelBuilderTest()
    : falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , first_feature{feature::var{.index = 0}}
    , second_feature{feature::app{.prim = 0, .args = {&first_feature}}}
    , first_bound_argument{feature::var{.index = 1}}
    , second_argument{feature::var{.index = 2}}
    , false_output{feature::constant{.literal = &falsity}}
    , true_output{feature::constant{.literal = &truth}}
    , empty_node{model_node::empty{}}
    , false_leaf{model_node::leaf{.output = &false_output}}
    , true_leaf{model_node::leaf{.output = &true_output}}
    , output_leaf{model_node::leaf{.output = &second_feature}}
    , inner_match{model_node::match{.scrutinee = &second_argument, .branches = {}}}
    , outer_match{model_node::match{.scrutinee = &first_feature, .branches = {}}}
    , root{.scope = {typed_column{.type = 0, .cells = {&falsity, &truth}}}, .labels = {&truth, &falsity}}
    , false_child{.scope = {typed_column{.type = 0, .cells = {&falsity}}}, .labels = {&falsity}}
    , true_child{.scope = {typed_column{.type = 0, .cells = {&truth}}}, .labels = {&truth}}
    , no_rows_child{.scope = {typed_column{.type = 0, .cells = {}}}, .labels = {}}
    , funnel_child{
          .scope = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                    typed_column{.type = 0, .cells = {&truth, &truth}},
                    typed_column{.type = 0, .cells = {&truth, &falsity}}},
          .labels = {&truth, &falsity}}
    , first_candidate{.expression = &first_feature, .cost = 1, .observed = root.scope[0]}
    , second_candidate{
          .expression = &second_feature, .cost = 2, .observed = typed_column{.type = 0, .cells = {&truth, &falsity}}}
    , first_bound_argument_candidate{.expression = &first_bound_argument, .cost = 1, .observed = funnel_child.scope[1]}
    , second_argument_candidate{.expression = &second_argument, .cost = 1, .observed = funnel_child.scope[2]}
    , features{first_candidate, second_candidate} {}

}

TEST_F(ModelBuilderTest, ABinWithoutRowsBecomesEmptyWithoutDetection) {
    EXPECT_CALL(detect_leaf, detect(_)).Times(0);
    EXPECT_EQ(builder.build(no_rows_child), &empty_node);
}

TEST_F(ModelBuilderTest, ADetectedOutputBecomesALeafWithoutSynthesis) {
    EXPECT_CALL(synthesize_features, synthesize(_)).Times(0);
    EXPECT_EQ(builder.build(true_child), &true_leaf);
}

TEST_F(ModelBuilderTest, ABinWithNoDetectedOutputIsSynthesized) {
    EXPECT_CALL(synthesize_features, synthesize(root)).WillOnce(ReturnRef(features));
    builder.build(root);
}

TEST_F(ModelBuilderTest, ASynthesizedOutputBecomesALeafWithoutAMatch) {
    EXPECT_CALL(find_output, find(features, root.labels)).WillOnce(Return(&second_feature));
    EXPECT_CALL(choose_feature, choose(_)).Times(0);
    EXPECT_CALL(make_match, make_match(_, _)).Times(0);
    EXPECT_EQ(builder.build(root), &output_leaf);
}

TEST_F(ModelBuilderTest, OtherwiseTheBinMatchesOnTheChosenFeatureAndBuildsEachChild) {
    EXPECT_CALL(choose_feature, choose(2)).WillOnce(Return(1));
    EXPECT_CALL(split_bin, split(root, second_candidate.observed))
        .WillOnce(Return(std::vector<bin>{true_child, false_child}));
    EXPECT_CALL(make_match, make_match(&second_feature, std::vector<const model_node*>{&true_leaf, &false_leaf}))
        .WillOnce(Return(&outer_match));
    EXPECT_EQ(builder.build(root), &outer_match);
}

TEST_F(ModelBuilderTest, TheScrutineeOutlivesTheSessionItCameFrom) {
    EXPECT_CALL(choose_feature, choose(2)).WillOnce(Return(1));
    ON_CALL(split_bin, split(root, second_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{true_child, false_child}));
    EXPECT_CALL(detect_leaf, detect(_)).Times(AnyNumber());
    EXPECT_CALL(detect_leaf, detect(true_child)).WillOnce(::testing::DoAll(
        ::testing::Assign(&features, std::vector<candidate>{}), Return(&true_output)));
    EXPECT_CALL(make_match, make_match(&second_feature, _)).WillOnce(Return(&outer_match));
    builder.build(root);
}

TEST_F(ModelBuilderTest, EmptyChildrenBecomeEmptyBranches) {
    ON_CALL(split_bin, split(root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{false_child, no_rows_child, true_child}));
    EXPECT_CALL(make_match,
                make_match(&first_feature, std::vector<const model_node*>{&false_leaf, &empty_node, &true_leaf}))
        .WillOnce(Return(&outer_match));
    builder.build(root);
}

TEST_F(ModelBuilderTest, AMatchWhoseRowsShareOneConstructorFunnelsIntoAChosenBoundArgument) {
    InSequence order;
    EXPECT_CALL(split_bin, split(root, first_candidate.observed))
        .WillOnce(Return(std::vector<bin>{no_rows_child, funnel_child}));
    EXPECT_CALL(choose_bound_argument, choose(funnel_child, 1)).WillOnce(Return(second_argument_candidate));
    EXPECT_CALL(split_bin, split(funnel_child, second_argument_candidate.observed))
        .WillOnce(Return(std::vector<bin>{false_child, true_child}));
    EXPECT_CALL(make_match, make_match(&second_argument, std::vector<const model_node*>{&false_leaf, &true_leaf}))
        .WillOnce(Return(&inner_match));
    EXPECT_CALL(make_match, make_match(&first_feature, std::vector<const model_node*>{&empty_node, &inner_match}))
        .WillOnce(Return(&outer_match));
    EXPECT_EQ(builder.build(root), &outer_match);
}

TEST_F(ModelBuilderTest, AFunnelledBinIsNeitherSynthesizedNorRechosen) {
    ON_CALL(split_bin, split(root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{no_rows_child, funnel_child}));
    ON_CALL(choose_bound_argument, choose(funnel_child, 1)).WillByDefault(Return(first_bound_argument_candidate));
    ON_CALL(split_bin, split(funnel_child, first_bound_argument_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{false_child, true_child}));
    EXPECT_CALL(synthesize_features, synthesize(_)).Times(AnyNumber());
    EXPECT_CALL(synthesize_features, synthesize(funnel_child)).Times(0);
    EXPECT_CALL(choose_feature, choose(_)).Times(1);
    builder.build(root);
}

TEST_F(ModelBuilderTest, AFunnelledBinWithADetectedOutputBecomesALeafInsteadOfMatchingAArgument) {
    ON_CALL(split_bin, split(root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{no_rows_child, funnel_child}));
    ON_CALL(detect_leaf, detect(funnel_child)).WillByDefault(Return(&second_argument));
    ON_CALL(make_leaf, make_leaf(&second_argument)).WillByDefault(Return(&output_leaf));
    EXPECT_CALL(choose_bound_argument, choose(_, _)).Times(0);
    EXPECT_CALL(make_match, make_match(&first_feature, std::vector<const model_node*>{&empty_node, &output_leaf}))
        .WillOnce(Return(&outer_match));
    builder.build(root);
}

TEST_F(ModelBuilderTest, ArgumentsAreChosenFromThoseTheMatchJustBound) {
    const bin wide_root{
        .scope = {typed_column{.type = 0, .cells = {&falsity, &truth}},
                  typed_column{.type = 0, .cells = {&truth, &falsity}}},
        .labels = {&truth, &falsity}};
    ON_CALL(split_bin, split(wide_root, first_candidate.observed))
        .WillByDefault(Return(std::vector<bin>{no_rows_child, funnel_child}));
    ON_CALL(split_bin, split(funnel_child, _)).WillByDefault(Return(std::vector<bin>{false_child, true_child}));
    EXPECT_CALL(choose_bound_argument, choose(funnel_child, 2)).WillOnce(Return(second_argument_candidate));
    builder.build(wide_root);
}
