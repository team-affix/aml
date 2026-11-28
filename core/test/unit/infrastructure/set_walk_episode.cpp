#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/set_walk_episode.hpp"

using namespace AML;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

using members_t = std::vector<feature_id>;

struct MockMoveSet {
    MOCK_METHOD(std::optional<members_t>, move, (const members_t&));
};

struct MockMeasureSetCost {
    MOCK_METHOD(size_t, measure, (const members_t&));
};

struct MockBuildTableModel {
    MOCK_METHOD(table_model, build, (const members_t&));
};

struct MockRecordBestTable {
    MOCK_METHOD(void, try_record, (const table_model&, size_t));
};

using test_set_walk_episode_t = set_walk_episode<
    NiceMock<MockMoveSet>, NiceMock<MockMeasureSetCost>, NiceMock<MockBuildTableModel>, NiceMock<MockRecordBestTable>>;

// A built model's graph outputs are its set's members, so a test can tell which set was built. The walk goes from
// the root {1, 2, 3} to {1, 2}, then {1, 2, 4}, then {2, 4}, and has no move after; costs are set per test.
struct SetWalkEpisodeTest : public ::testing::Test {
    SetWalkEpisodeTest();
    static table_model model_of(const members_t& members);
    test_set_walk_episode_t episode_with(size_t max_moves);
    void cost(const members_t& members, size_t value);
    members_t root;
    NiceMock<MockMoveSet> move_set;
    NiceMock<MockMeasureSetCost> measure_set_cost;
    NiceMock<MockBuildTableModel> build_table_model;
    NiceMock<MockRecordBestTable> record_best_table;

    void SetUp() override {
        ON_CALL(move_set, move(members_t{1, 2, 3})).WillByDefault(Return(members_t{1, 2}));
        ON_CALL(move_set, move(members_t{1, 2})).WillByDefault(Return(members_t{1, 2, 4}));
        ON_CALL(move_set, move(members_t{1, 2, 4})).WillByDefault(Return(members_t{2, 4}));
        ON_CALL(move_set, move(members_t{2, 4})).WillByDefault(Return(std::nullopt));
        ON_CALL(build_table_model, build(_)).WillByDefault(Invoke(&SetWalkEpisodeTest::model_of));
    }
};

SetWalkEpisodeTest::SetWalkEpisodeTest()
    : root{1, 2, 3}
    , move_set()
    , measure_set_cost()
    , build_table_model()
    , record_best_table() {}

table_model SetWalkEpisodeTest::model_of(const members_t& members) {
    return table_model{
        .graph = model_graph{.nodes = {}, .types = {}, .outputs = std::vector<size_t>(members.begin(), members.end())},
        .columns = {},
        .rows = {}};
}

test_set_walk_episode_t SetWalkEpisodeTest::episode_with(size_t max_moves) {
    return test_set_walk_episode_t(move_set, measure_set_cost, build_table_model, record_best_table, root, max_moves);
}

void SetWalkEpisodeTest::cost(const members_t& members, size_t value) {
    ON_CALL(measure_set_cost, measure(members)).WillByDefault(Return(value));
}

std::vector<size_t> outputs_of(const members_t& members) {
    return std::vector<size_t>(members.begin(), members.end());
}

}

TEST_F(SetWalkEpisodeTest, TheCheapestSetPassedThroughIsReturnedWithItsCost) {
    cost({1, 2, 3}, 9);
    cost({1, 2}, 12);
    cost({1, 2, 4}, 6);
    cost({2, 4}, 8);
    test_set_walk_episode_t episode = episode_with(10);
    const table_outcome outcome = episode.run();
    EXPECT_EQ(outcome.cost, 6u);
    EXPECT_EQ(outcome.model.graph.outputs, outputs_of({1, 2, 4}));
}

TEST_F(SetWalkEpisodeTest, TheRootIsReturnedWhenNoLaterSetIsCheaper) {
    cost({1, 2, 3}, 5);
    cost({1, 2}, 7);
    cost({1, 2, 4}, 6);
    cost({2, 4}, 8);
    test_set_walk_episode_t episode = episode_with(10);
    EXPECT_EQ(episode.run().model.graph.outputs, outputs_of({1, 2, 3}));
}

TEST_F(SetWalkEpisodeTest, ATieKeepsTheEarlierSet) {
    cost({1, 2, 3}, 9);
    cost({1, 2}, 6);
    cost({1, 2, 4}, 6);
    cost({2, 4}, 6);
    test_set_walk_episode_t episode = episode_with(10);
    EXPECT_EQ(episode.run().model.graph.outputs, outputs_of({1, 2}));
}

TEST_F(SetWalkEpisodeTest, TheWalkStopsAfterMaxMoves) {
    cost({1, 2, 3}, 9);
    cost({1, 2}, 8);
    cost({1, 2, 4}, 7);
    cost({2, 4}, 1);
    EXPECT_CALL(move_set, move(_)).Times(2);
    test_set_walk_episode_t episode = episode_with(2);
    const table_outcome outcome = episode.run();
    EXPECT_EQ(outcome.cost, 7u);
    EXPECT_EQ(outcome.model.graph.outputs, outputs_of({1, 2, 4}));
}

TEST_F(SetWalkEpisodeTest, NoMovesKeepsTheRoot) {
    cost({1, 2, 3}, 9);
    EXPECT_CALL(move_set, move(_)).Times(0);
    test_set_walk_episode_t episode = episode_with(0);
    const table_outcome outcome = episode.run();
    EXPECT_EQ(outcome.cost, 9u);
    EXPECT_EQ(outcome.model.graph.outputs, outputs_of({1, 2, 3}));
}

TEST_F(SetWalkEpisodeTest, TheWalkStopsOnceNoMoveIsLeft) {
    EXPECT_CALL(move_set, move(_)).Times(4);
    test_set_walk_episode_t episode = episode_with(100);
    episode.run();
}

TEST_F(SetWalkEpisodeTest, TheReturnedSetsModelIsRecordedWithItsCost) {
    cost({1, 2, 3}, 9);
    cost({1, 2}, 4);
    cost({1, 2, 4}, 6);
    cost({2, 4}, 8);
    EXPECT_CALL(record_best_table, try_record(Field(&table_model::graph, model_of({1, 2}).graph), 4));
    test_set_walk_episode_t episode = episode_with(10);
    episode.run();
}

TEST_F(SetWalkEpisodeTest, EachEpisodeStartsFromTheRoot) {
    InSequence order;
    EXPECT_CALL(move_set, move(members_t{1, 2, 3}));
    EXPECT_CALL(move_set, move(members_t{1, 2})).WillOnce(Return(std::nullopt));
    EXPECT_CALL(move_set, move(members_t{1, 2, 3})).WillOnce(Return(std::nullopt));
    test_set_walk_episode_t episode = episode_with(10);
    episode.run();
    episode.run();
}
