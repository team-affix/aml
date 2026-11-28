#include <typeindex>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/operand_pool_finder.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

std::vector<operand> feature_args(const std::vector<feature_id>& ids) {
    std::vector<operand> args;
    for (const feature_id id : ids)
        args.push_back(operand{operand::from_feature{.id = id}});
    return args;
}

struct MockGetFeatureRecord {
    MOCK_METHOD(const feature_record&, record, (feature_id), (const));
};

using pool_t = std::unordered_map<std::type_index, std::vector<feature_id>>;

// 0: boolean input; 1: integer application; 2: boolean input.
struct OperandPoolFinderTest : public ::testing::Test {
    OperandPoolFinderTest();
    std::vector<feature_record> records;
    NiceMock<MockGetFeatureRecord> get_feature_record;
    operand_pool_finder<NiceMock<MockGetFeatureRecord>> finder{get_feature_record};

    void SetUp() override {
        for (feature_id feature = 0; feature < records.size(); ++feature)
            ON_CALL(get_feature_record, record(feature)).WillByDefault(ReturnRef(records[feature]));
    }
};

OperandPoolFinderTest::OperandPoolFinderTest()
    : records{
          feature_record{.origin = derivation{derivation::input{.index = 0}}, .type = typeid(bool), .key = 0},
          feature_record{
              .origin = derivation{derivation::application{.prim = 0, .args = feature_args({0})}}, .type = typeid(int),
              .key = 0},
          feature_record{.origin = derivation{derivation::input{.index = 1}}, .type = typeid(bool), .key = 0}}
    , get_feature_record() {}

}

TEST_F(OperandPoolFinderTest, AnEmptyAncestryOffersNothing) {
    EXPECT_TRUE(finder.find({}).empty());
}

TEST_F(OperandPoolFinderTest, FeaturesAreListedByTypeInAncestryOrder) {
    EXPECT_EQ(finder.find({0, 1, 2}), (pool_t{{typeid(int), {1}}, {typeid(bool), {0, 2}}}));
}

TEST_F(OperandPoolFinderTest, ATypeTheAncestryLacksIsAbsent) {
    EXPECT_EQ(finder.find({1}), (pool_t{{typeid(int), {1}}}));
}
