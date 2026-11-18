#include "value_objects/model_node_hash.hpp"

#include <functional>
#include <variant>

size_t model_node_hash::operator()(const model_node& node) const noexcept {
    size_t seed = std::hash<size_t>{}(node.content.index());
    if (const model_node::leaf* leaf = std::get_if<model_node::leaf>(&node.content))
        return hash_combine(seed, std::hash<const value*>{}(leaf->label));
    const model_node::match* match = std::get_if<model_node::match>(&node.content);
    if (match == nullptr)
        return seed;
    seed = hash_combine(seed, std::hash<const feature*>{}(match->scrutinee));
    for (const model_node* branch : match->branches)
        seed = hash_combine(seed, std::hash<const model_node*>{}(branch));
    return seed;
}

size_t model_node_hash::hash_combine(size_t seed, size_t hashed) noexcept {
    return seed ^ (hashed + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
