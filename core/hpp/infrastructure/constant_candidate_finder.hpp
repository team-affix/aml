#ifndef CONSTANT_CANDIDATE_FINDER_HPP
#define CONSTANT_CANDIDATE_FINDER_HPP

#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/column.hpp"
#include "value_objects/constant_operand.hpp"
#include "value_objects/typed_column.hpp"

// Finds the constants as candidates over a bin: each observes its literal on every row.
template<typename IGetConstants, typename ISizeFeature>
struct constant_candidate_finder {
    constant_candidate_finder(IGetConstants& get_constants, ISizeFeature& size_feature);
    std::vector<candidate> find(const bin& data) const;
private:
    IGetConstants& get_constants_;
    ISizeFeature& size_feature_;
};

#define CONSTANT_CANDIDATE_FINDER_TEMPLATE template<typename IGetConstants, typename ISizeFeature>
#define CONSTANT_CANDIDATE_FINDER          constant_candidate_finder<IGetConstants, ISizeFeature>

CONSTANT_CANDIDATE_FINDER_TEMPLATE
CONSTANT_CANDIDATE_FINDER::constant_candidate_finder(IGetConstants& get_constants, ISizeFeature& size_feature)
    : get_constants_(get_constants)
    , size_feature_(size_feature) {}

CONSTANT_CANDIDATE_FINDER_TEMPLATE
std::vector<candidate> CONSTANT_CANDIDATE_FINDER::find(const bin& data) const {
    std::vector<candidate> candidates;
    for (const constant_operand& constant : get_constants_.constants()) {
        candidates.push_back(candidate{
            .expression = constant.expression,
            .cost = size_feature_.size(constant.expression),
            .observed = typed_column{.type = constant.type, .cells = column(data.labels.size(), constant.literal)}});
    }
    return candidates;
}

#undef CONSTANT_CANDIDATE_FINDER
#undef CONSTANT_CANDIDATE_FINDER_TEMPLATE

#endif
