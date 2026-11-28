#include <any>
#include <memory>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "value_objects/cached_constant.hpp"
#include "infrastructure/column_reader.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetFeatureRecord {
    MOCK_METHOD(const feature_record&, record, (feature_id), (const));
};

struct MockReadConstant {
    MOCK_METHOD(std::shared_ptr<const cached_constant>, read, (constant_id));
};

struct MockFindCachedColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, find, (feature_id));
};

struct MockComputeColumn {
    MOCK_METHOD(std::any, compute, (const derivation&, const std::vector<operand_data>&), (const));
};

struct MockMakeCachedColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, make, (std::type_index, std::any), (const));
};

struct MockMeasureColumnBytes {
    MOCK_METHOD(size_t, measure, (const cached_column&), (const));
};

struct MockInsertCachedColumn {
    MOCK_METHOD(void, insert, (feature_id, std::shared_ptr<const cached_column>, size_t));
};

using test_column_reader_t = column_reader<
    NiceMock<MockGetFeatureRecord>, NiceMock<MockReadConstant>, NiceMock<MockFindCachedColumn>,
    NiceMock<MockComputeColumn>, NiceMock<MockMakeCachedColumn>, NiceMock<MockMeasureColumnBytes>,
    NiceMock<MockInsertCachedColumn>>;

// Feature 0 is a boolean input; feature 1, an integer, applies primitive 3 to feature 0, constant 4 and feature 0.
struct ColumnReaderTest : public ::testing::Test {
    ColumnReaderTest();
    std::shared_ptr<const cached_column> cached_of(std::type_index type, std::any cells);
    feature_record input;
    feature_record application;
    std::shared_ptr<const cached_constant> four;
    std::shared_ptr<const cached_column> input_column;
    std::shared_ptr<const cached_column> application_column;
    NiceMock<MockGetFeatureRecord> get_feature_record;
    NiceMock<MockReadConstant> read_constant;
    NiceMock<MockFindCachedColumn> find_cached_column;
    NiceMock<MockComputeColumn> compute_column;
    NiceMock<MockMakeCachedColumn> make_cached_column;
    NiceMock<MockMeasureColumnBytes> measure_column_bytes;
    NiceMock<MockInsertCachedColumn> insert_cached_column;
    test_column_reader_t reader{
        get_feature_record, read_constant, find_cached_column, compute_column, make_cached_column,
        measure_column_bytes, insert_cached_column};

    void SetUp() override {
        ON_CALL(get_feature_record, record(0)).WillByDefault(ReturnRef(input));
        ON_CALL(get_feature_record, record(1)).WillByDefault(ReturnRef(application));
        ON_CALL(read_constant, read(4)).WillByDefault(Return(four));
        ON_CALL(make_cached_column, make(std::type_index(typeid(bool)), _)).WillByDefault(Return(input_column));
        ON_CALL(make_cached_column, make(std::type_index(typeid(int)), _))
            .WillByDefault(Return(application_column));
        ON_CALL(measure_column_bytes, measure(_)).WillByDefault(Return(16));
    }
};

ColumnReaderTest::ColumnReaderTest()
    : input{.origin = derivation{derivation::input{.index = 0}}, .type = typeid(bool), .key = 1}
    , application{
          .origin = derivation{derivation::application{
              .prim = 3,
              .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_constant{.id = 4}},
                       operand{operand::from_feature{.id = 0}}}}},
          .type = typeid(int),
          .key = 2}
    , four(std::make_shared<const cached_constant>(cached_constant{.type = typeid(int), .cell = std::any(4)}))
    , input_column(cached_of(typeid(bool), std::any(std::vector<bool>{false, true})))
    , application_column(cached_of(typeid(int), std::any(std::vector<int>{1, 1})))
    , get_feature_record()
    , read_constant()
    , find_cached_column()
    , compute_column()
    , make_cached_column()
    , measure_column_bytes()
    , insert_cached_column() {}

std::shared_ptr<const cached_column> ColumnReaderTest::cached_of(std::type_index type, std::any cells) {
    return std::make_shared<const cached_column>(
        cached_column{.type = type, .cells = std::move(cells), .classes = {0, 1}, .hash = 0});
}

}

TEST_F(ColumnReaderTest, ACachedColumnIsReadWithoutComputing) {
    ON_CALL(find_cached_column, find(1)).WillByDefault(Return(application_column));
    EXPECT_CALL(compute_column, compute(_, _)).Times(0);
    EXPECT_CALL(insert_cached_column, insert(_, _, _)).Times(0);
    EXPECT_EQ(reader.read(1), application_column);
}

TEST_F(ColumnReaderTest, AMissingInputIsComputedAndCachedWithItsBytes) {
    EXPECT_CALL(compute_column, compute(_, _)).Times(AnyNumber());
    EXPECT_CALL(compute_column, compute(input.origin, ElementsAre())).WillOnce(Return(std::any()));
    EXPECT_CALL(insert_cached_column, insert(0, input_column, 16));
    EXPECT_EQ(reader.read(0), input_column);
}

TEST_F(ColumnReaderTest, AMissingApplicationReadsFeatureColumnsAndConstantScalarsInArgumentOrder) {
    ON_CALL(find_cached_column, find(0)).WillByDefault(Return(input_column));
    EXPECT_CALL(compute_column, compute(_, _)).Times(AnyNumber());
    EXPECT_CALL(compute_column, compute(application.origin, ElementsAre(
        operand_data{.data = &input_column->cells, .scalar = false},
        operand_data{.data = &four->cell, .scalar = true},
        operand_data{.data = &input_column->cells, .scalar = false})))
        .WillOnce(Return(std::any()));
    EXPECT_EQ(reader.read(1), application_column);
}

TEST_F(ColumnReaderTest, AMissingOperandIsComputedAndCachedToo) {
    EXPECT_CALL(insert_cached_column, insert(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(insert_cached_column, insert(0, input_column, 16)).Times(2);
    EXPECT_CALL(insert_cached_column, insert(1, application_column, 16));
    EXPECT_EQ(reader.read(1), application_column);
}

TEST_F(ColumnReaderTest, TheColumnIsMadeWithTheFeaturesType) {
    ON_CALL(find_cached_column, find(0)).WillByDefault(Return(input_column));
    EXPECT_CALL(make_cached_column, make(_, _)).Times(AnyNumber());
    EXPECT_CALL(make_cached_column, make(std::type_index(typeid(int)), _)).WillOnce(Return(application_column));
    reader.read(1);
}
