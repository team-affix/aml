#ifndef GRAPH_ROW_EVALUATOR_HPP
#define GRAPH_ROW_EVALUATOR_HPP

#include <any>
#include <memory>
#include <variant>
#include <vector>
#include "value_objects/cached_constant.hpp"
#include "value_objects/derivation.hpp"
#include "value_objects/model_graph.hpp"
#include "value_objects/native_primitive.hpp"
#include "value_objects/native_type.hpp"
#include "value_objects/operand.hpp"
#include "value_objects/operand_data.hpp"

namespace AML {

// Evaluates a model graph on one input row, node by node, each as a one-row column: an input filled into one row, an
// application applied to its feature arguments' columns and its constant arguments' cached values as scalars. Gives the
// outputs' one-row columns in order.
template<typename IGetNativePrimitive, typename IGetNativeType, typename IReadConstant>
struct graph_row_evaluator {
    graph_row_evaluator(
        IGetNativePrimitive& get_native_primitive,
        IGetNativeType& get_native_type,
        IReadConstant& read_constant);
    std::vector<std::any> evaluate(const model_graph& graph, const std::vector<std::any>& inputs) const;
private:
    IGetNativePrimitive& get_native_primitive_;
    IGetNativeType& get_native_type_;
    IReadConstant& read_constant_;
};

#define GRAPH_ROW_EVALUATOR_TEMPLATE \
    template<typename IGetNativePrimitive, typename IGetNativeType, typename IReadConstant>
#define GRAPH_ROW_EVALUATOR graph_row_evaluator<IGetNativePrimitive, IGetNativeType, IReadConstant>

GRAPH_ROW_EVALUATOR_TEMPLATE
GRAPH_ROW_EVALUATOR::graph_row_evaluator(
    IGetNativePrimitive& get_native_primitive,
    IGetNativeType& get_native_type,
    IReadConstant& read_constant)
    : get_native_primitive_(get_native_primitive)
    , get_native_type_(get_native_type)
    , read_constant_(read_constant) {}

GRAPH_ROW_EVALUATOR_TEMPLATE
std::vector<std::any> GRAPH_ROW_EVALUATOR::evaluate(
    const model_graph& graph, const std::vector<std::any>& inputs) const {
    std::vector<std::any> columns;
    columns.reserve(graph.nodes.size());
    for (size_t node = 0; node < graph.nodes.size(); ++node) {
        const derivation& origin = graph.nodes[node];
        if (const derivation::input* input = std::get_if<derivation::input>(&origin.content)) {
            columns.push_back(get_native_type_.type(graph.types[node]).fill(inputs.at(input->index), 1));
            continue;
        }
        const derivation::application& application = std::get<derivation::application>(origin.content);
        std::vector<std::shared_ptr<const cached_constant>> held_constants;
        std::vector<operand_data> operands;
        for (const operand& arg : application.args) {
            const operand::from_feature* argument = std::get_if<operand::from_feature>(&arg.source);
            if (argument == nullptr) {
                held_constants.push_back(read_constant_.read(std::get<operand::from_constant>(arg.source).id));
                operands.push_back(operand_data{.data = &held_constants.back()->cell, .scalar = true});
                continue;
            }
            operands.push_back(operand_data{.data = &columns[argument->id], .scalar = false});
        }
        columns.push_back(get_native_primitive_.primitive(application.prim).apply(operands, 1));
    }
    std::vector<std::any> outputs;
    for (const size_t output : graph.outputs)
        outputs.push_back(columns[output]);
    return outputs;
}

#undef GRAPH_ROW_EVALUATOR
#undef GRAPH_ROW_EVALUATOR_TEMPLATE

}

#endif
