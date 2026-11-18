#include "infrastructure/tree_walker.hpp"

tree_walker::tree_walker()
    : edges_()
    , last_id_(0) {}

mcts_tree_node_id tree_walker::walk(mcts_tree_node_id node, const mcts_choice& choice) {
    map_t& children = edges_[node];
    const auto found = children.find(choice);
    if (found != children.end())
        return found->second;
    ++last_id_;
    children.emplace(choice, last_id_);
    return last_id_;
}
