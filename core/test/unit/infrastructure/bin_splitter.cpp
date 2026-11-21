#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/bin_splitter.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetTypeDef {
    MOCK_METHOD(const type_def&, def, (type_id), (const));
};

// Types: 0 = bool {false, true}, 1 = nat {zero, suc(nat)}.
struct BinSplitterTest : public ::testing::Test {
    BinSplitterTest();
    column_view view(type_id type, column cells);
    type_def boolean;
    type_def nat;
    value falsity;
    value truth;
    value zero;
    value one;
    value two;
    NiceMock<MockGetTypeDef> get_type_def;
    bin_splitter<NiceMock<MockGetTypeDef>> splitter{get_type_def};

    void SetUp() override {
        ON_CALL(get_type_def, def(0)).WillByDefault(ReturnRef(boolean));
        ON_CALL(get_type_def, def(1)).WillByDefault(ReturnRef(nat));
    }
};

BinSplitterTest::BinSplitterTest()
    : boolean{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {}}}}
    , nat{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {1}}}}
    , falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , zero{.ctor = 0, .args = {}}
    , one{.ctor = 1, .args = {zero}}
    , two{.ctor = 1, .args = {one}}
    , get_type_def() {}

column_view BinSplitterTest::view(type_id type, column cells) {
    return column_view::of(typed_column{.type = type, .cells = std::move(cells)});
}

}

TEST_F(BinSplitterTest, RowsGoToTheirConstructorsChildWithItsArgumentsBound) {
    const bin_view parent = bin_view::of(bin{
        .inputs = {typed_column{.type = 1, .cells = {zero, one, two}},
                   typed_column{.type = 0, .cells = {truth, falsity, truth}}},
        .labels = typed_column{.type = 0, .cells = {falsity, truth, truth}}});
    const bin_view zero_child = bin_view::of(bin{
        .inputs = {typed_column{.type = 1, .cells = {zero}}, typed_column{.type = 0, .cells = {truth}}},
        .labels = typed_column{.type = 0, .cells = {falsity}}});
    const bin_view suc_child = bin_view::of(bin{
        .inputs = {typed_column{.type = 1, .cells = {one, two}},
                   typed_column{.type = 0, .cells = {falsity, truth}},
                   typed_column{.type = 1, .cells = {zero, one}}},
        .labels = typed_column{.type = 0, .cells = {truth, truth}}});
    EXPECT_EQ(splitter.split(parent, parent.inputs[0]), (std::vector<bin_view>{zero_child, suc_child}));
}

TEST_F(BinSplitterTest, AConstructorNoRowHasGivesAnEmptyChildWithItsInputsTyped) {
    const bin_view parent = bin_view::of(bin{.inputs = {typed_column{.type = 1, .cells = {zero, zero}}},
                     .labels = typed_column{.type = 0, .cells = {falsity, truth}}});
    const bin_view suc_child = bin_view::of(bin{
        .inputs = {typed_column{.type = 1, .cells = {}}, typed_column{.type = 1, .cells = {}}},
        .labels = typed_column{.type = 0, .cells = {}}});
    EXPECT_EQ(splitter.split(parent, parent.inputs[0]).back(), suc_child);
}

TEST_F(BinSplitterTest, ThereIsOneChildPerConstructorInTagOrder) {
    const bin_view parent = bin_view::of(bin{.inputs = {typed_column{.type = 1, .cells = {one, zero}}},
                     .labels = typed_column{.type = 0, .cells = {truth, falsity}}});
    const std::vector<bin_view> children = splitter.split(parent, parent.inputs[0]);
    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(children[0].labels, view(0, {falsity}));
    EXPECT_EQ(children[1].labels, view(0, {truth}));
}

TEST_F(BinSplitterTest, RowsKeepTheirOrderWithinAChild) {
    const bin_view parent = bin_view::of(bin{
        .inputs = {typed_column{.type = 1, .cells = {two, zero, one}}},
        .labels = typed_column{.type = 0, .cells = {truth, falsity, falsity}}});
    const std::vector<bin_view> children = splitter.split(parent, parent.inputs[0]);
    EXPECT_EQ(children[1].inputs[0], view(1, {two, one}));
    EXPECT_EQ(children[1].labels, view(0, {truth, falsity}));
    EXPECT_EQ(children[1].inputs[1], view(1, {one, zero}));
}

TEST_F(BinSplitterTest, TheScrutineeNeedNotBeAVariableInScope) {
    const bin_view parent = bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {truth, falsity}}},
                     .labels = typed_column{.type = 0, .cells = {truth, falsity}}});
    const column_view derived = column_view::of(typed_column{.type = 1, .cells = {one, zero}});
    const bin_view zero_child = bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {falsity}}},
                         .labels = typed_column{.type = 0, .cells = {falsity}}});
    const bin_view suc_child = bin_view::of(bin{
        .inputs = {typed_column{.type = 0, .cells = {truth}}, typed_column{.type = 1, .cells = {zero}}},
        .labels = typed_column{.type = 0, .cells = {truth}}});
    EXPECT_EQ(splitter.split(parent, derived), (std::vector<bin_view>{zero_child, suc_child}));
}

