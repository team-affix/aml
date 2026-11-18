#include "infrastructure/model_node_pool.hpp"

model_node_pool::model_node_pool()
    : nodes_() {}

const model_node* model_node_pool::make_match(
    const feature* scrutinee, const std::vector<const model_node*>& branches) {
    return intern(model_node{model_node::match{.scrutinee = scrutinee, .branches = branches}});
}

const model_node* model_node_pool::make_leaf(const feature* output) {
    return intern(model_node{model_node::leaf{.output = output}});
}

const model_node* model_node_pool::make_empty() {
    return intern(model_node{model_node::empty{}});
}

const model_node* model_node_pool::intern(model_node&& node) {
    return &*nodes_.insert(std::move(node)).first;
}
