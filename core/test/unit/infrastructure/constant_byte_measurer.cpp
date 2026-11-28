#include <any>
#include <functional>
#include <typeindex>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_byte_measurer.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

struct ConstantByteMeasurerTest : public ::testing::Test {
    ConstantByteMeasurerTest();
    cached_constant half;
    native_type real;
    NiceMock<MockGetNativeType> get_native_type;
    constant_byte_measurer<NiceMock<MockGetNativeType>> measurer{get_native_type};

    void SetUp() override {
        ON_CALL(get_native_type, type(std::type_index(typeid(double)))).WillByDefault(ReturnRef(real));
    }
};

ConstantByteMeasurerTest::ConstantByteMeasurerTest()
    : half{.type = typeid(double), .cell = std::any(0.5)}
    , real(native_type::of<double, std::hash<double>>())
    , get_native_type() {}

}

TEST_F(ConstantByteMeasurerTest, AValueCostsOneCell) {
    EXPECT_EQ(measurer.measure(half), sizeof(double));
}

TEST_F(ConstantByteMeasurerTest, TheCellBytesComeFromTheValuesType) {
    real.cell_bytes = 100;
    EXPECT_EQ(measurer.measure(half), 100u);
}
