#include <any>
#include <functional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_computer.hpp"

using namespace AML;

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::MockFunction;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNativePrimitive {
    MOCK_METHOD(const native_primitive&, primitive, (prim_id), (const));
};

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

// Two constants are offered, 3 and true. Primitive 2 is a mock function from two integers to an integer.
struct ConstantComputerTest : public ::testing::Test {
    ConstantComputerTest();
    std::vector<native_constant> offered;
    native_type integer;
    MockFunction<std::any(const std::vector<operand_data>&, size_t)> apply;
    native_primitive applied;
    std::any left;
    std::any right;
    NiceMock<MockGetNativePrimitive> get_native_primitive;
    NiceMock<MockGetNativeType> get_native_type;
    constant_computer<NiceMock<MockGetNativePrimitive>, NiceMock<MockGetNativeType>> computer{
        get_native_primitive, get_native_type, offered};

    void SetUp() override {
        ON_CALL(get_native_primitive, primitive(2)).WillByDefault(ReturnRef(applied));
        ON_CALL(get_native_type, type(std::type_index(typeid(int)))).WillByDefault(ReturnRef(integer));
        ON_CALL(apply, Call(_, 1)).WillByDefault(Return(std::any(std::vector<int>{12})));
    }
};

ConstantComputerTest::ConstantComputerTest()
    : offered{native_constant::of(3), native_constant::of(true)}
    , integer(native_type::of<int, std::hash<int>>())
    , apply()
    , applied{.arg_types = {typeid(int), typeid(int)}, .result_type = typeid(int), .apply = apply.AsStdFunction()}
    , left(4)
    , right(3)
    , get_native_primitive()
    , get_native_type() {}

}

TEST_F(ConstantComputerTest, AnAtomIsItsOfferedValue) {
    EXPECT_EQ(std::any_cast<int>(computer.compute(constant_derivation{constant_derivation::atom{.index = 0}}, {})), 3);
    EXPECT_TRUE(std::any_cast<bool>(computer.compute(constant_derivation{constant_derivation::atom{.index = 1}}, {})));
}

TEST_F(ConstantComputerTest, AnApplicationIsItsPrimitivesOneCellOverTheOperandsAsOneRow) {
    const std::vector<operand_data> operands{
        operand_data{.data = &left, .scalar = true}, operand_data{.data = &right, .scalar = true}};
    EXPECT_CALL(apply, Call(ElementsAre(operands[0], operands[1]), 1))
        .WillOnce(Return(std::any(std::vector<int>{12})));
    const std::any cell = computer.compute(
        constant_derivation{constant_derivation::application{.prim = 2, .args = {5, 6}}}, operands);
    EXPECT_EQ(std::any_cast<int>(cell), 12);
}
