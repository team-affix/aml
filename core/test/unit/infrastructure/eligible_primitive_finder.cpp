#include <typeindex>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/eligible_primitive_finder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetPrimitivesOfArgumentType {
    MOCK_METHOD(const std::vector<prim_id>&, of_type, (std::type_index), (const));
};

using pool_t = std::unordered_map<std::type_index, std::vector<feature_id>>;

// int is taken by primitives 0 and 2; bool by 1 and 2; double by none.
struct EligiblePrimitiveFinderTest : public ::testing::Test {
    EligiblePrimitiveFinderTest();
    std::vector<prim_id> int_takers;
    std::vector<prim_id> bool_takers;
    std::vector<prim_id> none;
    NiceMock<MockGetPrimitivesOfArgumentType> get_primitives_of_argument_type;
    eligible_primitive_finder<NiceMock<MockGetPrimitivesOfArgumentType>> finder{get_primitives_of_argument_type};

    void SetUp() override {
        ON_CALL(get_primitives_of_argument_type, of_type(_)).WillByDefault(ReturnRef(none));
        ON_CALL(get_primitives_of_argument_type, of_type(std::type_index(typeid(int))))
            .WillByDefault(ReturnRef(int_takers));
        ON_CALL(get_primitives_of_argument_type, of_type(std::type_index(typeid(bool))))
            .WillByDefault(ReturnRef(bool_takers));
    }
};

EligiblePrimitiveFinderTest::EligiblePrimitiveFinderTest()
    : int_takers{0, 2}
    , bool_takers{1, 2}
    , none()
    , get_primitives_of_argument_type() {}

}

TEST_F(EligiblePrimitiveFinderTest, AnEmptyPoolFeedsNothing) {
    EXPECT_TRUE(finder.find(pool_t{}).empty());
}

TEST_F(EligiblePrimitiveFinderTest, APoolTypeMakesItsTakersEligible) {
    EXPECT_EQ(finder.find(pool_t{{typeid(int), {4}}}), (std::vector<prim_id>{0, 2}));
}

TEST_F(EligiblePrimitiveFinderTest, TakersOfSeveralTypesAreListedOnceInIncreasingId) {
    EXPECT_EQ(finder.find(pool_t{{typeid(bool), {1}}, {typeid(int), {4}}}), (std::vector<prim_id>{0, 1, 2}));
}

TEST_F(EligiblePrimitiveFinderTest, ATypeWithNoFeaturesMakesNothingEligible) {
    EXPECT_EQ(finder.find(pool_t{{typeid(bool), {}}, {typeid(int), {4}}}), (std::vector<prim_id>{0, 2}));
}

TEST_F(EligiblePrimitiveFinderTest, ATypeNoPrimitiveTakesMakesNothingEligible) {
    EXPECT_TRUE(finder.find(pool_t{{typeid(double), {4}}}).empty());
}
