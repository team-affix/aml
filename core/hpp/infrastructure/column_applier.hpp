#ifndef COLUMN_APPLIER_HPP
#define COLUMN_APPLIER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/column.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"
#include "debug_assert.hpp"

// Applies a primitive row by row to argument columns of equal length.
template<typename IApplyPrimitive>
struct column_applier {
    column_applier(IApplyPrimitive& apply_primitive);
    column apply(prim_id prim, const std::vector<const column*>& args) const;
private:
    IApplyPrimitive& apply_primitive_;
};

#define COLUMN_APPLIER_TEMPLATE template<typename IApplyPrimitive>
#define COLUMN_APPLIER          column_applier<IApplyPrimitive>

COLUMN_APPLIER_TEMPLATE
COLUMN_APPLIER::column_applier(IApplyPrimitive& apply_primitive)
    : apply_primitive_(apply_primitive) {}

COLUMN_APPLIER_TEMPLATE
column COLUMN_APPLIER::apply(prim_id prim, const std::vector<const column*>& args) const {
    DEBUG_ASSERT(!args.empty());
    const size_t row_count = args.front()->size();
    column cells;
    cells.reserve(row_count);
    std::vector<const value*> row_args(args.size());
    for (size_t row = 0; row < row_count; ++row) {
        for (size_t position = 0; position < args.size(); ++position)
            row_args[position] = (*args[position])[row];
        cells.push_back(apply_primitive_.apply(prim, row_args));
    }
    return cells;
}

#undef COLUMN_APPLIER
#undef COLUMN_APPLIER_TEMPLATE

#endif