TEST_F(BinSplitterTest, EveryChildKeepsTheParentsVariablesFirst) {
    const bin_view parent = bin_view::of(bin{
        .inputs = {typed_column{.type = 0, .cells = {truth, falsity}},
                   typed_column{.type = 1, .cells = {one, zero}}},
        .labels = typed_column{.type = 0, .cells = {truth, falsity}}});
    for (const bin_view& child : splitter.split(parent, parent.inputs[1])) {
        EXPECT_EQ(child.inputs[0].type, 0u);
        EXPECT_EQ(child.inputs[1].type, 1u);
    }
}

TEST_F(BinSplitterTest, TheScrutineesTypeGivesTheConstructors) {
    const bin_view parent = bin_view::of(bin{.inputs = {typed_column{.type = 0, .cells = {truth, falsity}}},
                     .labels = typed_column{.type = 0, .cells = {truth, falsity}}});
    EXPECT_CALL(get_type_def, def(0)).WillOnce(ReturnRef(boolean));
    EXPECT_EQ(splitter.split(parent, parent.inputs[0]).size(), 2u);
}

TEST_F(BinSplitterTest, AChildSharesItsParentsCellsAndSelectsItsRows) {
    const bin_view parent = bin_view::of(bin{
        .inputs = {typed_column{.type = 0, .cells = {truth, falsity, truth}}},
        .labels = typed_column{.type = 0, .cells = {falsity, truth, truth}}});
    const std::vector<bin_view> children = splitter.split(parent, parent.inputs[0]);
    EXPECT_EQ(children[1].inputs[0].cells, parent.inputs[0].cells);
    EXPECT_EQ(children[1].labels.cells, parent.labels.cells);
    EXPECT_EQ(*children[1].inputs[0].rows, (std::vector<size_t>{0, 2}));
}

TEST_F(BinSplitterTest, ColumnsSharingTheParentsSelectionShareTheChilds) {
    const bin_view parent = bin_view::of(bin{
        .inputs = {typed_column{.type = 0, .cells = {truth, falsity}},
                   typed_column{.type = 0, .cells = {falsity, truth}}},
        .labels = typed_column{.type = 0, .cells = {falsity, truth}}});
    const std::vector<bin_view> children = splitter.split(parent, parent.inputs[0]);
    EXPECT_EQ(children[0].inputs[0].rows, children[0].labels.rows);
    EXPECT_EQ(children[0].inputs[1].rows, children[0].labels.rows);
}

TEST_F(BinSplitterTest, ABoundArgumentHasCellsOfItsOwnForTheChildsRows) {
    const bin_view parent = bin_view::of(bin{
        .inputs = {typed_column{.type = 1, .cells = {two, zero, one}}},
        .labels = typed_column{.type = 0, .cells = {truth, falsity, falsity}}});
    const std::vector<bin_view> children = splitter.split(parent, parent.inputs[0]);
    const column_view& bound = children[1].inputs[1];
    EXPECT_NE(bound.cells, parent.inputs[0].cells);
    EXPECT_EQ(*bound.cells, (column{one, zero}));
    EXPECT_EQ(*bound.rows, (std::vector<size_t>{0, 1}));
}

TEST_F(BinSplitterTest, AParentsSelectionIsComposedWithTheChilds) {
    const column_view selected{
        .type = 0,
        .cells = std::make_shared<const column>(column{falsity, truth, falsity, truth}),
        .rows = std::make_shared<const std::vector<size_t>>(std::vector<size_t>{3, 0, 1})};
    const bin_view parent{.inputs = {selected}, .labels = selected};
    const std::vector<bin_view> children = splitter.split(parent, parent.inputs[0]);
    EXPECT_EQ(*children[0].labels.rows, (std::vector<size_t>{0}));
    EXPECT_EQ(*children[1].labels.rows, (std::vector<size_t>{3, 1}));
    EXPECT_EQ(children[1].labels, view(0, {truth, truth}));
}

TEST_F(BinSplitterTest, ColumnsWithDifferentSelectionsAreSelectedApart) {
    const bin_view parent = bin_view::of(bin{
        .inputs = {typed_column{.type = 1, .cells = {two, one, zero}}},
        .labels = typed_column{.type = 0, .cells = {truth, falsity, truth}}});
    const bin_view suc_child = splitter.split(parent, parent.inputs[0])[1];
    const std::vector<bin_view> grandchildren = splitter.split(suc_child, suc_child.inputs[1]);
    const bin_view& suc_suc = grandchildren[1];
    EXPECT_EQ(suc_suc.inputs[0], view(1, {two}));
    EXPECT_EQ(suc_suc.inputs[1], view(1, {one}));
    EXPECT_EQ(suc_suc.inputs[2], view(1, {zero}));
    EXPECT_EQ(suc_suc.labels, view(0, {truth}));
    EXPECT_NE(suc_suc.inputs[0].rows, suc_suc.inputs[1].rows);
}
