#ifndef TABLE_OUTCOME_HPP
#define TABLE_OUTCOME_HPP

#include <cstddef>
#include "value_objects/table_model.hpp"

namespace AML {

// What one search step produced: the model of the set it visited and that set's cost. Not comparable, since a table
// model is not.
struct table_outcome {
    table_model model;
    size_t cost;
};

}

#endif
