#ifndef OPERAND_APPLIER_HPP
#define OPERAND_APPLIER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/constant_operand.hpp"
#include "value_objects/prim_id.hpp"
#include "value_objects/value.hpp"

// Applies a primitive row by row to a tuple of operand indices: the constants first, then the session's features.
// A constant gives its literal on every row and is never laid out as a column.
template<typename IGetConstants, typename IGetFeatureCandidates, typename IApplyPrimitive>
struct operand_applier {
    operand_applier(
        IGetConstants& get_constants, IGetFeatureCandidates& get_feature_candidates, IApplyPrimitive& apply_primitive);
    column apply(prim_id prim, const std::vector<size_t>& args, size_t row_count) const;
private:
    IGetConstants& get_constants_;
    IGetFeatureCandidates& get_feature_candidates_;
    IApplyPrimitive& apply_primitive_;
};

#define OPERAND_APPLIER_TEMPLATE template<typename IGC, typename IGFC, typename IAP>
#define OPERAND_APPLIER          operand_applier<IGC, IGFC, IAP>

OPERAND_APPLIER_TEMPLATE
OPERAND_APPLIER::operand_applier(IGC& get_constants, IGFC& get_feature_candidates, IAP& apply_primitive)
    : get_constants_(get_constants)
    , get_feature_candidates_(get_feature_candidates)
    , apply_primitive_(apply_primitive) {}

OPERAND_APPLIER_TEMPLATE
column OPERAND_APPLIER::apply(prim_id prim, const std::vector<size_t>& args, size_t row_count) const {
    const std::vector<constant_operand>& constants = get_constants_.constants();
    const std::vector<candidate>& features = get_feature_candidates_.candidates();
    column cells;
    cells.reserve(row_count);
    std::vector<const value*> row_args(args.size());
    for (size_t row = 0; row < row_count; ++row) {
        for (size_t position = 0; position < args.size(); ++position) {
            const size_t operand = args[position];
            const bool is_constant = operand < constants.size();
            row_args[position] = is_constant
                ? constants[operand].literal
                : features[operand - constants.size()].observed.cells[row];
        }
        cells.push_back(apply_primitive_.apply(prim, row_args));
    }
    return cells;
}

#undef OPERAND_APPLIER
#undef OPERAND_APPLIER_TEMPLATE

#endif
