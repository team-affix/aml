#ifndef TREE_WALKER_HPP
#define TREE_WALKER_HPP

#include <unordered_map>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"

namespace AML {

// Names MCTS tree nodes: the child reached from a node by a choice gets a fresh id the first time it is walked.
// The root is 0. A rollout grows no tree, so walking during one records nothing and stays at the node it began from.
template<typename IIsInRollout>
struct tree_walker {
    tree_walker(IIsInRollout& is_in_rollout);
    mcts_tree_node_id walk(mcts_tree_node_id node, const mcts_choice& choice);
private:
    using map_t = std::unordered_map<mcts_choice, mcts_tree_node_id>;

    IIsInRollout& is_in_rollout_;
    std::unordered_map<mcts_tree_node_id, map_t> edges_;
    mcts_tree_node_id last_id_;
};

#define TREE_WALKER_TEMPLATE template<typename IIsInRollout>
#define TREE_WALKER          tree_walker<IIsInRollout>

TREE_WALKER_TEMPLATE
TREE_WALKER::tree_walker(IIsInRollout& is_in_rollout)
    : is_in_rollout_(is_in_rollout)
    , edges_()
    , last_id_(0) {}

TREE_WALKER_TEMPLATE
mcts_tree_node_id TREE_WALKER::walk(mcts_tree_node_id node, const mcts_choice& choice) {
    if (is_in_rollout_.is_in_rollout())
        return node;
    map_t& children = edges_[node];
    const auto found = children.find(choice);
    if (found != children.end())
        return found->second;
    ++last_id_;
    children.emplace(choice, last_id_);
    return last_id_;
}

#undef TREE_WALKER
#undef TREE_WALKER_TEMPLATE

}

#endif
