#include <any>
#include <functional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/column_byte_measurer.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNativeType {
    MOCK_METHOD(const native_type&, type, (std::type_index), (const));
};

struct ColumnByteMeasurerTest : public ::testing::Test {
    ColumnByteMeasurerTest();
    cached_column doubles_of(size_t row_count);
    native_type real;
    NiceMock<MockGetNativeType> get_native_type;
    column_byte_measurer<NiceMock<MockGetNativeType>> measurer{get_native_type};

    void SetUp() override {
        ON_CALL(get_native_type, type(std::type_index(typeid(double)))).WillByDefault(ReturnRef(real));
    }
};

ColumnByteMeasurerTest::ColumnByteMeasurerTest()
    : real(native_type::of<double, std::hash<double>>())
    , get_native_type() {}

cached_column ColumnByteMeasurerTest::doubles_of(size_t row_count) {
    return cached_column{
        .type = typeid(double), .cells = std::any(std::vector<double>(row_count, 1.5)),
        .classes = std::vector<size_t>(row_count, 0), .hash = 0};
}

}

TEST_F(ColumnByteMeasurerTest, EachRowCostsItsCellAndItsClass) {
    EXPECT_EQ(measurer.measure(doubles_of(3)), 3 * (sizeof(double) + sizeof(size_t)));
}

TEST_F(ColumnByteMeasurerTest, AnEmptyColumnHoldsNothing) {
    EXPECT_EQ(measurer.measure(doubles_of(0)), 0u);
}

TEST_F(ColumnByteMeasurerTest, TheCellBytesComeFromTheColumnsType) {
    real.cell_bytes = 100;
    EXPECT_EQ(measurer.measure(doubles_of(2)), 2 * (100 + sizeof(size_t)));
}
