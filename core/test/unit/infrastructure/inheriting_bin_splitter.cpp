#include <cstddef>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/inheriting_bin_splitter.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockSplitBin {
    MOCK_METHOD(std::vector<bin_view>, split, (const bin_view&, const column_view&), (const));
};

struct MockGetFeatureCandidates {
    MOCK_METHOD(const std::vector<candidate>&, candidates, (), (const));
};

struct MockFindScrutineePieces {
    MOCK_METHOD(std::vector<feature>, find, (const feature&), (const));
};

struct MockEvaluateFeatureColumn {
    MOCK_METHOD(column_view, evaluate, (const feature&, const bin_view&), (const));
};

// Values are bare constructors; the scrutinee's constructors are 0 and 1, so rows 0 and 2 go to the first child and
// row 1 to the second. Its pieces are `sum` and `twice`, read over the parent's three rows.
struct InheritingBinSplitterTest : public ::testing::Test {
    InheritingBinSplitterTest();
    static value leaf(ctor_id ctor);
    static column_view view(column cells);
    static bin_view one_column_bin(column cells);
    feature x0;
    feature twice;
    feature sum;
    bin_view parent;
    bin_view first_child;
    bin_view second_child;
    column_view scrutinee_column;
    column_view sum_column;
    column_view twice_column;
    std::vector<candidate> candidates;
    NiceMock<MockSplitBin> split_bin;
    NiceMock<MockGetFeatureCandidates> get_feature_candidates;
    NiceMock<MockFindScrutineePieces> find_scrutinee_pieces;
    NiceMock<MockEvaluateFeatureColumn> evaluate_feature_column;
    inheriting_bin_splitter<
        NiceMock<MockSplitBin>, NiceMock<MockGetFeatureCandidates>, NiceMock<MockFindScrutineePieces>,
        NiceMock<MockEvaluateFeatureColumn>>
        splitter{split_bin, get_feature_candidates, find_scrutinee_pieces, evaluate_feature_column};

    void SetUp() override {
        ON_CALL(split_bin, split(_, _)).WillByDefault(Return(std::vector<bin_view>{first_child, second_child}));
        ON_CALL(get_feature_candidates, candidates()).WillByDefault(ReturnRef(candidates));
        ON_CALL(find_scrutinee_pieces, find(sum)).WillByDefault(Return(std::vector<feature>{sum, twice}));
        ON_CALL(evaluate_feature_column, evaluate(sum, _)).WillByDefault(Return(sum_column));
        ON_CALL(evaluate_feature_column, evaluate(twice, _)).WillByDefault(Return(twice_column));
    }
};

InheritingBinSplitterTest::InheritingBinSplitterTest()
    : x0{feature::var{.index = 0}}
    , twice{feature::app{.prim = 1, .args = {x0}}}
    , sum{feature::app{.prim = 0, .args = {twice, x0}}}
    , parent(one_column_bin({leaf(0), leaf(1), leaf(2)}))
    , first_child(one_column_bin({leaf(0), leaf(2)}))
    , second_child(one_column_bin({leaf(1)}))
    , scrutinee_column(view({leaf(0), leaf(1), leaf(0)}))
    , sum_column(view({leaf(3), leaf(4), leaf(5)}))
    , twice_column(view({leaf(6), leaf(6), leaf(7)}))
    , candidates{candidate{.expression = x0, .cost = 1, .observed = parent.inputs[0]},
                 candidate{.expression = sum, .cost = 3, .observed = scrutinee_column}}
    , split_bin()
    , get_feature_candidates()
    , find_scrutinee_pieces()
    , evaluate_feature_column() {}

value InheritingBinSplitterTest::leaf(ctor_id ctor) {
    return value{.ctor = ctor, .args = {}};
}

column_view InheritingBinSplitterTest::view(column cells) {
    return column_view::of(typed_column{.type = 0, .cells = std::move(cells)});
}

bin_view InheritingBinSplitterTest::one_column_bin(column cells) {
    return bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = cells}},
                            .labels = typed_column{.type = 0, .cells = cells}});
}

}

