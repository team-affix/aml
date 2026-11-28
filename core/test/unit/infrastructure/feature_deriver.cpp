#include <any>
#include <memory>
#include <optional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "value_objects/cached_constant.hpp"
#include "infrastructure/feature_deriver.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockFindDerivation {
    MOCK_METHOD(std::optional<feature_id>, find, (const derivation&), (const));
};

struct MockReadColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, read, (feature_id));
};

struct MockReadConstant {
    MOCK_METHOD(std::shared_ptr<const cached_constant>, read, (constant_id));
};

struct MockComputeColumn {
    MOCK_METHOD(std::any, compute, (const derivation&, const std::vector<operand_data>&), (const));
};

struct MockGetNativePrimitive {
    MOCK_METHOD(const native_primitive&, primitive, (prim_id), (const));
};

struct MockMakeCachedColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, make, (std::type_index, std::any), (const));
};

struct MockDetectUniformColumn {
    MOCK_METHOD(bool, detect, (const cached_column&), (const));
};

struct MockFindEqualFeature {
    MOCK_METHOD(std::optional<feature_id>, find, (const cached_column&));
};

struct MockTryReplaceOrigin {
    MOCK_METHOD(void, try_replace, (feature_id, const derivation&));
};

struct MockInsertDerivation {
    MOCK_METHOD(void, insert, (const derivation&, feature_id));
};

struct MockRecordFeature {
    MOCK_METHOD(feature_id, record, (const derivation&, std::shared_ptr<const cached_column>));
};

using test_feature_deriver_t = feature_deriver<
    NiceMock<MockFindDerivation>, NiceMock<MockReadColumn>, NiceMock<MockReadConstant>,
    NiceMock<MockComputeColumn>, NiceMock<MockGetNativePrimitive>, NiceMock<MockMakeCachedColumn>,
    NiceMock<MockDetectUniformColumn>,
    NiceMock<MockFindEqualFeature>, NiceMock<MockTryReplaceOrigin>, NiceMock<MockInsertDerivation>,
    NiceMock<MockRecordFeature>>;

// The origin applies primitive 2, which gives an int, to feature 0, constant 6 and feature 1.
struct FeatureDeriverTest : public ::testing::Test {
    FeatureDeriverTest();
    std::shared_ptr<const cached_column> integers_of(std::vector<int> cells);
    derivation origin;
    native_primitive signature;
    std::shared_ptr<const cached_constant> six;
    std::shared_ptr<const cached_column> first;
    std::shared_ptr<const cached_column> second;
    std::shared_ptr<const cached_column> made;
    NiceMock<MockFindDerivation> find_derivation;
    NiceMock<MockReadColumn> read_column;
    NiceMock<MockReadConstant> read_constant;
    NiceMock<MockComputeColumn> compute_column;
    NiceMock<MockGetNativePrimitive> get_native_primitive;
    NiceMock<MockMakeCachedColumn> make_cached_column;
    NiceMock<MockDetectUniformColumn> detect_uniform_column;
    NiceMock<MockFindEqualFeature> find_equal_feature;
    NiceMock<MockTryReplaceOrigin> try_replace_origin;
    NiceMock<MockInsertDerivation> insert_derivation;
    NiceMock<MockRecordFeature> record_feature;
    test_feature_deriver_t deriver{
        find_derivation, read_column, read_constant, compute_column, get_native_primitive, make_cached_column,
        detect_uniform_column, find_equal_feature, try_replace_origin, insert_derivation, record_feature};

    void SetUp() override {
        ON_CALL(read_column, read(0)).WillByDefault(Return(first));
        ON_CALL(read_column, read(1)).WillByDefault(Return(second));
        ON_CALL(read_constant, read(6)).WillByDefault(Return(six));
        ON_CALL(get_native_primitive, primitive(2)).WillByDefault(ReturnRef(signature));
        ON_CALL(make_cached_column, make(std::type_index(typeid(int)), _)).WillByDefault(Return(made));
        ON_CALL(record_feature, record(_, _)).WillByDefault(Return(8));
    }
};

