#ifndef TABLE_MODEL_HPP
#define TABLE_MODEL_HPP

#include <any>
#include <map>
#include <memory>
#include <vector>
#include "value_objects/cached_column.hpp"
#include "value_objects/model_graph.hpp"

namespace AML {

// A model that reads a row of feature values and looks its label up. `columns` holds each feature's training column,
// in the order of the graph's outputs; a value takes the class of the training row holding it, and `rows` maps each
// distinct tuple of classes to its label. Not comparable, since its columns and labels are anys.
struct table_model {
    model_graph graph;
    std::vector<std::shared_ptr<const cached_column>> columns;
    std::map<std::vector<size_t>, std::any> rows;
};

}

#endif
