#include <optional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/set_mover.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

using pool_t = std::unordered_map<std::type_index, std::vector<feature_id>>;

struct MockFindDroppableMembers {
    MOCK_METHOD(std::vector<size_t>, find, (const std::vector<feature_id>&));
};

struct MockFindAncestry {
    MOCK_METHOD(std::vector<feature_id>, find, (const std::vector<feature_id>&), (const));
};

struct MockFindOperandPool {
    MOCK_METHOD(pool_t, find, (const std::vector<feature_id>&), (const));
};

struct MockFindEligiblePrimitives {
    MOCK_METHOD(std::vector<prim_id>, find, (const pool_t&), (const));
};

struct MockFindFillablePrimitives {
    MOCK_METHOD(std::vector<prim_id>, find, (const pool_t&, const std::vector<prim_id>&), (const));
};

struct MockChooseOption {
    MOCK_METHOD(size_t, choose, (size_t));
};

struct MockChooseApplication {
    MOCK_METHOD(derivation, choose, (const pool_t&, const std::vector<prim_id>&));
};

struct MockDeriveFeature {
    MOCK_METHOD(std::optional<feature_id>, derive, (const derivation&));
};

using test_set_mover_t = set_mover<
    NiceMock<MockFindDroppableMembers>, NiceMock<MockFindAncestry>, NiceMock<MockFindOperandPool>,
    NiceMock<MockFindEligiblePrimitives>, NiceMock<MockFindFillablePrimitives>, NiceMock<MockChooseOption>,
    NiceMock<MockChooseApplication>, NiceMock<MockDeriveFeature>>;

// The set {2, 5, 9} has ancestry {1, 2, 5, 9}, whose pool is ints {1, 2, 5, 9}; primitive 3 is eligible, and it is
// fillable unless a test says otherwise. Members 0 and 2 are droppable unless a test says otherwise.
struct SetMoverTest : public ::testing::Test {
    SetMoverTest();
    std::vector<feature_id> members;
    pool_t pool;
    derivation application;
    NiceMock<MockFindDroppableMembers> find_droppable_members;
    NiceMock<MockFindAncestry> find_ancestry;
    NiceMock<MockFindOperandPool> find_operand_pool;
    NiceMock<MockFindEligiblePrimitives> find_eligible_primitives;
    NiceMock<MockFindFillablePrimitives> find_fillable_primitives;
    NiceMock<MockChooseOption> choose_option;
    NiceMock<MockChooseApplication> choose_application;
    NiceMock<MockDeriveFeature> derive_feature;
    test_set_mover_t mover{
        find_droppable_members, find_ancestry, find_operand_pool, find_eligible_primitives, find_fillable_primitives,
        choose_option, choose_application, derive_feature};

    void SetUp() override {
        ON_CALL(find_droppable_members, find(members)).WillByDefault(Return(std::vector<size_t>{0, 2}));
        ON_CALL(find_ancestry, find(members)).WillByDefault(Return(std::vector<feature_id>{1, 2, 5, 9}));
        ON_CALL(find_operand_pool, find(std::vector<feature_id>{1, 2, 5, 9})).WillByDefault(Return(pool));
        ON_CALL(find_eligible_primitives, find(pool)).WillByDefault(Return(std::vector<prim_id>{3}));
        ON_CALL(find_fillable_primitives, find(pool, std::vector<prim_id>{3}))
            .WillByDefault(Return(std::vector<prim_id>{3}));
        ON_CALL(choose_application, choose(pool, std::vector<prim_id>{3})).WillByDefault(Return(application));
    }
};

SetMoverTest::SetMoverTest()
    : members{2, 5, 9}
    , pool{{typeid(int), {1, 2, 5, 9}}}
    , application{derivation::application{.prim = 3, .args = {operand{operand::from_feature{.id = 2}}}}}
    , find_droppable_members()
    , find_ancestry()
    , find_operand_pool()
    , find_eligible_primitives()
    , find_fillable_primitives()
    , choose_option()
    , choose_application()
    , derive_feature() {}

}

TEST_F(SetMoverTest, OptionZeroDropsTheChosenDroppableMember) {
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    InSequence order;
    EXPECT_CALL(choose_option, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_option, choose(2)).WillOnce(Return(1));
    EXPECT_EQ(mover.move(members), (std::vector<feature_id>{2, 5}));
}

TEST_F(SetMoverTest, OptionOneAddsTheDerivedFeatureInOrder) {
    ON_CALL(choose_option, choose(2)).WillByDefault(Return(1));
    ON_CALL(derive_feature, derive(application)).WillByDefault(Return(7));
    EXPECT_EQ(mover.move(members), (std::vector<feature_id>{2, 5, 7, 9}));
}

TEST_F(SetMoverTest, AnAdditionDerivingNoFeatureLeavesTheSet) {
    ON_CALL(choose_option, choose(2)).WillByDefault(Return(1));
    ON_CALL(derive_feature, derive(application)).WillByDefault(Return(std::nullopt));
    EXPECT_EQ(mover.move(members), members);
}

TEST_F(SetMoverTest, AnAdditionDerivingAMemberLeavesTheSet) {
    ON_CALL(choose_option, choose(2)).WillByDefault(Return(1));
    ON_CALL(derive_feature, derive(application)).WillByDefault(Return(5));
    EXPECT_EQ(mover.move(members), members);
}

TEST_F(SetMoverTest, AFeatureBelowEveryMemberIsAddedFirst) {
    ON_CALL(choose_option, choose(2)).WillByDefault(Return(1));
    ON_CALL(derive_feature, derive(application)).WillByDefault(Return(0));
    EXPECT_EQ(mover.move(members), (std::vector<feature_id>{0, 2, 5, 9}));
}

TEST_F(SetMoverTest, WithNothingDroppableAnAdditionIsTakenWithoutAKindChoice) {
    ON_CALL(find_droppable_members, find(members)).WillByDefault(Return(std::vector<size_t>{}));
    ON_CALL(derive_feature, derive(application)).WillByDefault(Return(11));
    EXPECT_CALL(choose_option, choose(_)).Times(0);
    EXPECT_EQ(mover.move(members), (std::vector<feature_id>{2, 5, 9, 11}));
}

TEST_F(SetMoverTest, WithNothingFillableADropIsTakenWithoutAKindChoice) {
    ON_CALL(find_fillable_primitives, find(pool, std::vector<prim_id>{3}))
        .WillByDefault(Return(std::vector<prim_id>{}));
    EXPECT_CALL(choose_option, choose(_)).Times(AnyNumber());
    EXPECT_CALL(choose_option, choose(2)).WillOnce(Return(0));
    EXPECT_CALL(choose_application, choose(_, _)).Times(0);
    EXPECT_EQ(mover.move(members), (std::vector<feature_id>{5, 9}));
}

TEST_F(SetMoverTest, WithNeitherThereIsNoMove) {
    ON_CALL(find_droppable_members, find(members)).WillByDefault(Return(std::vector<size_t>{}));
    ON_CALL(find_fillable_primitives, find(pool, std::vector<prim_id>{3}))
        .WillByDefault(Return(std::vector<prim_id>{}));
    EXPECT_FALSE(mover.move(members).has_value());
}

TEST_F(SetMoverTest, TheLoneMemberOfASetCanBeDropped) {
    const std::vector<feature_id> lone{4};
    ON_CALL(find_droppable_members, find(lone)).WillByDefault(Return(std::vector<size_t>{0}));
    ON_CALL(choose_option, choose(1)).WillByDefault(Return(0));
    EXPECT_EQ(mover.move(lone), (std::vector<feature_id>{}));
}