FeatureDeriverTest::FeatureDeriverTest()
    : origin{derivation::application{
          .prim = 2,
          .args = {operand{operand::from_feature{.id = 0}}, operand{operand::from_constant{.id = 6}},
                   operand{operand::from_feature{.id = 1}}}}}
    , signature{
          .arg_types = {typeid(int), typeid(int), typeid(int)}, .result_type = typeid(int), .apply = nullptr}
    , six(std::make_shared<const cached_constant>(cached_constant{.type = typeid(int), .cell = std::any(6)}))
    , first(integers_of({1, 2}))
    , second(integers_of({3, 3}))
    , made(integers_of({4, 5}))
    , find_derivation()
    , read_column()
    , read_constant()
    , compute_column()
    , get_native_primitive()
    , make_cached_column()
    , detect_uniform_column()
    , find_equal_feature()
    , try_replace_origin()
    , insert_derivation()
    , record_feature() {}

std::shared_ptr<const cached_column> FeatureDeriverTest::integers_of(std::vector<int> cells) {
    return std::make_shared<const cached_column>(
        cached_column{.type = typeid(int), .cells = std::any(std::move(cells)), .classes = {0, 1}, .hash = 0});
}

}

TEST_F(FeatureDeriverTest, ADerivationMadeBeforeGivesItsFeatureWithoutComputing) {
    ON_CALL(find_derivation, find(origin)).WillByDefault(Return(std::optional<feature_id>(3)));
    EXPECT_CALL(compute_column, compute(_, _)).Times(0);
    EXPECT_EQ(deriver.derive(origin), std::optional<feature_id>(3));
}

TEST_F(FeatureDeriverTest, TheColumnIsComputedFromColumnsAndConstantScalarsInArgumentOrder) {
    EXPECT_CALL(compute_column, compute(_, _)).Times(AnyNumber());
    EXPECT_CALL(compute_column, compute(origin, ElementsAre(
        operand_data{.data = &first->cells, .scalar = false}, operand_data{.data = &six->cell, .scalar = true},
        operand_data{.data = &second->cells, .scalar = false})))
        .WillOnce(Return(std::any()));
    deriver.derive(origin);
}

TEST_F(FeatureDeriverTest, ANewColumnBecomesANewFeatureOfThePrimitivesResultType) {
    EXPECT_CALL(make_cached_column, make(_, _)).Times(AnyNumber());
    EXPECT_CALL(make_cached_column, make(std::type_index(typeid(int)), _)).WillOnce(Return(made));
    EXPECT_CALL(record_feature, record(origin, made)).WillOnce(Return(8));
    EXPECT_EQ(deriver.derive(origin), std::optional<feature_id>(8));
}

TEST_F(FeatureDeriverTest, AUniformColumnGivesNoFeature) {
    ON_CALL(detect_uniform_column, detect(_)).WillByDefault(Return(true));
    EXPECT_CALL(record_feature, record(_, _)).Times(0);
    EXPECT_CALL(insert_derivation, insert(_, _)).Times(0);
    EXPECT_EQ(deriver.derive(origin), std::nullopt);
}

TEST_F(FeatureDeriverTest, AColumnEqualToAFeaturesGivesThatFeature) {
    ON_CALL(find_equal_feature, find(_)).WillByDefault(Return(std::optional<feature_id>(4)));
    EXPECT_CALL(record_feature, record(_, _)).Times(0);
    EXPECT_EQ(deriver.derive(origin), std::optional<feature_id>(4));
}

TEST_F(FeatureDeriverTest, AnEqualFeatureMayTakeTheApplicationAsItsOrigin) {
    ON_CALL(find_equal_feature, find(_)).WillByDefault(Return(std::optional<feature_id>(4)));
    EXPECT_CALL(try_replace_origin, try_replace(4, origin));
    deriver.derive(origin);
}

TEST_F(FeatureDeriverTest, AnEqualFeatureIndexesTheDerivationSoItIsNotComputedAgain) {
    ON_CALL(find_equal_feature, find(_)).WillByDefault(Return(std::optional<feature_id>(4)));
    EXPECT_CALL(insert_derivation, insert(origin, 4));
    deriver.derive(origin);
}

TEST_F(FeatureDeriverTest, ANewFeatureIsIndexedByTheRecorderAlone) {
    EXPECT_CALL(insert_derivation, insert(_, _)).Times(0);
    EXPECT_CALL(try_replace_origin, try_replace(_, _)).Times(0);
    deriver.derive(origin);
}
