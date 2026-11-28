#ifndef MODEL_GRAPH_HPP
#define MODEL_GRAPH_HPP

#include <compare>
#include <cstddef>
#include <typeindex>
#include <vector>
#include "value_objects/derivation.hpp"

namespace AML {

// The features a model reads, as a graph of their own: `nodes` lists every feature of their ancestry once, each
// application's feature arguments naming earlier nodes by position and its constant arguments naming constants of the
// constant graph, `types` gives each node's type, and `outputs` names the model's features in order.
struct model_graph {
    std::vector<derivation> nodes;
    std::vector<std::type_index> types;
    std::vector<size_t> outputs;
    std::strong_ordering operator<=>(const model_graph&) const = default;
};

}

#endif
