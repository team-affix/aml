#ifndef TABLE_MODEL_EVALUATOR_HPP
#define TABLE_MODEL_EVALUATOR_HPP

#include <any>
#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/table_model.hpp"

namespace AML {

// Evaluates a table model on one input: evaluates its graph on the inputs, finds each feature's value's class in the
// feature's training column, and looks the tuple of classes up. A value no training row held, or a tuple the table
// does not hold, gives no label.
template<typename IEvaluateGraphRow, typename IFindValueClass>
struct table_model_evaluator {
    table_model_evaluator(IEvaluateGraphRow& evaluate_graph_row, IFindValueClass& find_value_class);
    std::optional<std::any> evaluate(const table_model& model, const std::vector<std::any>& inputs) const;
private:
    IEvaluateGraphRow& evaluate_graph_row_;
    IFindValueClass& find_value_class_;
};

#define TABLE_MODEL_EVALUATOR_TEMPLATE template<typename IEvaluateGraphRow, typename IFindValueClass>
#define TABLE_MODEL_EVALUATOR          table_model_evaluator<IEvaluateGraphRow, IFindValueClass>

TABLE_MODEL_EVALUATOR_TEMPLATE
TABLE_MODEL_EVALUATOR::table_model_evaluator(IEvaluateGraphRow& evaluate_graph_row, IFindValueClass& find_value_class)
    : evaluate_graph_row_(evaluate_graph_row)
    , find_value_class_(find_value_class) {}

TABLE_MODEL_EVALUATOR_TEMPLATE
std::optional<std::any> TABLE_MODEL_EVALUATOR::evaluate(
    const table_model& model, const std::vector<std::any>& inputs) const {
    const std::vector<std::any> values = evaluate_graph_row_.evaluate(model.graph, inputs);
    std::vector<size_t> key;
    for (size_t position = 0; position < values.size(); ++position) {
        const std::optional<size_t> value_class = find_value_class_.find(*model.columns[position], values[position]);
        if (!value_class.has_value())
            return std::nullopt;
        key.push_back(value_class.value());
    }
    const auto found = model.rows.find(key);
    if (found == model.rows.end())
        return std::nullopt;
    return found->second;
}

#undef TABLE_MODEL_EVALUATOR
#undef TABLE_MODEL_EVALUATOR_TEMPLATE

}

#endif
