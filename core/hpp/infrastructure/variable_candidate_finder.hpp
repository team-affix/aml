#ifndef VARIABLE_CANDIDATE_FINDER_HPP
#define VARIABLE_CANDIDATE_FINDER_HPP

#include <vector>
#include "value_objects/bin.hpp"
#include "value_objects/candidate.hpp"
#include "value_objects/var_id.hpp"

// Finds the variables from `first_var` on that are heterogeneous over a bin, each as a candidate.
template<typename IMakeVarFeature, typename ISizeFeature, typename IFindUniformValue>
struct variable_candidate_finder {
    variable_candidate_finder(
        IMakeVarFeature& make_var_feature, ISizeFeature& size_feature, IFindUniformValue& find_uniform_value);
    std::vector<candidate> find(const bin& data, var_id first_var) const;
private:
    IMakeVarFeature& make_var_feature_;
    ISizeFeature& size_feature_;
    IFindUniformValue& find_uniform_value_;
};

#define VARIABLE_CANDIDATE_FINDER_TEMPLATE \
    template<typename IMakeVarFeature, typename ISizeFeature, typename IFindUniformValue>
#define VARIABLE_CANDIDATE_FINDER variable_candidate_finder<IMakeVarFeature, ISizeFeature, IFindUniformValue>

VARIABLE_CANDIDATE_FINDER_TEMPLATE
VARIABLE_CANDIDATE_FINDER::variable_candidate_finder(
    IMakeVarFeature& make_var_feature, ISizeFeature& size_feature, IFindUniformValue& find_uniform_value)
    : make_var_feature_(make_var_feature)
    , size_feature_(size_feature)
    , find_uniform_value_(find_uniform_value) {}

VARIABLE_CANDIDATE_FINDER_TEMPLATE
std::vector<candidate> VARIABLE_CANDIDATE_FINDER::find(const bin& data, var_id first_var) const {
    std::vector<candidate> candidates;
    for (var_id index = first_var; index < data.scope.size(); ++index) {
        if (find_uniform_value_.find(data.scope[index].cells).has_value())
            continue;
        const feature* expression = make_var_feature_.make_var(index);
        candidates.push_back(candidate{
            .expression = expression, .cost = size_feature_.size(expression), .observed = data.scope[index]});
    }
    return candidates;
}

#undef VARIABLE_CANDIDATE_FINDER
#undef VARIABLE_CANDIDATE_FINDER_TEMPLATE

#endif
