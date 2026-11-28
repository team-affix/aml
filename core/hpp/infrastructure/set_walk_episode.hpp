#ifndef SET_WALK_EPISODE_HPP
#define SET_WALK_EPISODE_HPP

#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/feature_id.hpp"
#include "value_objects/table_model.hpp"
#include "value_objects/table_outcome.hpp"

namespace AML {

// Walks from the root set by up to max_moves moves, stopping early once no move is left, and keeps the cheapest set
// it passed through, the root included; a tie keeps the earlier set. Records that set's table model if it is the
// cheapest yet, and returns it with its cost as measured when it was passed.
template<typename IMoveSet, typename IMeasureSetCost, typename IBuildTableModel, typename IRecordBestTable>
struct set_walk_episode {
    set_walk_episode(
        IMoveSet& move_set,
        IMeasureSetCost& measure_set_cost,
        IBuildTableModel& build_table_model,
        IRecordBestTable& record_best_table,
        const std::vector<feature_id>& root,
        size_t max_moves);
    table_outcome run();
private:
    IMoveSet& move_set_;
    IMeasureSetCost& measure_set_cost_;
    IBuildTableModel& build_table_model_;
    IRecordBestTable& record_best_table_;
    const std::vector<feature_id>& root_;
    size_t max_moves_;
};

#define SET_WALK_EPISODE_TEMPLATE \
    template<typename IMoveSet, typename IMeasureSetCost, typename IBuildTableModel, typename IRecordBestTable>
#define SET_WALK_EPISODE set_walk_episode<IMoveSet, IMeasureSetCost, IBuildTableModel, IRecordBestTable>

SET_WALK_EPISODE_TEMPLATE
SET_WALK_EPISODE::set_walk_episode(
    IMoveSet& move_set,
    IMeasureSetCost& measure_set_cost,
    IBuildTableModel& build_table_model,
    IRecordBestTable& record_best_table,
    const std::vector<feature_id>& root,
    size_t max_moves)
    : move_set_(move_set)
    , measure_set_cost_(measure_set_cost)
    , build_table_model_(build_table_model)
    , record_best_table_(record_best_table)
    , root_(root)
    , max_moves_(max_moves) {}

SET_WALK_EPISODE_TEMPLATE
table_outcome SET_WALK_EPISODE::run() {
    std::vector<feature_id> members = root_;
    std::vector<feature_id> cheapest = members;
    size_t cheapest_cost = measure_set_cost_.measure(members);
    for (size_t move = 0; move < max_moves_; ++move) {
        std::optional<std::vector<feature_id>> moved = move_set_.move(members);
        if (!moved.has_value())
            break;
        members = std::move(moved.value());
        const size_t cost = measure_set_cost_.measure(members);
        if (cost >= cheapest_cost)
            continue;
        cheapest = members;
        cheapest_cost = cost;
    }
    const table_model model = build_table_model_.build(cheapest);
    record_best_table_.try_record(model, cheapest_cost);
    return table_outcome{.model = model, .cost = cheapest_cost};
}

#undef SET_WALK_EPISODE
#undef SET_WALK_EPISODE_TEMPLATE

}

#endif
