#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/operand_applier.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetOperandCell {
    MOCK_METHOD(const value*, cell, (size_t, size_t), (const));
};

struct MockApplyPrimitive {
    MOCK_METHOD(const value*, apply, (prim_id, const std::vector<const value*>&), (const));
};

// Operand 4 is false then true; operand 7 is true then false.
struct OperandApplierTest : public ::testing::Test {
    OperandApplierTest();
    value falsity;
    value truth;
    NiceMock<MockGetOperandCell> get_operand_cell;
    NiceMock<MockApplyPrimitive> apply_primitive;
    operand_applier<NiceMock<MockGetOperandCell>, NiceMock<MockApplyPrimitive>> applier{
        get_operand_cell, apply_primitive};

    void SetUp() override {
        ON_CALL(get_operand_cell, cell(4, 0)).WillByDefault(Return(&falsity));
        ON_CALL(get_operand_cell, cell(4, 1)).WillByDefault(Return(&truth));
        ON_CALL(get_operand_cell, cell(7, 0)).WillByDefault(Return(&truth));
        ON_CALL(get_operand_cell, cell(7, 1)).WillByDefault(Return(&falsity));
    }
};

OperandApplierTest::OperandApplierTest()
    : falsity{.ctor = 0, .fields = {}}
    , truth{.ctor = 1, .fields = {}}
    , get_operand_cell()
    , apply_primitive() {}

}

TEST_F(OperandApplierTest, ThePrimitiveIsAppliedToEachRowsOperandCells) {
    InSequence order;
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&falsity, &truth})).WillOnce(Return(&falsity));
    EXPECT_CALL(apply_primitive, apply(2, std::vector<const value*>{&truth, &falsity})).WillOnce(Return(&truth));
    EXPECT_EQ(applier.apply(2, {4, 7}, 2), (column{&falsity, &truth}));
}

TEST_F(OperandApplierTest, NoRowsGiveAnEmptyColumn) {
    EXPECT_CALL(apply_primitive, apply(_, _)).Times(0);
    EXPECT_TRUE(applier.apply(0, {4}, 0).empty());
}
