#include <any>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/constant_ancestry_finder.hpp"

using namespace AML;

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetConstantRecord {
    MOCK_METHOD(const constant_record&, record, (constant_id), (const));
};

// 0 and 1 are atoms; 2 = p(0, 0); 3 = p(2, 1).
struct ConstantAncestryFinderTest : public ::testing::Test {
    ConstantAncestryFinderTest();
    constant_record atom(size_t index);
    constant_record application(std::vector<constant_id> args);
    std::vector<constant_record> records;
    NiceMock<MockGetConstantRecord> get_constant_record;
    constant_ancestry_finder<NiceMock<MockGetConstantRecord>> finder{get_constant_record};

    void SetUp() override {
        for (constant_id constant = 0; constant < records.size(); ++constant)
            ON_CALL(get_constant_record, record(constant)).WillByDefault(ReturnRef(records[constant]));
    }
};

ConstantAncestryFinderTest::ConstantAncestryFinderTest()
    : records{atom(0), atom(1), application({0, 0}), application({2, 1})}
    , get_constant_record() {}

constant_record ConstantAncestryFinderTest::atom(size_t index) {
    return constant_record{
        .origin = constant_derivation{constant_derivation::atom{.index = index}}, .type = typeid(int)};
}

constant_record ConstantAncestryFinderTest::application(std::vector<constant_id> args) {
    return constant_record{
        .origin = constant_derivation{constant_derivation::application{.prim = 0, .args = std::move(args)}},
        .type = typeid(int)};
}

}

TEST_F(ConstantAncestryFinderTest, NoRootsHaveNoAncestry) {
    EXPECT_TRUE(finder.find({}).empty());
}

TEST_F(ConstantAncestryFinderTest, AnAtomIsItsOwnAncestry) {
    EXPECT_EQ(finder.find({1}), (std::vector<constant_id>{1}));
}

TEST_F(ConstantAncestryFinderTest, AnApplicationReachesEveryArgumentTransitivelyOnce) {
    EXPECT_EQ(finder.find({3}), (std::vector<constant_id>{0, 1, 2, 3}));
}

TEST_F(ConstantAncestryFinderTest, RepeatedRootsAreListedOnce) {
    EXPECT_EQ(finder.find({2, 0, 2}), (std::vector<constant_id>{0, 2}));
}
