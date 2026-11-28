#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/adding_expander.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

std::vector<operand> feature_args(const std::vector<feature_id>& ids) {
    std::vector<operand> args;
    for (const feature_id id : ids)
        args.push_back(operand{operand::from_feature{.id = id}});
    return args;
}

using pool_t = std::unordered_map<std::type_index, std::vector<feature_id>>;

struct MockFindAncestry {
    MOCK_METHOD(std::vector<feature_id>, find, (const std::vector<feature_id>&), (const));
};

struct MockFindOperandPool {
    MOCK_METHOD(pool_t, find, (const std::vector<feature_id>&), (const));
};

struct MockFindEligiblePrimitives {
    MOCK_METHOD(std::vector<prim_id>, find, (const pool_t&), (const));
};

struct MockSampleDerivation {
    MOCK_METHOD(std::optional<derivation>, sample, (const pool_t&, const std::vector<prim_id>&));
};

struct MockDeriveFeature {
    MOCK_METHOD(std::optional<feature_id>, derive, (const derivation&));
};

struct MockPushChild {
    MOCK_METHOD(void, push, (std::vector<feature_id>));
};

using test_adding_expander_t = adding_expander<
    NiceMock<MockFindAncestry>, NiceMock<MockFindOperandPool>, NiceMock<MockFindEligiblePrimitives>,
    NiceMock<MockSampleDerivation>, NiceMock<MockDeriveFeature>, NiceMock<MockPushChild>>;

// The set {4, 7} has the ancestry {1, 4, 7}, whose pool feeds primitives 0 and 3; two samples per feature make six.
struct AddingExpanderTest : public ::testing::Test {
    AddingExpanderTest();
    derivation origin;
    pool_t pool;
    std::vector<prim_id> eligible;
    NiceMock<MockFindAncestry> find_ancestry;
    NiceMock<MockFindOperandPool> find_operand_pool;
    NiceMock<MockFindEligiblePrimitives> find_eligible_primitives;
    NiceMock<MockSampleDerivation> sample_derivation;
    NiceMock<MockDeriveFeature> derive_feature;
    NiceMock<MockPushChild> push_child;
    test_adding_expander_t expander{
        find_ancestry, find_operand_pool, find_eligible_primitives, sample_derivation, derive_feature, push_child, 2};

    void SetUp() override {
        ON_CALL(find_ancestry, find(std::vector<feature_id>{4, 7}))
            .WillByDefault(Return(std::vector<feature_id>{1, 4, 7}));
        ON_CALL(find_operand_pool, find(std::vector<feature_id>{1, 4, 7})).WillByDefault(Return(pool));
        ON_CALL(find_eligible_primitives, find(pool)).WillByDefault(Return(eligible));
        ON_CALL(sample_derivation, sample(pool, eligible)).WillByDefault(Return(std::optional<derivation>(origin)));
        ON_CALL(derive_feature, derive(origin)).WillByDefault(Return(std::optional<feature_id>(12)));
    }
};

AddingExpanderTest::AddingExpanderTest()
    : origin{derivation::application{.prim = 0, .args = feature_args({1, 4})}}
    , pool{{typeid(int), {1, 4, 7}}}
    , eligible{0, 3}
    , find_ancestry()
    , find_operand_pool()
    , find_eligible_primitives()
    , sample_derivation()
    , derive_feature()
    , push_child() {}

}

TEST_F(AddingExpanderTest, EachFeatureInTheAncestryAllowsItsShareOfSamples) {
    EXPECT_CALL(sample_derivation, sample(pool, eligible))
        .Times(6)
        .WillRepeatedly(Return(std::optional<derivation>(origin)));
    expander.expand({4, 7});
}

TEST_F(AddingExpanderTest, ADerivedFeatureIsPushedAddedToTheSet) {
    EXPECT_CALL(push_child, push(std::vector<feature_id>{4, 7, 12})).Times(6);
    expander.expand({4, 7});
}

TEST_F(AddingExpanderTest, ASampleWithNoApplicationIsSpent) {
    ON_CALL(sample_derivation, sample(_, _)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(sample_derivation, sample(_, _)).Times(6);
    EXPECT_CALL(derive_feature, derive(_)).Times(0);
    EXPECT_CALL(push_child, push(_)).Times(0);
    expander.expand({4, 7});
}

TEST_F(AddingExpanderTest, AnApplicationDerivingNoFeatureIsSpent) {
    ON_CALL(derive_feature, derive(_)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(push_child, push(_)).Times(0);
    expander.expand({4, 7});
}

TEST_F(AddingExpanderTest, AFeatureTheSetHoldsIsNotAdded) {
    ON_CALL(derive_feature, derive(_)).WillByDefault(Return(std::optional<feature_id>(7)));
    EXPECT_CALL(push_child, push(_)).Times(0);
    expander.expand({4, 7});
}

TEST_F(AddingExpanderTest, AnAncestorTheSetDoesNotHoldIsAdded) {
    ON_CALL(derive_feature, derive(_)).WillByDefault(Return(std::optional<feature_id>(1)));
    EXPECT_CALL(push_child, push(std::vector<feature_id>{4, 7, 1})).Times(6);
    expander.expand({4, 7});
}

TEST_F(AddingExpanderTest, AnEmptySetHasNoAncestryAndSamplesNothing) {
    EXPECT_CALL(sample_derivation, sample(_, _)).Times(0);
    expander.expand({});
}
