#ifndef MODEL_GRAPH_BUILDER_HPP
#define MODEL_GRAPH_BUILDER_HPP

#include <cstddef>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/derivation.hpp"
#include "value_objects/feature_id.hpp"
#include "value_objects/feature_record.hpp"
#include "value_objects/model_graph.hpp"
#include "value_objects/operand.hpp"

namespace AML {

// Builds the graph a model of a set reads: the members' feature ancestry, each feature once and after its feature
// arguments, with those arguments renumbered as node positions and constant arguments kept as constant ids, and the
// members as the outputs in order.
template<typename IGetFeatureRecord>
struct model_graph_builder {
    model_graph_builder(IGetFeatureRecord& get_feature_record);
    model_graph build(const std::vector<feature_id>& members) const;
private:
    size_t place(feature_id feature, model_graph& graph, std::unordered_map<feature_id, size_t>& positions) const;
    size_t append(
        feature_id feature, const feature_record& record, const derivation& origin, model_graph& graph,
        std::unordered_map<feature_id, size_t>& positions) const;
    IGetFeatureRecord& get_feature_record_;
};

#define MODEL_GRAPH_BUILDER_TEMPLATE template<typename IGetFeatureRecord>
#define MODEL_GRAPH_BUILDER          model_graph_builder<IGetFeatureRecord>

MODEL_GRAPH_BUILDER_TEMPLATE
MODEL_GRAPH_BUILDER::model_graph_builder(IGetFeatureRecord& get_feature_record)
    : get_feature_record_(get_feature_record) {}

MODEL_GRAPH_BUILDER_TEMPLATE
model_graph MODEL_GRAPH_BUILDER::build(const std::vector<feature_id>& members) const {
    model_graph graph{.nodes = {}, .types = {}, .outputs = {}};
    std::unordered_map<feature_id, size_t> positions;
    for (const feature_id member : members)
        graph.outputs.push_back(place(member, graph, positions));
    return graph;
}

MODEL_GRAPH_BUILDER_TEMPLATE
size_t MODEL_GRAPH_BUILDER::place(
    feature_id feature, model_graph& graph, std::unordered_map<feature_id, size_t>& positions) const {
    const auto placed = positions.find(feature);
    if (placed != positions.end())
        return placed->second;
    const feature_record& record = get_feature_record_.record(feature);
    derivation origin = record.origin;
    derivation::application* application = std::get_if<derivation::application>(&origin.content);
    if (application == nullptr)
        return append(feature, record, origin, graph, positions);
    for (operand& arg : application->args) {
        operand::from_feature* argument = std::get_if<operand::from_feature>(&arg.source);
        if (argument == nullptr)
            continue;
        argument->id = place(argument->id, graph, positions);
    }
    return append(feature, record, origin, graph, positions);
}

MODEL_GRAPH_BUILDER_TEMPLATE
size_t MODEL_GRAPH_BUILDER::append(
    feature_id feature, const feature_record& record, const derivation& origin, model_graph& graph,
    std::unordered_map<feature_id, size_t>& positions) const {
    graph.nodes.push_back(origin);
    graph.types.push_back(record.type);
    positions.emplace(feature, graph.nodes.size() - 1);
    return graph.nodes.size() - 1;
}

#undef MODEL_GRAPH_BUILDER
#undef MODEL_GRAPH_BUILDER_TEMPLATE

}

#endif
