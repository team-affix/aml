#ifndef SET_COST_MEASURER_HPP
#define SET_COST_MEASURER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/feature_id.hpp"

namespace AML {

// Measures a set's cost: the size of its members' ancestry, constants included, plus its distinct rows.
template<typename ISizeAncestry, typename ICountDistinctRows>
struct set_cost_measurer {
    set_cost_measurer(ISizeAncestry& size_ancestry, ICountDistinctRows& count_distinct_rows);
    size_t measure(const std::vector<feature_id>& members);
private:
    ISizeAncestry& size_ancestry_;
    ICountDistinctRows& count_distinct_rows_;
};

#define SET_COST_MEASURER_TEMPLATE template<typename ISizeAncestry, typename ICountDistinctRows>
#define SET_COST_MEASURER          set_cost_measurer<ISizeAncestry, ICountDistinctRows>

SET_COST_MEASURER_TEMPLATE
SET_COST_MEASURER::set_cost_measurer(ISizeAncestry& size_ancestry, ICountDistinctRows& count_distinct_rows)
    : size_ancestry_(size_ancestry)
    , count_distinct_rows_(count_distinct_rows) {}

SET_COST_MEASURER_TEMPLATE
size_t SET_COST_MEASURER::measure(const std::vector<feature_id>& members) {
    return size_ancestry_.size(members, {}) + count_distinct_rows_.count(members);
}

#undef SET_COST_MEASURER
#undef SET_COST_MEASURER_TEMPLATE

}

#endif
