#ifndef TREE_WALKER_HPP
#define TREE_WALKER_HPP

#include <unordered_map>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"

// Names MCTS tree nodes: the child reached from a node by a choice gets a fresh id the first time it is walked.
// The root is 0.
struct tree_walker {
    tree_walker();
    mcts_tree_node_id walk(mcts_tree_node_id node, const mcts_choice& choice);
private:
    using map_t = std::unordered_map<mcts_choice, mcts_tree_node_id>;

    std::unordered_map<mcts_tree_node_id, map_t> edges_;
    mcts_tree_node_id last_id_;
};

#endif
