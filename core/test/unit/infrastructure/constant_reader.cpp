#include <any>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_reader.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetConstantRecord {
    MOCK_METHOD(const constant_record&, record, (constant_id), (const));
};

struct MockFindCachedConstant {
    MOCK_METHOD(std::shared_ptr<const cached_constant>, find, (constant_id));
};

struct MockComputeConstant {
    MOCK_METHOD(std::any, compute, (const constant_derivation&, const std::vector<operand_data>&), (const));
};

struct MockMeasureConstantBytes {
    MOCK_METHOD(size_t, measure, (const cached_constant&), (const));
};

struct MockInsertCachedConstant {
    MOCK_METHOD(void, insert, (constant_id, std::shared_ptr<const cached_constant>, size_t));
};

MATCHER_P(HoldsInteger, expected, "") {
    return std::any_cast<int>(arg->cell) == expected;
}

using test_constant_reader_t = constant_reader<
    NiceMock<MockGetConstantRecord>, NiceMock<MockFindCachedConstant>, NiceMock<MockComputeConstant>,
    NiceMock<MockMeasureConstantBytes>, NiceMock<MockInsertCachedConstant>>;

// Constant 0 is the atom 3; constant 1, an integer, applies primitive 4 to constant 0 twice; constant 2 is a boolean
// atom.
struct ConstantReaderTest : public ::testing::Test {
    ConstantReaderTest();
    constant_record three;
    constant_record square;
    constant_record truth;
    std::shared_ptr<const cached_constant> three_cached;
    NiceMock<MockGetConstantRecord> get_constant_record;
    NiceMock<MockFindCachedConstant> find_cached_constant;
    NiceMock<MockComputeConstant> compute_constant;
    NiceMock<MockMeasureConstantBytes> measure_constant_bytes;
    NiceMock<MockInsertCachedConstant> insert_cached_constant;
    test_constant_reader_t reader{
        get_constant_record, find_cached_constant, compute_constant, measure_constant_bytes, insert_cached_constant};

    void SetUp() override {
        ON_CALL(get_constant_record, record(0)).WillByDefault(ReturnRef(three));
        ON_CALL(get_constant_record, record(1)).WillByDefault(ReturnRef(square));
        ON_CALL(get_constant_record, record(2)).WillByDefault(ReturnRef(truth));
        ON_CALL(compute_constant, compute(three.origin, _)).WillByDefault(Return(std::any(3)));
        ON_CALL(compute_constant, compute(square.origin, _)).WillByDefault(Return(std::any(9)));
        ON_CALL(compute_constant, compute(truth.origin, _)).WillByDefault(Return(std::any(true)));
        ON_CALL(measure_constant_bytes, measure(_)).WillByDefault(Return(4));
    }
};

ConstantReaderTest::ConstantReaderTest()
    : three{.origin = constant_derivation{constant_derivation::atom{.index = 0}}, .type = typeid(int)}
    , square{
          .origin = constant_derivation{constant_derivation::application{.prim = 4, .args = {0, 0}}},
          .type = typeid(int)}
    , truth{.origin = constant_derivation{constant_derivation::atom{.index = 1}}, .type = typeid(bool)}
    , three_cached(std::make_shared<const cached_constant>(cached_constant{.type = typeid(int), .cell = std::any(3)}))
    , get_constant_record()
    , find_cached_constant()
    , compute_constant()
    , measure_constant_bytes()
    , insert_cached_constant() {}

}

TEST_F(ConstantReaderTest, ACachedValueIsReadWithoutComputing) {
    ON_CALL(find_cached_constant, find(0)).WillByDefault(Return(three_cached));
    EXPECT_CALL(compute_constant, compute(_, _)).Times(0);
    EXPECT_CALL(insert_cached_constant, insert(_, _, _)).Times(0);
    EXPECT_EQ(reader.read(0), three_cached);
}

TEST_F(ConstantReaderTest, AMissingAtomIsComputedAndCachedWithItsBytes) {
    EXPECT_CALL(compute_constant, compute(_, _)).Times(AnyNumber());
    EXPECT_CALL(compute_constant, compute(three.origin, ElementsAre())).WillOnce(Return(std::any(3)));
    EXPECT_CALL(insert_cached_constant, insert(0, HoldsInteger(3), 4));
    EXPECT_THAT(reader.read(0), HoldsInteger(3));
}

TEST_F(ConstantReaderTest, TheValueTakesTheConstantsType) {
    EXPECT_EQ(reader.read(2)->type, std::type_index(typeid(bool)));
    EXPECT_EQ(reader.read(1)->type, std::type_index(typeid(int)));
}

TEST_F(ConstantReaderTest, AMissingApplicationReadsItsArgumentsAsScalarsInOrder) {
    ON_CALL(find_cached_constant, find(0)).WillByDefault(Return(three_cached));
    EXPECT_CALL(compute_constant, compute(_, _)).Times(AnyNumber());
    EXPECT_CALL(compute_constant, compute(square.origin, ElementsAre(
        operand_data{.data = &three_cached->cell, .scalar = true},
        operand_data{.data = &three_cached->cell, .scalar = true})))
        .WillOnce(Return(std::any(9)));
    EXPECT_THAT(reader.read(1), HoldsInteger(9));
}

TEST_F(ConstantReaderTest, AMissingArgumentIsComputedAndCachedToo) {
    EXPECT_CALL(insert_cached_constant, insert(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(insert_cached_constant, insert(0, HoldsInteger(3), 4)).Times(2);
    EXPECT_CALL(insert_cached_constant, insert(1, HoldsInteger(9), 4));
    reader.read(1);
}
