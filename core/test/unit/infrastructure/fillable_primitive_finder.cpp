#include <typeindex>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/fillable_primitive_finder.hpp"
#include "value_objects/constant_id.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNativePrimitive {
    MOCK_METHOD(const native_primitive&, primitive, (prim_id), (const));
};

struct MockGetConstantsOfType {
    MOCK_METHOD(const std::vector<constant_id>&, of_type, (std::type_index), (const));
};

using pool_t = std::unordered_map<std::type_index, std::vector<feature_id>>;

// Primitive 0 takes (int, bool), 1 takes int, 2 takes (int, double). Only double has a constant, constant 5.
struct FillablePrimitiveFinderTest : public ::testing::Test {
    FillablePrimitiveFinderTest();
    native_primitive int_bool;
    native_primitive int_only;
    native_primitive int_double;
    std::vector<constant_id> doubles;
    std::vector<constant_id> none;
    NiceMock<MockGetNativePrimitive> get_native_primitive;
    NiceMock<MockGetConstantsOfType> get_constants_of_type;
    fillable_primitive_finder<NiceMock<MockGetNativePrimitive>, NiceMock<MockGetConstantsOfType>> finder{
        get_native_primitive, get_constants_of_type};

    void SetUp() override {
        ON_CALL(get_native_primitive, primitive(0)).WillByDefault(ReturnRef(int_bool));
        ON_CALL(get_native_primitive, primitive(1)).WillByDefault(ReturnRef(int_only));
        ON_CALL(get_native_primitive, primitive(2)).WillByDefault(ReturnRef(int_double));
        ON_CALL(get_constants_of_type, of_type(_)).WillByDefault(ReturnRef(none));
        ON_CALL(get_constants_of_type, of_type(std::type_index(typeid(double)))).WillByDefault(ReturnRef(doubles));
    }
};

FillablePrimitiveFinderTest::FillablePrimitiveFinderTest()
    : int_bool{.arg_types = {typeid(int), typeid(bool)}, .result_type = typeid(int), .apply = nullptr}
    , int_only{.arg_types = {typeid(int)}, .result_type = typeid(int), .apply = nullptr}
    , int_double{.arg_types = {typeid(int), typeid(double)}, .result_type = typeid(int), .apply = nullptr}
    , doubles{5}
    , none()
    , get_native_primitive()
    , get_constants_of_type() {}

}

TEST_F(FillablePrimitiveFinderTest, AnArgumentWithNoFeatureAndNoConstantLeavesThePrimitiveOut) {
    const pool_t pool{{typeid(int), {3}}};
    EXPECT_EQ(finder.find(pool, {0, 1}), (std::vector<prim_id>{1}));
}

TEST_F(FillablePrimitiveFinderTest, AConstantFillsAnArgumentThePoolCannot) {
    const pool_t pool{{typeid(int), {3}}};
    EXPECT_EQ(finder.find(pool, {1, 2}), (std::vector<prim_id>{1, 2}));
}

TEST_F(FillablePrimitiveFinderTest, AFeatureFillsEveryArgumentOfItsType) {
    const pool_t pool{{typeid(int), {3}}, {typeid(bool), {4}}};
    EXPECT_EQ(finder.find(pool, {0, 1, 2}), (std::vector<prim_id>{0, 1, 2}));
}

TEST_F(FillablePrimitiveFinderTest, AnEmptyListForATypeFillsNothing) {
    const pool_t pool{{typeid(int), {3}}, {typeid(bool), {}}};
    EXPECT_EQ(finder.find(pool, {0}), (std::vector<prim_id>{}));
}

TEST_F(FillablePrimitiveFinderTest, TheEligibleOrderIsKept) {
    const pool_t pool{{typeid(int), {3}}, {typeid(bool), {4}}};
    EXPECT_EQ(finder.find(pool, {2, 0}), (std::vector<prim_id>{2, 0}));
}

TEST_F(FillablePrimitiveFinderTest, NoEligiblePrimitivesFindNone) {
    const pool_t pool{{typeid(int), {3}}};
    EXPECT_TRUE(finder.find(pool, {}).empty());
}
