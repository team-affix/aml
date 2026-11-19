#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_enumerator.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::InSequence;

namespace {

struct MockGetTypeCtors {
    MOCK_METHOD(const std::vector<ctor_id>&, ctors, (type_id), (const));
};

struct MockGetCtorDecl {
    MOCK_METHOD(const ctor_decl&, decl, (ctor_id), (const));
};

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_id, const std::vector<const value*>&));
};

// Types: 0 = bool {false = 0, true = 1}, 1 = nat {zero = 2, suc = 3}, 2 = pair(bool, bool) {pair = 4}.
struct ConstantEnumeratorTest : public ::testing::Test {
    ConstantEnumeratorTest();
    std::vector<ctor_id> bool_ctors;
    std::vector<ctor_id> nat_ctors;
    std::vector<ctor_id> pair_ctors;
    ctor_decl false_decl;
    ctor_decl true_decl;
    ctor_decl zero_decl;
    ctor_decl suc_decl;
    ctor_decl pair_decl;
    value falsity;
    value truth;
    value zero;
    value one;
    value two;
    NiceMock<MockGetTypeCtors> get_type_ctors;
    NiceMock<MockGetCtorDecl> get_ctor_decl;
    NiceMock<MockMakeValue> make_value;
    constant_enumerator<NiceMock<MockGetTypeCtors>, NiceMock<MockGetCtorDecl>, NiceMock<MockMakeValue>> enumerator{
        get_type_ctors, get_ctor_decl, make_value};

    void SetUp() override {
        ON_CALL(get_type_ctors, ctors(0)).WillByDefault(ReturnRef(bool_ctors));
        ON_CALL(get_type_ctors, ctors(1)).WillByDefault(ReturnRef(nat_ctors));
        ON_CALL(get_type_ctors, ctors(2)).WillByDefault(ReturnRef(pair_ctors));
        ON_CALL(get_ctor_decl, decl(0)).WillByDefault(ReturnRef(false_decl));
        ON_CALL(get_ctor_decl, decl(1)).WillByDefault(ReturnRef(true_decl));
        ON_CALL(get_ctor_decl, decl(2)).WillByDefault(ReturnRef(zero_decl));
        ON_CALL(get_ctor_decl, decl(3)).WillByDefault(ReturnRef(suc_decl));
        ON_CALL(get_ctor_decl, decl(4)).WillByDefault(ReturnRef(pair_decl));
        ON_CALL(make_value, make(0, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(1, std::vector<const value*>{})).WillByDefault(Return(&truth));
        ON_CALL(make_value, make(2, std::vector<const value*>{})).WillByDefault(Return(&zero));
        ON_CALL(make_value, make(3, std::vector<const value*>{&zero})).WillByDefault(Return(&one));
        ON_CALL(make_value, make(3, std::vector<const value*>{&one})).WillByDefault(Return(&two));
    }
};

ConstantEnumeratorTest::ConstantEnumeratorTest()
    : bool_ctors{0, 1}
    , nat_ctors{2, 3}
    , pair_ctors{4}
    , false_decl{.type = 0, .tag = 0, .arg_types = {}}
    , true_decl{.type = 0, .tag = 1, .arg_types = {}}
    , zero_decl{.type = 1, .tag = 0, .arg_types = {}}
    , suc_decl{.type = 1, .tag = 1, .arg_types = {1}}
    , pair_decl{.type = 2, .tag = 0, .arg_types = {0, 0}}
    , falsity{.ctor = 0, .args = {}}
    , truth{.ctor = 1, .args = {}}
    , zero{.ctor = 2, .args = {}}
    , one{.ctor = 3, .args = {&zero}}
    , two{.ctor = 3, .args = {&one}}
    , get_type_ctors()
    , get_ctor_decl()
    , make_value() {}

}

TEST_F(ConstantEnumeratorTest, NullaryConstructorsAreTheValuesOfSizeOne) {
    EXPECT_EQ(enumerator.enumerate(0, 1), (std::vector<const value*>{&falsity, &truth}));
}

TEST_F(ConstantEnumeratorTest, RecursiveValuesAreEnumeratedSmallestFirstUpToTheSize) {
    EXPECT_EQ(enumerator.enumerate(1, 3), (std::vector<const value*>{&zero, &one, &two}));
}

TEST_F(ConstantEnumeratorTest, ASizeZeroEnumeratesNothing) {
    EXPECT_TRUE(enumerator.enumerate(1, 0).empty());
}

TEST_F(ConstantEnumeratorTest, EveryCombinationOfArgumentsThatFitsTheSizeIsMade) {
    EXPECT_CALL(make_value, make(_, _)).Times(AnyNumber());
    EXPECT_CALL(make_value, make(4, std::vector<const value*>{&falsity, &falsity}));
    EXPECT_CALL(make_value, make(4, std::vector<const value*>{&falsity, &truth}));
    EXPECT_CALL(make_value, make(4, std::vector<const value*>{&truth, &falsity}));
    EXPECT_CALL(make_value, make(4, std::vector<const value*>{&truth, &truth}));
    EXPECT_EQ(enumerator.enumerate(2, 3).size(), 4u);
}

TEST_F(ConstantEnumeratorTest, AValueWhoseArgumentsCannotFitIsNotMade) {
    EXPECT_CALL(make_value, make(4, _)).Times(0);
    EXPECT_TRUE(enumerator.enumerate(2, 2).empty());
}

TEST_F(ConstantEnumeratorTest, ASizeBelowTheSmallestValueEnumeratesNothing) {
    EXPECT_TRUE(enumerator.enumerate(0, 0).empty());
}

TEST_F(ConstantEnumeratorTest, ValuesOfOneSizeComeBeforeLargerOnes) {
    EXPECT_EQ(enumerator.enumerate(1, 2), (std::vector<const value*>{&zero, &one}));
}

TEST_F(ConstantEnumeratorTest, ASizeLargerThanAFiniteTypeNeedsGivesJustItsValues) {
    EXPECT_EQ(enumerator.enumerate(0, 5), (std::vector<const value*>{&falsity, &truth}));
}

TEST_F(ConstantEnumeratorTest, ConstructorsAreTriedInTagOrderWithinASize) {
    InSequence order;
    EXPECT_CALL(make_value, make(0, std::vector<const value*>{})).WillOnce(Return(&falsity));
    EXPECT_CALL(make_value, make(1, std::vector<const value*>{})).WillOnce(Return(&truth));
    enumerator.enumerate(0, 1);
}
