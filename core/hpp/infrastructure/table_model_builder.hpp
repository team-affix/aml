#ifndef TABLE_MODEL_BUILDER_HPP
#define TABLE_MODEL_BUILDER_HPP

#include <cstddef>
#include <memory>
#include <vector>
#include "value_objects/cached_column.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/native_column.hpp"
#include "value_objects/native_type.hpp"
#include "value_objects/table_model.hpp"

namespace AML {

// Builds the table model of a set: the graph its members read, their training columns, and each distinct tuple of
// the members' classes mapped to the label of the first training row holding it.
template<typename IBuildModelGraph, typename IReadColumn, typename IGetNativeType>
struct table_model_builder {
    table_model_builder(
        IBuildModelGraph& build_model_graph,
        IReadColumn& read_column,
        IGetNativeType& get_native_type,
        const native_column& labels,
        size_t row_count);
    table_model build(const std::vector<feature_id>& members);
private:
    IBuildModelGraph& build_model_graph_;
    IReadColumn& read_column_;
    IGetNativeType& get_native_type_;
    const native_column& labels_;
    size_t row_count_;
};

#define TABLE_MODEL_BUILDER_TEMPLATE template<typename IBuildModelGraph, typename IReadColumn, typename IGetNativeType>
#define TABLE_MODEL_BUILDER          table_model_builder<IBuildModelGraph, IReadColumn, IGetNativeType>

TABLE_MODEL_BUILDER_TEMPLATE
TABLE_MODEL_BUILDER::table_model_builder(
    IBuildModelGraph& build_model_graph,
    IReadColumn& read_column,
    IGetNativeType& get_native_type,
    const native_column& labels,
    size_t row_count)
    : build_model_graph_(build_model_graph)
    , read_column_(read_column)
    , get_native_type_(get_native_type)
    , labels_(labels)
    , row_count_(row_count) {}

TABLE_MODEL_BUILDER_TEMPLATE
table_model TABLE_MODEL_BUILDER::build(const std::vector<feature_id>& members) {
    table_model model{.graph = build_model_graph_.build(members), .columns = {}, .rows = {}};
    for (const feature_id member : members)
        model.columns.push_back(read_column_.read(member));
    const native_type& label_type = get_native_type_.type(labels_.type);
    for (size_t row = 0; row < row_count_; ++row) {
        std::vector<size_t> key;
        for (const std::shared_ptr<const cached_column>& column : model.columns)
            key.push_back(column->classes[row]);
        if (model.rows.contains(key))
            continue;
        model.rows.emplace(std::move(key), label_type.cell(labels_.cells, row));
    }
    return model;
}

#undef TABLE_MODEL_BUILDER
#undef TABLE_MODEL_BUILDER_TEMPLATE

}

#endif
