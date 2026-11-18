#ifndef FEATURE_EVALUATOR_HPP
#define FEATURE_EVALUATOR_HPP

#include <variant>
#include <vector>
#include "value_objects/feature.hpp"
#include "value_objects/value.hpp"

// Evaluates a feature on one row, given the values of the variables in scope.
template<typename IApplyPrimitive>
struct feature_evaluator {
    feature_evaluator(IApplyPrimitive& apply_primitive);
    const value* evaluate(const feature* expression, const std::vector<const value*>& scope) const;
private:
    IApplyPrimitive& apply_primitive_;
};

#define FEATURE_EVALUATOR_TEMPLATE template<typename IApplyPrimitive>
#define FEATURE_EVALUATOR          feature_evaluator<IApplyPrimitive>

FEATURE_EVALUATOR_TEMPLATE
FEATURE_EVALUATOR::feature_evaluator(IApplyPrimitive& apply_primitive)
    : apply_primitive_(apply_primitive) {}

FEATURE_EVALUATOR_TEMPLATE
const value* FEATURE_EVALUATOR::evaluate(const feature* expression, const std::vector<const value*>& scope) const {
    if (const feature::var* variable = std::get_if<feature::var>(&expression->content))
        return scope.at(variable->index);
    if (const feature::constant* constant = std::get_if<feature::constant>(&expression->content))
        return constant->literal;
    const feature::app& application = std::get<feature::app>(expression->content);
    std::vector<const value*> args;
    for (const feature* arg : application.args)
        args.push_back(evaluate(arg, scope));
    return apply_primitive_.apply(application.prim, args);
}

#undef FEATURE_EVALUATOR
#undef FEATURE_EVALUATOR_TEMPLATE

#endif
