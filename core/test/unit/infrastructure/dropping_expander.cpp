#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dropping_expander.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindDroppableMembers {
    MOCK_METHOD(std::vector<size_t>, find, (const std::vector<feature_id>&));
};

struct MockPushChild {
    MOCK_METHOD(void, push, (std::vector<feature_id>));
};

struct DroppingExpanderTest : public ::testing::Test {
    NiceMock<MockFindDroppableMembers> find_droppable_members;
    NiceMock<MockPushChild> push_child;
    dropping_expander<NiceMock<MockFindDroppableMembers>, NiceMock<MockPushChild>> expander{
        find_droppable_members, push_child};
};

}

TEST_F(DroppingExpanderTest, EachDroppableMemberGivesOneChildWithoutIt) {
    ON_CALL(find_droppable_members, find(std::vector<feature_id>{4, 7, 9}))
        .WillByDefault(Return(std::vector<size_t>{0, 2}));
    InSequence order;
    EXPECT_CALL(push_child, push(std::vector<feature_id>{7, 9}));
    EXPECT_CALL(push_child, push(std::vector<feature_id>{4, 7}));
    expander.expand({4, 7, 9});
}

TEST_F(DroppingExpanderTest, NothingDroppablePushesNothing) {
    EXPECT_CALL(push_child, push(_)).Times(0);
    expander.expand({4, 7, 9});
}

TEST_F(DroppingExpanderTest, ALoneDroppableMemberLeavesTheEmptySet) {
    ON_CALL(find_droppable_members, find(std::vector<feature_id>{4})).WillByDefault(Return(std::vector<size_t>{0}));
    EXPECT_CALL(push_child, push(std::vector<feature_id>{}));
    expander.expand({4});
}
