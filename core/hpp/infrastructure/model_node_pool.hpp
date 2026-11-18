#ifndef MODEL_NODE_POOL_HPP
#define MODEL_NODE_POOL_HPP

#include <unordered_set>
#include <vector>
#include "value_objects/feature.hpp"
#include "value_objects/model_node.hpp"
#include "value_objects/model_node_hash.hpp"
#include "value_objects/value.hpp"

// Interns model nodes, so equal subtrees share one pointer.
struct model_node_pool {
    model_node_pool();
    const model_node* make_match(const feature* scrutinee, const std::vector<const model_node*>& branches);
    const model_node* make_leaf(const value* label);
    const model_node* make_empty();
private:
    const model_node* intern(model_node&& node);
    std::unordered_set<model_node, model_node_hash> nodes_;
};

#endif
