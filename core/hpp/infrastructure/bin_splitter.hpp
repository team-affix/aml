#ifndef BIN_SPLITTER_HPP
#define BIN_SPLITTER_HPP

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include "value_objects/bin_view.hpp"
#include "value_objects/column.hpp"
#include "value_objects/column_view.hpp"
#include "value_objects/ctor_def.hpp"
#include "value_objects/type_def.hpp"
#include "value_objects/typed_column.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

namespace AML {

// Splits a bin by a one-layer case on a scrutinee column: one child per constructor of the scrutinee's type, in id
// order. A child holds the rows whose scrutinee has its constructor, with that constructor's arguments bound as new
// variables after the parent's inputs. A constructor no row has gives an empty child. A child's inputs and labels
// share their parent's cells and select the child's rows from them; only the bound arguments get cells of their own.
template<typename IGetTypeDef>
struct bin_splitter {
    bin_splitter(IGetTypeDef& get_type_def);
    std::vector<bin_view> split(const bin_view& parent, const column_view& scrutinee) const;
private:
    // A parent's selection of rows and the child's selection made from it, so columns sharing a selection share the
    // child's too.
    using selections_t = std::vector<std::pair<const std::vector<size_t>*, std::shared_ptr<const std::vector<size_t>>>>;

    column_view select(const column_view& parent_column, const std::vector<size_t>& positions,
                       selections_t& selections) const;

    IGetTypeDef& get_type_def_;
};

#define BIN_SPLITTER_TEMPLATE template<typename IGetTypeDef>
#define BIN_SPLITTER          bin_splitter<IGetTypeDef>

BIN_SPLITTER_TEMPLATE
BIN_SPLITTER::bin_splitter(IGetTypeDef& get_type_def)
    : get_type_def_(get_type_def) {}

BIN_SPLITTER_TEMPLATE
std::vector<bin_view> BIN_SPLITTER::split(const bin_view& parent, const column_view& scrutinee) const {
    const size_t row_count = parent.labels.rows->size();
    DEBUG_ASSERT(scrutinee.rows->size() == row_count);
    const std::vector<ctor_def>& ctors = get_type_def_.def(scrutinee.type).ctors;
    // Each child's rows, as positions among the parent's rows.
    std::vector<std::vector<size_t>> positions(ctors.size());
    for (size_t row = 0; row < row_count; ++row)
        positions.at((*scrutinee.cells)[(*scrutinee.rows)[row]].ctor).push_back(row);

    std::vector<bin_view> children;
    children.reserve(ctors.size());
    for (size_t ctor = 0; ctor < ctors.size(); ++ctor) {
        selections_t selections;
        bin_view child{.inputs = {}, .labels = select(parent.labels, positions[ctor], selections)};
        child.inputs.reserve(parent.inputs.size() + ctors[ctor].arg_types.size());
        for (const column_view& variable : parent.inputs)
            child.inputs.push_back(select(variable, positions[ctor], selections));
        for (size_t argument = 0; argument < ctors[ctor].arg_types.size(); ++argument) {
            column cells;
            cells.reserve(positions[ctor].size());
            for (const size_t row : positions[ctor])
                cells.push_back((*scrutinee.cells)[(*scrutinee.rows)[row]].args[argument]);
            child.inputs.push_back(
                column_view::of(typed_column{.type = ctors[ctor].arg_types[argument], .cells = std::move(cells)}));
        }
        children.push_back(std::move(child));
    }
    return children;
}

BIN_SPLITTER_TEMPLATE
column_view BIN_SPLITTER::select(
    const column_view& parent_column, const std::vector<size_t>& positions, selections_t& selections) const {
    const std::vector<size_t>* parent_rows = parent_column.rows.get();
    for (const auto& [shared, selected] : selections) {
        if (shared == parent_rows)
            return column_view{.type = parent_column.type, .cells = parent_column.cells, .rows = selected};
    }
    std::vector<size_t> rows;
    rows.reserve(positions.size());
    for (const size_t position : positions)
        rows.push_back((*parent_rows)[position]);
    selections.emplace_back(parent_rows, std::make_shared<const std::vector<size_t>>(std::move(rows)));
    return column_view{.type = parent_column.type, .cells = parent_column.cells, .rows = selections.back().second};
}

#undef BIN_SPLITTER
#undef BIN_SPLITTER_TEMPLATE

}

#endif
