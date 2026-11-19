#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/primitive_option_finder.hpp"

using ::testing::AnyNumber;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetPrimitiveCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockGetPrimitiveType {
    MOCK_METHOD(const primitive_type&, type, (prim_id), (const));
};

struct MockDetectWitness {
    MOCK_METHOD(bool, detect, (prim_id, const std::vector<type_id>&, const std::vector<size_t>&), (const));
};

// Three primitives; primitive 1 has no witness.
struct PrimitiveOptionFinderTest : public ::testing::Test {
    PrimitiveOptionFinderTest();
    primitive_type unary;
    primitive_type binary;
    NiceMock<MockGetPrimitiveCount> get_primitive_count;
    NiceMock<MockGetPrimitiveType> get_primitive_type;
    NiceMock<MockDetectWitness> detect_witness;
    primitive_option_finder<
        NiceMock<MockGetPrimitiveCount>, NiceMock<MockGetPrimitiveType>, NiceMock<MockDetectWitness>>
        finder{get_primitive_count, get_primitive_type, detect_witness};

    void SetUp() override {
        ON_CALL(get_primitive_count, count()).WillByDefault(Return(3));
        ON_CALL(get_primitive_type, type(_)).WillByDefault(ReturnRef(unary));
        ON_CALL(detect_witness, detect(_, _, _)).WillByDefault(Return(true));
        ON_CALL(detect_witness, detect(1, _, _)).WillByDefault(Return(false));
    }
};

PrimitiveOptionFinderTest::PrimitiveOptionFinderTest()
    : unary{.arg_types = {0}, .result_type = 0}
    , binary{.arg_types = {4, 5}, .result_type = 6}
    , get_primitive_count()
    , get_primitive_type()
    , detect_witness() {}

}

TEST_F(PrimitiveOptionFinderTest, ThePrimitivesWithAWitnessAreFoundInIdOrder) {
    EXPECT_EQ(finder.find(), (std::vector<prim_id>{0, 2}));
}

TEST_F(PrimitiveOptionFinderTest, AWitnessIsSoughtForEachTypeFromAnEmptyPrefix) {
    ON_CALL(get_primitive_type, type(2)).WillByDefault(ReturnRef(binary));
    EXPECT_CALL(detect_witness, detect(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(detect_witness, detect(2, std::vector<type_id>{4, 5}, std::vector<size_t>{})).WillOnce(Return(true));
    finder.find();
}

TEST_F(PrimitiveOptionFinderTest, NoPrimitivesGiveNoOptions) {
    ON_CALL(get_primitive_count, count()).WillByDefault(Return(0));
    EXPECT_TRUE(finder.find().empty());
}

TEST_F(PrimitiveOptionFinderTest, NoWitnessesGiveNoOptions) {
    ON_CALL(detect_witness, detect(_, _, _)).WillByDefault(Return(false));
    EXPECT_TRUE(finder.find().empty());
}

TEST_F(PrimitiveOptionFinderTest, EveryPrimitivesTypeIsLookedUp) {
    EXPECT_CALL(get_primitive_type, type(0)).WillOnce(ReturnRef(unary));
    EXPECT_CALL(get_primitive_type, type(1)).WillOnce(ReturnRef(unary));
    EXPECT_CALL(get_primitive_type, type(2)).WillOnce(ReturnRef(unary));
    finder.find();
}
