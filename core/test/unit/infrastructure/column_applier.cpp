#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/column_applier.hpp"

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockApplyPrimitive {
    MOCK_METHOD(const value*, apply, (prim_id, const std::vector<const value*>&), (const));
};

struct ColumnApplierTest : public ::testing::Test {
    ColumnApplierTest();
    value falsity;
    value truth;
    NiceMock<MockApplyPrimitive> apply_primitive;
    column_applier<NiceMock<MockApplyPrimitive>> applier{apply_primitive};
};

ColumnApplierTest::ColumnApplierTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , apply_primitive() {}

}

TEST_F(ColumnApplierTest, ThePrimitiveIsAppliedToEachRowsArguments) {
    const column left{&falsity, &truth, &truth};
    const column right{&truth, &truth, &falsity};
    InSequence order;
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&falsity, &truth})).WillOnce(Return(&falsity));
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &truth})).WillOnce(Return(&truth));
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &falsity})).WillOnce(Return(&falsity));
    EXPECT_EQ(applier.apply(2, {&left, &right}), (column{&falsity, &truth, &falsity}));
}

TEST_F(ColumnApplierTest, EmptyArgumentColumnsGiveAnEmptyColumn) {
    const column cells{};
    EXPECT_TRUE(applier.apply(0, {&cells}).empty());
}
