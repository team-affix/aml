#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/ancestry_finder.hpp"

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

// 0 and 1 are inputs, 2 a constant; 3 = p(0, 2), 4 = p(3, 1), 5 = p(3, 3).
struct AncestryFinderTest : public ::testing::Test {
    AncestryFinderTest();
    feature_record application(std::vector<feature_id> args);
    std::vector<feature_record> records;
    NiceMock<MockGetFeatureRecord> get_feature_record;
    ancestry_finder<NiceMock<MockGetFeatureRecord>> finder{get_feature_record};

    void SetUp() override {
        for (feature_id feature = 0; feature < records.size(); ++feature)
            ON_CALL(get_feature_record, record(feature)).WillByDefault(ReturnRef(records[feature]));
    }
};

AncestryFinderTest::AncestryFinderTest()
    : records{
          feature_record{.origin = derivation{derivation::input{.index = 0}}, .type = typeid(int), .key = 0},
          feature_record{.origin = derivation{derivation::input{.index = 1}}, .type = typeid(int), .key = 0},
          feature_record{.origin = derivation{derivation::input{.index = 2}}, .type = typeid(int), .key = 0},
          application({0, 2}),
          application({3, 1}),
          application({3, 3})}
    , get_feature_record() {}

feature_record AncestryFinderTest::application(std::vector<feature_id> args) {
    return feature_record{
        .origin = derivation{derivation::application{.prim = 0, .args = feature_args(args)}}, .type = typeid(int),
        .key = 0};
}

}

TEST_F(AncestryFinderTest, ConstantOperandsAreNotFollowed) {
    const feature_record with_constant{
        .origin = derivation{derivation::application{
            .prim = 0, .args = {operand{operand::from_constant{.id = 3}}, operand{operand::from_feature{.id = 1}}}}},
        .type = typeid(int),
        .key = 0};
    ON_CALL(get_feature_record, record(9)).WillByDefault(ReturnRef(with_constant));
    EXPECT_EQ(finder.find({9}), (std::vector<feature_id>{1, 9}));
}

TEST_F(AncestryFinderTest, NoRootsHaveNoAncestry) {
    EXPECT_TRUE(finder.find({}).empty());
}

TEST_F(AncestryFinderTest, AnInputIsItsOwnAncestry) {
    EXPECT_EQ(finder.find({1}), (std::vector<feature_id>{1}));
}

TEST_F(AncestryFinderTest, AnApplicationReachesEveryArgumentTransitively) {
    EXPECT_EQ(finder.find({4}), (std::vector<feature_id>{0, 1, 2, 3, 4}));
}

TEST_F(AncestryFinderTest, ASharedAncestorIsListedOnce) {
    EXPECT_EQ(finder.find({4, 5}), (std::vector<feature_id>{0, 1, 2, 3, 4, 5}));
}

TEST_F(AncestryFinderTest, AnArgumentRepeatedInOneApplicationIsListedOnce) {
    EXPECT_EQ(finder.find({5}), (std::vector<feature_id>{0, 2, 3, 5}));
}

TEST_F(AncestryFinderTest, ARootThatIsAnotherRootsAncestorIsListedOnce) {
    EXPECT_EQ(finder.find({3, 0}), (std::vector<feature_id>{0, 2, 3}));
}

TEST_F(AncestryFinderTest, TheAncestryIsInIncreasingIdWhateverTheRootsOrder) {
    EXPECT_EQ(finder.find({1, 0}), (std::vector<feature_id>{0, 1}));
}
