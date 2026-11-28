#ifndef DISTINCT_ROW_COUNTER_HPP
#define DISTINCT_ROW_COUNTER_HPP

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>
#include "value_objects/cached_column.hpp"
#include "value_objects/feature_id.hpp"

namespace AML {

// Counts the distinct rows of a set of features: the distinct tuples of their values over the training rows. With no
// features every row is the empty tuple, so a nonempty training set has one.
template<typename IReadColumn, typename INumberRowKeys>
struct distinct_row_counter {
    distinct_row_counter(IReadColumn& read_column, INumberRowKeys& number_row_keys, size_t row_count);
    size_t count(const std::vector<feature_id>& members);
private:
    IReadColumn& read_column_;
    INumberRowKeys& number_row_keys_;
    size_t row_count_;
};

#define DISTINCT_ROW_COUNTER_TEMPLATE template<typename IReadColumn, typename INumberRowKeys>
#define DISTINCT_ROW_COUNTER          distinct_row_counter<IReadColumn, INumberRowKeys>

DISTINCT_ROW_COUNTER_TEMPLATE
DISTINCT_ROW_COUNTER::distinct_row_counter(
    IReadColumn& read_column, INumberRowKeys& number_row_keys, size_t row_count)
    : read_column_(read_column)
    , number_row_keys_(number_row_keys)
    , row_count_(row_count) {}

DISTINCT_ROW_COUNTER_TEMPLATE
size_t DISTINCT_ROW_COUNTER::count(const std::vector<feature_id>& members) {
    std::vector<std::shared_ptr<const cached_column>> held;
    std::vector<const std::vector<size_t>*> columns;
    for (const feature_id member : members)
        held.push_back(read_column_.read(member));
    for (const std::shared_ptr<const cached_column>& column : held)
        columns.push_back(&column->classes);
    const std::vector<std::vector<size_t>> layers = number_row_keys_.number(columns, row_count_);
    size_t distinct = 0;
    for (const size_t key : layers.back())
        distinct = std::max(distinct, key + 1);
    return distinct;
}

#undef DISTINCT_ROW_COUNTER
#undef DISTINCT_ROW_COUNTER_TEMPLATE

}

#endif
