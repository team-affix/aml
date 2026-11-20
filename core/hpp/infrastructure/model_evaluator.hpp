#ifndef MODEL_EVALUATOR_HPP
#define MODEL_EVALUATOR_HPP

#include <optional>
#include <variant>
#include <vector>
#include "value_objects/model_node.hpp"
#include "value_objects/value.hpp"

namespace AML {

// Infers a label for one input tuple: each match evaluates its scrutinee, binds the matched constructor's arguments
// and descends into that constructor's branch. A leaf gives its output evaluated in that scope; an empty node gives
// nothing.
template<typename IEvaluateFeature>
struct model_evaluator {
    model_evaluator(IEvaluateFeature& evaluate_feature);
    std::optional<value> evaluate(const model_node& model, const std::vector<value>& inputs) const;
private:
    IEvaluateFeature& evaluate_feature_;
};

#define MODEL_EVALUATOR_TEMPLATE template<typename IEvaluateFeature>
#define MODEL_EVALUATOR          model_evaluator<IEvaluateFeature>

MODEL_EVALUATOR_TEMPLATE
MODEL_EVALUATOR::model_evaluator(IEvaluateFeature& evaluate_feature)
    : evaluate_feature_(evaluate_feature) {}

MODEL_EVALUATOR_TEMPLATE
std::optional<value> MODEL_EVALUATOR::evaluate(const model_node& model, const std::vector<value>& inputs) const {
    std::vector<value> scope = inputs;
    const model_node* node = &model;
    while (const model_node::match* match = std::get_if<model_node::match>(&node->content)) {
        const value matched = evaluate_feature_.evaluate(match->scrutinee, scope);
        scope.insert(scope.end(), matched.args.begin(), matched.args.end());
        node = &match->branches.at(matched.ctor);
    }
    if (const model_node::leaf* leaf = std::get_if<model_node::leaf>(&node->content))
        return evaluate_feature_.evaluate(leaf->output, scope);
    return std::nullopt;
}

#undef MODEL_EVALUATOR
#undef MODEL_EVALUATOR_TEMPLATE

}

#endif
