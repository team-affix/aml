#include <optional>
#include <typeindex>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/cheapest_set_visitor.hpp"

using namespace AML;

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockPopSet {
    MOCK_METHOD(std::optional<queued_set>, pop, ());
};

struct MockBuildTableModel {
    MOCK_METHOD(table_model, build, (const std::vector<feature_id>&));
};

struct MockRecordBestTable {
    MOCK_METHOD(void, try_record, (const table_model&, size_t));
};

struct MockExpandSet {
    MOCK_METHOD(void, expand, (const std::vector<feature_id>&));
};

using test_cheapest_set_visitor_t = cheapest_set_visitor<
    NiceMock<MockPopSet>, NiceMock<MockBuildTableModel>, NiceMock<MockRecordBestTable>, NiceMock<MockExpandSet>,
    NiceMock<MockExpandSet>>;

struct CheapestSetVisitorTest : public ::testing::Test {
    CheapestSetVisitorTest();
    queued_set cheapest;
    table_model model;
    NiceMock<MockPopSet> pop_set;
    NiceMock<MockBuildTableModel> build_table_model;
    NiceMock<MockRecordBestTable> record_best_table;
    NiceMock<MockExpandSet> expand_set0;
    NiceMock<MockExpandSet> expand_set1;
    test_cheapest_set_visitor_t visitor{pop_set, build_table_model, record_best_table, expand_set0, expand_set1};

    void SetUp() override {
        ON_CALL(pop_set, pop()).WillByDefault(Return(std::optional<queued_set>(cheapest)));
        ON_CALL(build_table_model, build(cheapest.members)).WillByDefault(Return(model));
    }
};

CheapestSetVisitorTest::CheapestSetVisitorTest()
    : cheapest{.members = {2, 6}, .cost = 11}
    , model{
          .graph = model_graph{
              .nodes = {derivation{derivation::input{.index = 0}}}, .types = {typeid(int)}, .outputs = {0}},
          .columns = {},
          .rows = {}}
    , pop_set()
    , build_table_model()
    , record_best_table()
    , expand_set0()
    , expand_set1() {}

}

TEST_F(CheapestSetVisitorTest, AnEmptyQueueGivesNoOutcomeAndDoesNothing) {
    ON_CALL(pop_set, pop()).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(record_best_table, try_record(_, _)).Times(0);
    EXPECT_CALL(expand_set0, expand(_)).Times(0);
    EXPECT_CALL(expand_set1, expand(_)).Times(0);
    EXPECT_FALSE(visitor.visit().has_value());
}

TEST_F(CheapestSetVisitorTest, TheOutcomeIsTheSetsModelAndCost) {
    const std::optional<table_outcome> outcome = visitor.visit();
    ASSERT_TRUE(outcome.has_value());
    EXPECT_EQ(outcome.value().model.graph, model.graph);
    EXPECT_EQ(outcome.value().cost, 11u);
}

TEST_F(CheapestSetVisitorTest, TheModelIsOfferedAsTheBestWithTheSetsCost) {
    EXPECT_CALL(record_best_table, try_record(::testing::Field(&table_model::graph, model.graph), 11));
    visitor.visit();
}

TEST_F(CheapestSetVisitorTest, BothExpandersExpandTheSetInOrder) {
    InSequence order;
    EXPECT_CALL(expand_set0, expand(cheapest.members));
    EXPECT_CALL(expand_set1, expand(cheapest.members));
    visitor.visit();
}

TEST_F(CheapestSetVisitorTest, EachVisitPopsOneSet) {
    EXPECT_CALL(pop_set, pop()).Times(1).WillOnce(Return(std::optional<queued_set>(cheapest)));
    visitor.visit();
}
