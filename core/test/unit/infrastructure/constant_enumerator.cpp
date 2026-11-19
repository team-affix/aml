#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_enumerator.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetTypeDef {
    MOCK_METHOD(const type_def&, def, (type_id), (const));
};

struct MockMakeValue {
    MOCK_METHOD(const value*, make, (ctor_ref, const std::vector<const value*>&));
};

// Types: 0 = bool {false, true}, 1 = nat {zero, suc(nat)}, 2 = pair(bool, bool).
struct ConstantEnumeratorTest : public ::testing::Test {
    ConstantEnumeratorTest();
    type_def boolean;
    type_def nat;
    type_def pair;
    ctor_ref false_ctor;
    ctor_ref true_ctor;
    ctor_ref zero_ctor;
    ctor_ref suc_ctor;
    ctor_ref pair_ctor;
    value falsity;
    value truth;
    value zero;
    value one;
    value two;
    NiceMock<MockGetTypeDef> get_type_def;
    NiceMock<MockMakeValue> make_value;
    constant_enumerator<NiceMock<MockGetTypeDef>, NiceMock<MockMakeValue>> enumerator{get_type_def, make_value};

    void SetUp() override {
        ON_CALL(get_type_def, def(0)).WillByDefault(ReturnRef(boolean));
        ON_CALL(get_type_def, def(1)).WillByDefault(ReturnRef(nat));
        ON_CALL(get_type_def, def(2)).WillByDefault(ReturnRef(pair));
        ON_CALL(make_value, make(false_ctor, std::vector<const value*>{})).WillByDefault(Return(&falsity));
        ON_CALL(make_value, make(true_ctor, std::vector<const value*>{})).WillByDefault(Return(&truth));
        ON_CALL(make_value, make(zero_ctor, std::vector<const value*>{})).WillByDefault(Return(&zero));
        ON_CALL(make_value, make(suc_ctor, std::vector<const value*>{&zero})).WillByDefault(Return(&one));
        ON_CALL(make_value, make(suc_ctor, std::vector<const value*>{&one})).WillByDefault(Return(&two));
    }
};

ConstantEnumeratorTest::ConstantEnumeratorTest()
    : boolean{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {}}}}
    , nat{.ctors = {ctor_def{.arg_types = {}}, ctor_def{.arg_types = {1}}}}
    , pair{.ctors = {ctor_def{.arg_types = {0, 0}}}}
    , false_ctor{.type = 0, .tag = 0}
    , true_ctor{.type = 0, .tag = 1}
    , zero_ctor{.type = 1, .tag = 0}
    , suc_ctor{.type = 1, .tag = 1}
    , pair_ctor{.type = 2, .tag = 0}
    , falsity{.ctor = false_ctor, .args = {}}
    , truth{.ctor = true_ctor, .args = {}}
    , zero{.ctor = zero_ctor, .args = {}}
    , one{.ctor = suc_ctor, .args = {&zero}}
    , two{.ctor = suc_ctor, .args = {&one}}
    , get_type_def()
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
    EXPECT_CALL(make_value, make(pair_ctor, std::vector<const value*>{&falsity, &falsity}));
    EXPECT_CALL(make_value, make(pair_ctor, std::vector<const value*>{&falsity, &truth}));
    EXPECT_CALL(make_value, make(pair_ctor, std::vector<const value*>{&truth, &falsity}));
    EXPECT_CALL(make_value, make(pair_ctor, std::vector<const value*>{&truth, &truth}));
    EXPECT_EQ(enumerator.enumerate(2, 3).size(), 4u);
}

TEST_F(ConstantEnumeratorTest, AValueWhoseArgumentsCannotFitIsNotMade) {
    EXPECT_CALL(make_value, make(pair_ctor, _)).Times(0);
    EXPECT_TRUE(enumerator.enumerate(2, 2).empty());
}

TEST_F(ConstantEnumeratorTest, ValuesOfOneSizeComeBeforeLargerOnes) {
    EXPECT_EQ(enumerator.enumerate(1, 2), (std::vector<const value*>{&zero, &one}));
}

TEST_F(ConstantEnumeratorTest, ASizeLargerThanAFiniteTypeNeedsGivesJustItsValues) {
    EXPECT_EQ(enumerator.enumerate(0, 5), (std::vector<const value*>{&falsity, &truth}));
}

TEST_F(ConstantEnumeratorTest, ConstructorsAreTriedInTagOrderWithinASize) {
    InSequence order;
    EXPECT_CALL(make_value, make(false_ctor, std::vector<const value*>{})).WillOnce(Return(&falsity));
    EXPECT_CALL(make_value, make(true_ctor, std::vector<const value*>{})).WillOnce(Return(&truth));
    enumerator.enumerate(0, 1);
}

TEST_F(ConstantEnumeratorTest, AValueIsMadeWithItsTypeAndTag) {
    EXPECT_CALL(make_value, make(_, _)).Times(AnyNumber());
    EXPECT_CALL(make_value, make(ctor_ref{.type = 1, .tag = 1}, std::vector<const value*>{&zero}))
        .WillOnce(Return(&one));
    enumerator.enumerate(1, 2);
}

TEST_F(ConstantEnumeratorTest, ATypeWithoutConstructorsHasNoValues) {
    const type_def uninhabited{.ctors = {}};
    ON_CALL(get_type_def, def(7)).WillByDefault(ReturnRef(uninhabited));
    EXPECT_TRUE(enumerator.enumerate(7, 4).empty());
}
