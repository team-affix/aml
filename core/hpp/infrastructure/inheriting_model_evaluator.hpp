#ifndef INHERITING_MODEL_EVALUATOR_HPP
#define INHERITING_MODEL_EVALUATOR_HPP

#include <optional>
#include <variant>
#include <vector>
#include "value_objects/feature.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/value.hpp"

namespace AML {

// Infers a label for one input tuple from a model whose matches pass their scrutinee's pieces down as variables (see
// inheriting_bin_splitter.hpp): each match evaluates its scrutinee and its pieces, binds the matched constructor's
// arguments and then the pieces' values, and descends into that constructor's branch. A leaf gives its output
// evaluated in that scope; an empty node gives nothing.
template<typename IEvaluateFeature, typename IFindScrutineePieces>
struct inheriting_model_evaluator {
    inheriting_model_evaluator(IEvaluateFeature& evaluate_feature, IFindScrutineePieces& find_scrutinee_pieces);
    std::optional<value> evaluate(const model_node& model, const std::vector<value>& inputs) const;
private:
    IEvaluateFeature& evaluate_feature_;
    IFindScrutineePieces& find_scrutinee_pieces_;
};

#define INHERITING_MODEL_EVALUATOR_TEMPLATE template<typename IEvaluateFeature, typename IFindScrutineePieces>
#define INHERITING_MODEL_EVALUATOR          inheriting_model_evaluator<IEvaluateFeature, IFindScrutineePieces>

INHERITING_MODEL_EVALUATOR_TEMPLATE
INHERITING_MODEL_EVALUATOR::inheriting_model_evaluator(
    IEvaluateFeature& evaluate_feature, IFindScrutineePieces& find_scrutinee_pieces)
    : evaluate_feature_(evaluate_feature)
    , find_scrutinee_pieces_(find_scrutinee_pieces) {}

INHERITING_MODEL_EVALUATOR_TEMPLATE
std::optional<value> INHERITING_MODEL_EVALUATOR::evaluate(
    const model_node& model, const std::vector<value>& inputs) const {
    std::vector<value> scope = inputs;
    const model_node* node = &model;
    while (const model_node::match* match = std::get_if<model_node::match>(&node->content)) {
        const value matched = evaluate_feature_.evaluate(match->scrutinee, scope);
        std::vector<value> piece_values;
        for (const feature& piece : find_scrutinee_pieces_.find(match->scrutinee))
            piece_values.push_back(evaluate_feature_.evaluate(piece, scope));
        scope.insert(scope.end(), matched.args.begin(), matched.args.end());
        scope.insert(scope.end(), piece_values.begin(), piece_values.end());
        node = &match->branches.at(matched.ctor);
    }
    if (const model_node::leaf* leaf = std::get_if<model_node::leaf>(&node->content))
        return evaluate_feature_.evaluate(leaf->output, scope);
    return std::nullopt;
}

#undef INHERITING_MODEL_EVALUATOR
#undef INHERITING_MODEL_EVALUATOR_TEMPLATE

}

#endif
