#ifndef MODEL_EVALUATOR_HPP
#define MODEL_EVALUATOR_HPP

#include <optional>
#include <variant>
#include <vector>
#include "value_objects/ctor_decl.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/value.hpp"

// Infers a label for one input tuple: each match evaluates its scrutinee, binds the matched constructor's arguments
// and descends into that constructor's branch. A leaf gives its output evaluated in that scope; an empty node gives
// nothing.
template<typename IEvaluateFeature, typename IGetCtorDecl>
struct model_evaluator {
    model_evaluator(IEvaluateFeature& evaluate_feature, IGetCtorDecl& get_ctor_decl);
    std::optional<const value*> evaluate(const model_node* model, const std::vector<const value*>& inputs) const;
private:
    IEvaluateFeature& evaluate_feature_;
    IGetCtorDecl& get_ctor_decl_;
};

#define MODEL_EVALUATOR_TEMPLATE template<typename IEvaluateFeature, typename IGetCtorDecl>
#define MODEL_EVALUATOR          model_evaluator<IEvaluateFeature, IGetCtorDecl>

MODEL_EVALUATOR_TEMPLATE
MODEL_EVALUATOR::model_evaluator(IEvaluateFeature& evaluate_feature, IGetCtorDecl& get_ctor_decl)
    : evaluate_feature_(evaluate_feature)
    , get_ctor_decl_(get_ctor_decl) {}

MODEL_EVALUATOR_TEMPLATE
std::optional<const value*> MODEL_EVALUATOR::evaluate(
    const model_node* model, const std::vector<const value*>& inputs) const {
    std::vector<const value*> scope = inputs;
    const model_node* node = model;
    while (const model_node::match* match = std::get_if<model_node::match>(&node->content)) {
        const value* matched = evaluate_feature_.evaluate(match->scrutinee, scope);
        scope.insert(scope.end(), matched->args.begin(), matched->args.end());
        node = match->branches.at(get_ctor_decl_.decl(matched->ctor).tag);
    }
    if (const model_node::leaf* leaf = std::get_if<model_node::leaf>(&node->content))
        return evaluate_feature_.evaluate(leaf->output, scope);
    return std::nullopt;
}

#undef MODEL_EVALUATOR
#undef MODEL_EVALUATOR_TEMPLATE

#endif
