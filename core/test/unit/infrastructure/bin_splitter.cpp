#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/bin_splitter.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetTypeDef {
    MOCK_METHOD(const type_def&, def, (type_id), (const));
};

// Types: 0 = bool {false, true}, 1 = nat {zero, suc(nat)}.
struct BinSplitterTest : public ::testing::Test {
    BinSplitterTest();
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
    , falsity{.ctor = {.type = 0, .tag = 0}, .args = {}}
    , truth{.ctor = {.type = 0, .tag = 1}, .args = {}}
    , zero{.ctor = {.type = 1, .tag = 0}, .args = {}}
    , one{.ctor = {.type = 1, .tag = 1}, .args = {&zero}}
    , two{.ctor = {.type = 1, .tag = 1}, .args = {&one}}
    , get_type_def() {}

}

TEST_F(BinSplitterTest, RowsGoToTheirConstructorsChildWithItsArgumentsBound) {
    const bin parent{
        .inputs = {typed_column{.type = 1, .cells = {&zero, &one, &two}},
                   typed_column{.type = 0, .cells = {&truth, &falsity, &truth}}},
        .labels = {&falsity, &truth, &truth}};
    const bin zero_child{
        .inputs = {typed_column{.type = 1, .cells = {&zero}}, typed_column{.type = 0, .cells = {&truth}}},
        .labels = {&falsity}};
    const bin suc_child{
        .inputs = {typed_column{.type = 1, .cells = {&one, &two}},
                   typed_column{.type = 0, .cells = {&falsity, &truth}},
                   typed_column{.type = 1, .cells = {&zero, &one}}},
        .labels = {&truth, &truth}};
    EXPECT_EQ(splitter.split(parent, parent.inputs[0]), (std::vector<bin>{zero_child, suc_child}));
}

TEST_F(BinSplitterTest, AConstructorNoRowHasGivesAnEmptyChildWithItsInputsTyped) {
    const bin parent{.inputs = {typed_column{.type = 1, .cells = {&zero, &zero}}}, .labels = {&falsity, &truth}};
    const bin suc_child{
        .inputs = {typed_column{.type = 1, .cells = {}}, typed_column{.type = 1, .cells = {}}}, .labels = {}};
    EXPECT_EQ(splitter.split(parent, parent.inputs[0]).back(), suc_child);
}

TEST_F(BinSplitterTest, ThereIsOneChildPerConstructorInTagOrder) {
    const bin parent{.inputs = {typed_column{.type = 1, .cells = {&one, &zero}}}, .labels = {&truth, &falsity}};
    const std::vector<bin> children = splitter.split(parent, parent.inputs[0]);
    ASSERT_EQ(children.size(), 2u);
    EXPECT_EQ(children[0].labels, (column{&falsity}));
    EXPECT_EQ(children[1].labels, (column{&truth}));
}

TEST_F(BinSplitterTest, RowsKeepTheirOrderWithinAChild) {
    const bin parent{
        .inputs = {typed_column{.type = 1, .cells = {&two, &zero, &one}}}, .labels = {&truth, &falsity, &falsity}};
    const std::vector<bin> children = splitter.split(parent, parent.inputs[0]);
    EXPECT_EQ(children[1].inputs[0].cells, (column{&two, &one}));
    EXPECT_EQ(children[1].labels, (column{&truth, &falsity}));
    EXPECT_EQ(children[1].inputs[1].cells, (column{&one, &zero}));
}

TEST_F(BinSplitterTest, TheScrutineeNeedNotBeAVariableInScope) {
    const bin parent{.inputs = {typed_column{.type = 0, .cells = {&truth, &falsity}}}, .labels = {&truth, &falsity}};
    const typed_column derived{.type = 1, .cells = {&one, &zero}};
    const bin zero_child{.inputs = {typed_column{.type = 0, .cells = {&falsity}}}, .labels = {&falsity}};
    const bin suc_child{
        .inputs = {typed_column{.type = 0, .cells = {&truth}}, typed_column{.type = 1, .cells = {&zero}}},
        .labels = {&truth}};
    EXPECT_EQ(splitter.split(parent, derived), (std::vector<bin>{zero_child, suc_child}));
}

TEST_F(BinSplitterTest, EveryChildKeepsTheParentsVariablesFirst) {
    const bin parent{
        .inputs = {typed_column{.type = 0, .cells = {&truth, &falsity}},
                   typed_column{.type = 1, .cells = {&one, &zero}}},
        .labels = {&truth, &falsity}};
    for (const bin& child : splitter.split(parent, parent.inputs[1])) {
        EXPECT_EQ(child.inputs[0].type, 0u);
        EXPECT_EQ(child.inputs[1].type, 1u);
    }
}

TEST_F(BinSplitterTest, TheScrutineesTypeGivesTheConstructors) {
    const bin parent{.inputs = {typed_column{.type = 0, .cells = {&truth, &falsity}}}, .labels = {&truth, &falsity}};
    EXPECT_CALL(get_type_def, def(0)).WillOnce(ReturnRef(boolean));
    EXPECT_EQ(splitter.split(parent, parent.inputs[0]).size(), 2u);
}
