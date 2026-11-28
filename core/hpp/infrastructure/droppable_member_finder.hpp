#ifndef DROPPABLE_MEMBER_FINDER_HPP
#define DROPPABLE_MEMBER_FINDER_HPP

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>
#include "value_objects/cached_column.hpp"
#include "value_objects/feature_id.hpp"

namespace AML {

// Finds the positions of the members a set can drop and still be a function: once the member is gone, rows that agree
// on every other member still share a label. A row's key without member i is its prefix key over the members before
// i paired with its suffix key over the members after i, so every position is checked in one pass over the rows.
template<typename IReadColumn, typename INumberRowKeys>
struct droppable_member_finder {
    droppable_member_finder(
        IReadColumn& read_column, INumberRowKeys& number_row_keys, const std::vector<size_t>& label_classes);
    std::vector<size_t> find(const std::vector<feature_id>& members);
private:
    IReadColumn& read_column_;
    INumberRowKeys& number_row_keys_;
    const std::vector<size_t>& label_classes_;
};

#define DROPPABLE_MEMBER_FINDER_TEMPLATE template<typename IReadColumn, typename INumberRowKeys>
#define DROPPABLE_MEMBER_FINDER          droppable_member_finder<IReadColumn, INumberRowKeys>

DROPPABLE_MEMBER_FINDER_TEMPLATE
DROPPABLE_MEMBER_FINDER::droppable_member_finder(
    IReadColumn& read_column, INumberRowKeys& number_row_keys, const std::vector<size_t>& label_classes)
    : read_column_(read_column)
    , number_row_keys_(number_row_keys)
    , label_classes_(label_classes) {}

DROPPABLE_MEMBER_FINDER_TEMPLATE
std::vector<size_t> DROPPABLE_MEMBER_FINDER::find(const std::vector<feature_id>& members) {
    const size_t row_count = label_classes_.size();
    std::vector<std::shared_ptr<const cached_column>> held;
    for (const feature_id member : members)
        held.push_back(read_column_.read(member));
    std::vector<const std::vector<size_t>*> forward;
    for (const std::shared_ptr<const cached_column>& column : held)
        forward.push_back(&column->classes);
    const std::vector<const std::vector<size_t>*> backward(forward.rbegin(), forward.rend());
    const std::vector<std::vector<size_t>> prefixes = number_row_keys_.number(forward, row_count);
    const std::vector<std::vector<size_t>> suffixes = number_row_keys_.number(backward, row_count);
    std::vector<size_t> droppable;
    for (size_t position = 0; position < members.size(); ++position) {
        const std::vector<size_t>& before = prefixes[position];
        const std::vector<size_t>& after = suffixes[members.size() - 1 - position];
        std::unordered_map<size_t, size_t> labels;
        bool function = true;
        for (size_t row = 0; row < row_count && function; ++row) {
            const size_t key = before[row] * row_count + after[row];
            function = labels.try_emplace(key, label_classes_[row]).first->second == label_classes_[row];
        }
        if (!function)
            continue;
        droppable.push_back(position);
    }
    return droppable;
}

#undef DROPPABLE_MEMBER_FINDER
#undef DROPPABLE_MEMBER_FINDER_TEMPLATE

}

#endif
