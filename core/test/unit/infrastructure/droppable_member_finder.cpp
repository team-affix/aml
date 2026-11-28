#include <any>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/droppable_member_finder.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockReadColumn {
    MOCK_METHOD(std::shared_ptr<const cached_column>, read, (feature_id));
};

struct MockNumberRowKeys {
    MOCK_METHOD(
        std::vector<std::vector<size_t>>, number, (const std::vector<const std::vector<size_t>*>&, size_t), (const));
};

using layers_t = std::vector<std::vector<size_t>>;

// Four rows labelled a xor b. Feature 0 is a, 1 is b, 2 is a xor b. The numberings below are the prefix keys of
// the listed columns.
struct DroppableMemberFinderTest : public ::testing::Test {
    DroppableMemberFinderTest();
    std::shared_ptr<const cached_column> classes_of(std::vector<size_t> classes);
    void number_as(const std::vector<feature_id>& order, const layers_t& layers);
    std::vector<size_t> labels;
    std::vector<std::shared_ptr<const cached_column>> columns;
    NiceMock<MockReadColumn> read_column;
    NiceMock<MockNumberRowKeys> number_row_keys;
    droppable_member_finder<NiceMock<MockReadColumn>, NiceMock<MockNumberRowKeys>> finder{
        read_column, number_row_keys, labels};

    void SetUp() override {
        for (feature_id feature = 0; feature < columns.size(); ++feature)
            ON_CALL(read_column, read(feature)).WillByDefault(Return(columns[feature]));
        number_as({}, {{0, 0, 0, 0}});
        number_as({0, 1}, {{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 2, 3}});
        number_as({1, 0}, {{0, 0, 0, 0}, {0, 1, 0, 1}, {0, 1, 2, 3}});
        number_as({0, 1, 2}, {{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 2, 3}, {0, 1, 2, 3}});
        number_as({2, 1, 0}, {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 2, 3}, {0, 1, 2, 3}});
        number_as({2}, {{0, 0, 0, 0}, {0, 1, 1, 0}});
        number_as({1}, {{0, 0, 0, 0}, {0, 1, 0, 1}});
    }
};

DroppableMemberFinderTest::DroppableMemberFinderTest()
    : labels{0, 1, 1, 0}
    , columns{classes_of({0, 0, 1, 1}), classes_of({0, 1, 0, 1}), classes_of({0, 1, 1, 0})}
    , read_column()
    , number_row_keys() {}

std::shared_ptr<const cached_column> DroppableMemberFinderTest::classes_of(std::vector<size_t> classes) {
    return std::make_shared<const cached_column>(cached_column{
        .type = typeid(int), .cells = std::any(std::vector<int>(classes.size(), 0)), .classes = classes, .hash = 0});
}

void DroppableMemberFinderTest::number_as(const std::vector<feature_id>& order, const layers_t& layers) {
    std::vector<const std::vector<size_t>*> listed;
    for (const feature_id feature : order)
        listed.push_back(&columns[feature]->classes);
    ON_CALL(number_row_keys, number(listed, 4)).WillByDefault(Return(layers));
}

}

TEST_F(DroppableMemberFinderTest, NoMembersHaveNothingToDrop) {
    EXPECT_TRUE(finder.find({}).empty());
}

TEST_F(DroppableMemberFinderTest, NeitherInputOfXorCanBeDropped) {
    EXPECT_TRUE(finder.find({0, 1}).empty());
}

TEST_F(DroppableMemberFinderTest, BesideTheXorEveryMemberCanBeDropped) {
    EXPECT_EQ(finder.find({0, 1, 2}), (std::vector<size_t>{0, 1, 2}));
}

TEST_F(DroppableMemberFinderTest, TheXorAloneCannotBeDropped) {
    EXPECT_TRUE(finder.find({2}).empty());
}

TEST_F(DroppableMemberFinderTest, UniformLabelsLetALoneMemberGo) {
    labels = {1, 1, 1, 1};
    EXPECT_EQ(finder.find({1}), (std::vector<size_t>{0}));
}

TEST_F(DroppableMemberFinderTest, PrefixesAreNumberedForwardAndSuffixesBackwardOverEveryRow) {
    EXPECT_CALL(number_row_keys, number(_, _)).Times(AnyNumber());
    EXPECT_CALL(number_row_keys, number(ElementsAre(&columns[0]->classes, &columns[1]->classes), 4))
        .WillOnce(Return(layers_t{{0, 0, 0, 0}, {0, 0, 1, 1}, {0, 1, 2, 3}}));
    EXPECT_CALL(number_row_keys, number(ElementsAre(&columns[1]->classes, &columns[0]->classes), 4))
        .WillOnce(Return(layers_t{{0, 0, 0, 0}, {0, 1, 0, 1}, {0, 1, 2, 3}}));
    finder.find({0, 1});
}

TEST_F(DroppableMemberFinderTest, AMemberGoesWhenItsPrefixAndSuffixKeysStillFixTheLabel) {
    // Dropping member 1 of three keys rows by layer 1 forward paired with layer 1 backward.
    number_as({0, 1, 2}, {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}});
    number_as({2, 1, 0}, {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}});
    EXPECT_EQ(finder.find({0, 1, 2}), (std::vector<size_t>{1}));
}
