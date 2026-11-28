#include <any>
#include <cstddef>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/column_computer.hpp"

using namespace AML;

using ::testing::_;
using ::testing::MockFunction;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNativePrimitive {
    MOCK_METHOD(const native_primitive&, primitive, (prim_id), (const));
};

// Two boolean inputs over three rows. Primitive 5's application is a mock function.
struct ColumnComputerTest : public ::testing::Test {
    ColumnComputerTest();
    std::vector<native_column> inputs;
    MockFunction<std::any(const std::vector<operand_data>&, size_t)> apply;
    native_primitive applied;
    NiceMock<MockGetNativePrimitive> get_native_primitive;
    column_computer<NiceMock<MockGetNativePrimitive>> computer{get_native_primitive, inputs, 3};

    void SetUp() override {
        ON_CALL(get_native_primitive, primitive(5)).WillByDefault(ReturnRef(applied));
    }
};

ColumnComputerTest::ColumnComputerTest()
    : inputs{native_column::of(std::vector<bool>{false, true, true}),
             native_column::of(std::vector<bool>{true, false, false})}
    , apply()
    , applied{.arg_types = {typeid(bool)}, .result_type = typeid(bool), .apply = apply.AsStdFunction()}
    , get_native_primitive() {}

}

TEST_F(ColumnComputerTest, AnInputGivesItsOwnColumn) {
    const std::any computed = computer.compute(derivation{derivation::input{.index = 1}}, {});
    EXPECT_EQ(std::any_cast<const std::vector<bool>&>(computed), (std::vector<bool>{true, false, false}));
}

TEST_F(ColumnComputerTest, AnApplicationAppliesItsPrimitiveToTheOperandsOverEveryRow) {
    const std::any column(std::vector<bool>{false, false, true});
    const std::any scalar(true);
    const std::vector<operand_data> operands{
        operand_data{.data = &column, .scalar = false}, operand_data{.data = &scalar, .scalar = true}};
    EXPECT_CALL(apply, Call(operands, 3)).WillOnce(Return(std::any(std::vector<bool>{true, true, false})));
    const derivation origin{derivation::application{
        .prim = 5, .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_constant{.id = 0}}}}};
    EXPECT_EQ(std::any_cast<const std::vector<bool>&>(computer.compute(origin, operands)),
              (std::vector<bool>{true, true, false}));
}

TEST_F(ColumnComputerTest, AnInputAppliesNothing) {
    EXPECT_CALL(apply, Call(_, _)).Times(0);
    computer.compute(derivation{derivation::input{.index = 0}}, {});
}
