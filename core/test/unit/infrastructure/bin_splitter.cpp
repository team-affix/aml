#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/bin_splitter.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetTypeCtors {
    MOCK_METHOD(const std::vector<ctor_id>&, ctors, (type_id), (const));
};

struct MockGetCtorDecl {
    MOCK_METHOD(const ctor_decl&, decl, (ctor_id), (const));
};

// Types: 0 = bool, 1 = nat {zero = 2, suc = 3}.
struct BinSplitterTest : public ::testing::Test {
    BinSplitterTest();
    std::vector<ctor_id> nat_ctors;
    ctor_decl zero_decl;
    ctor_decl suc_decl;
    value falsity;
    value truth;
    value zero;
    value one;
    value two;
    NiceMock<MockGetTypeCtors> get_type_ctors;
    NiceMock<MockGetCtorDecl> get_ctor_decl;
    bin_splitter<NiceMock<MockGetTypeCtors>, NiceMock<MockGetCtorDecl>> splitter{get_type_ctors, get_ctor_decl};

    void SetUp() override {
        ON_CALL(get_type_ctors, ctors(1)).WillByDefault(ReturnRef(nat_ctors));
        ON_CALL(get_ctor_decl, decl(2)).WillByDefault(ReturnRef(zero_decl));
        ON_CALL(get_ctor_decl, decl(3)).WillByDefault(ReturnRef(suc_decl));
    }
};

BinSplitterTest::BinSplitterTest()
    : nat_ctors{2, 3}
    , zero_decl{.type = 1, .tag = 0, .field_types = {}}
    , suc_decl{.type = 1, .tag = 1, .field_types = {1}}
    , falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , zero{.ctor = 2, .fields = {}}
    , one{.ctor = 3, .fields = {&zero}}
    , two{.ctor = 3, .fields = {&one}}
    , get_type_ctors()
    , get_ctor_decl() {}

}

TEST_F(BinSplitterTest, RowsGoToTheirConstructorsChildWithItsFieldsBound) {
    const bin parent{
        .scope = {typed_column{.type = 1, .cells = {&zero, &one, &two}},
                  typed_column{.type = 0, .cells = {&truth, &falsity, &truth}}},
        .labels = {&falsity, &truth, &truth}};
    const bin zero_child{
        .scope = {typed_column{.type = 1, .cells = {&zero}}, typed_column{.type = 0, .cells = {&truth}}},
        .labels = {&falsity}};
    const bin suc_child{
        .scope = {typed_column{.type = 1, .cells = {&one, &two}},
                  typed_column{.type = 0, .cells = {&falsity, &truth}},
                  typed_column{.type = 1, .cells = {&zero, &one}}},
        .labels = {&truth, &truth}};
    EXPECT_EQ(splitter.split(parent, parent.scope[0]), (std::vector<bin>{zero_child, suc_child}));
}

TEST_F(BinSplitterTest, AConstructorNoRowHasGivesAnEmptyChildWithItsScopeTyped) {
    const bin parent{.scope = {typed_column{.type = 1, .cells = {&zero, &zero}}}, .labels = {&falsity, &truth}};
    const bin suc_child{
        .scope = {typed_column{.type = 1, .cells = {}}, typed_column{.type = 1, .cells = {}}}, .labels = {}};
    EXPECT_EQ(splitter.split(parent, parent.scope[0]).back(), suc_child);
}
