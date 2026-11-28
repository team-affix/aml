#include <any>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/distinct_row_counter.hpp"

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

struct DistinctRowCounterTest : public ::testing::Test {
    DistinctRowCounterTest();
    std::shared_ptr<const cached_column> classes_of(std::vector<size_t> classes);
    std::shared_ptr<const cached_column> first;
    std::shared_ptr<const cached_column> second;
    NiceMock<MockReadColumn> read_column;
    NiceMock<MockNumberRowKeys> number_row_keys;
    distinct_row_counter<NiceMock<MockReadColumn>, NiceMock<MockNumberRowKeys>> counter{
        read_column, number_row_keys, 4};

    void SetUp() override {
        ON_CALL(read_column, read(7)).WillByDefault(Return(first));
        ON_CALL(read_column, read(9)).WillByDefault(Return(second));
    }
};

DistinctRowCounterTest::DistinctRowCounterTest()
    : first(classes_of({0, 0, 1, 1}))
    , second(classes_of({0, 1, 0, 1}))
    , read_column()
    , number_row_keys() {}

std::shared_ptr<const cached_column> DistinctRowCounterTest::classes_of(std::vector<size_t> classes) {
    return std::make_shared<const cached_column>(cached_column{
        .type = typeid(int), .cells = std::any(std::vector<int>(classes.size(), 0)), .classes = classes, .hash = 0});
}

}

TEST_F(DistinctRowCounterTest, TheMembersClassesAreNumberedInOrderOverEveryRow) {
    EXPECT_CALL(number_row_keys, number(_, _)).Times(AnyNumber());
    EXPECT_CALL(number_row_keys, number(ElementsAre(&second->classes, &first->classes), 4))
        .WillOnce(Return(std::vector<std::vector<size_t>>{{0, 0, 0, 0}, {0, 1, 0, 1}, {0, 1, 2, 3}}));
    EXPECT_EQ(counter.count({9, 7}), 4u);
}

TEST_F(DistinctRowCounterTest, TheCountIsOneMoreThanTheLargestFullKey) {
    ON_CALL(number_row_keys, number(_, _))
        .WillByDefault(Return(std::vector<std::vector<size_t>>{{0, 0, 0, 0}, {0, 1, 0, 1}}));
    EXPECT_EQ(counter.count({9}), 2u);
}

TEST_F(DistinctRowCounterTest, NoMembersCountTheEmptyRowOnce) {
    ON_CALL(number_row_keys, number(ElementsAre(), 4))
        .WillByDefault(Return(std::vector<std::vector<size_t>>{{0, 0, 0, 0}}));
    EXPECT_EQ(counter.count({}), 1u);
}

TEST_F(DistinctRowCounterTest, NoRowsCountNone) {
    distinct_row_counter<NiceMock<MockReadColumn>, NiceMock<MockNumberRowKeys>> empty_counter{
        read_column, number_row_keys, 0};
    ON_CALL(number_row_keys, number(_, 0)).WillByDefault(Return(std::vector<std::vector<size_t>>{{}}));
    EXPECT_EQ(empty_counter.count({}), 0u);
}