TEST_F(InheritingBinSplitterTest, TheWrappedSplitterSplitsTheBin) {
    EXPECT_CALL(split_bin, split(_, _)).Times(AnyNumber());
    EXPECT_CALL(split_bin, split(parent, scrutinee_column))
        .WillOnce(Return(std::vector<bin_view>{first_child, second_child}));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, EachChildAppendsThePiecesOverItsOwnRows) {
    const std::vector<bin_view> children = splitter.split(parent, scrutinee_column);
    ASSERT_EQ(children[0].inputs.size(), 3u);
    EXPECT_EQ(children[0].inputs[1], view({leaf(3), leaf(5)}));
    EXPECT_EQ(children[0].inputs[2], view({leaf(6), leaf(7)}));
    ASSERT_EQ(children[1].inputs.size(), 3u);
    EXPECT_EQ(children[1].inputs[1], view({leaf(4)}));
    EXPECT_EQ(children[1].inputs[2], view({leaf(6)}));
}

TEST_F(InheritingBinSplitterTest, TheWrappedSplittersVariablesComeFirst) {
    const std::vector<bin_view> children = splitter.split(parent, scrutinee_column);
    EXPECT_EQ(children[0].inputs[0], first_child.inputs[0]);
    EXPECT_EQ(children[1].inputs[0], second_child.inputs[0]);
}

TEST_F(InheritingBinSplitterTest, TheChildrensLabelsAreTheWrappedSplitters) {
    const std::vector<bin_view> children = splitter.split(parent, scrutinee_column);
    EXPECT_EQ(children[0].labels, first_child.labels);
    EXPECT_EQ(children[1].labels, second_child.labels);
}

TEST_F(InheritingBinSplitterTest, ThePiecesAreFoundFromTheCandidateThatObservedTheScrutinee) {
    EXPECT_CALL(find_scrutinee_pieces, find(_)).Times(AnyNumber());
    EXPECT_CALL(find_scrutinee_pieces, find(sum)).WillOnce(Return(std::vector<feature>{sum, twice}));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, EachPieceIsEvaluatedOnceOverTheParent) {
    EXPECT_CALL(evaluate_feature_column, evaluate(_, _)).Times(AnyNumber());
    EXPECT_CALL(evaluate_feature_column, evaluate(sum, parent)).WillOnce(Return(sum_column));
    EXPECT_CALL(evaluate_feature_column, evaluate(twice, parent)).WillOnce(Return(twice_column));
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, AColumnNoCandidateObservedPassesNothing) {
    const column_view bound_argument = view({leaf(0), leaf(1), leaf(0)});
    EXPECT_CALL(evaluate_feature_column, evaluate(_, _)).Times(0);
    const std::vector<bin_view> children = splitter.split(parent, bound_argument);
    EXPECT_EQ(children, (std::vector<bin_view>{first_child, second_child}));
}

TEST_F(InheritingBinSplitterTest, AScrutineeWithoutPiecesPassesNothing) {
    ON_CALL(find_scrutinee_pieces, find(sum)).WillByDefault(Return(std::vector<feature>{}));
    const std::vector<bin_view> children = splitter.split(parent, scrutinee_column);
    EXPECT_EQ(children, (std::vector<bin_view>{first_child, second_child}));
}

TEST_F(InheritingBinSplitterTest, ACandidateReadingEqualValuesFromAnotherColumnIsNotTheScrutinee) {
    candidates[1].observed = view({leaf(0), leaf(1), leaf(0)});
    EXPECT_CALL(find_scrutinee_pieces, find(_)).Times(0);
    splitter.split(parent, scrutinee_column);
}

TEST_F(InheritingBinSplitterTest, AChildNoRowFallsIntoGetsEmptyPieces) {
    const column_view all_first = view({leaf(0), leaf(0), leaf(0)});
    candidates[1].observed = all_first;
    const bin_view empty_child = one_column_bin({});
    ON_CALL(split_bin, split(_, _)).WillByDefault(Return(std::vector<bin_view>{parent, empty_child}));
    const std::vector<bin_view> children = splitter.split(parent, all_first);
    ASSERT_EQ(children[1].inputs.size(), 3u);
    EXPECT_EQ(children[1].inputs[1], view({}));
}
