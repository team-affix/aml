#ifndef VARIABLE_CANDIDATE_FINDER_HPP
#define VARIABLE_CANDIDATE_FINDER_HPP

#include <cstddef>
#include <utility>
#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/feature.hpp"
#include "value_objects/var_id.hpp"

// Finds the variables from `first_var` on that are heterogeneous over a bin, each as a candidate.
template<typename ISizeFeature, typename IFindUniformValue>
struct variable_candidate_finder {
    variable_candidate_finder(ISizeFeature& size_feature, IFindUniformValue& find_uniform_value);
    std::vector<candidate> find(const bin& data, var_id first_var) const;
private:
    ISizeFeature& size_feature_;
    IFindUniformValue& find_uniform_value_;
};

#define VARIABLE_CANDIDATE_FINDER_TEMPLATE template<typename ISizeFeature, typename IFindUniformValue>
#define VARIABLE_CANDIDATE_FINDER          variable_candidate_finder<ISizeFeature, IFindUniformValue>

VARIABLE_CANDIDATE_FINDER_TEMPLATE
VARIABLE_CANDIDATE_FINDER::variable_candidate_finder(ISizeFeature& size_feature, IFindUniformValue& find_uniform_value)
    : size_feature_(size_feature)
    , find_uniform_value_(find_uniform_value) {}

VARIABLE_CANDIDATE_FINDER_TEMPLATE
std::vector<candidate> VARIABLE_CANDIDATE_FINDER::find(const bin& data, var_id first_var) const {
    std::vector<candidate> candidates;
    for (var_id index = first_var; index < data.inputs.size(); ++index) {
        if (find_uniform_value_.find(data.inputs[index].cells).has_value())
            continue;
        feature expression{feature::var{.index = index}};
        const size_t cost = size_feature_.size(expression);
        candidates.push_back(
            candidate{.expression = std::move(expression), .cost = cost, .observed = data.inputs[index]});
    }
    return candidates;
}

#undef VARIABLE_CANDIDATE_FINDER
#undef VARIABLE_CANDIDATE_FINDER_TEMPLATE

#endif
