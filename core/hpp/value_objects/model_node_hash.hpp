#ifndef MODEL_NODE_HASH_HPP
#define MODEL_NODE_HASH_HPP

#include <cstddef>
#include "value_objects/model_node.hpp"

struct model_node_hash {
    size_t operator()(const model_node& node) const noexcept;
private:
    static size_t hash_combine(size_t seed, size_t hashed) noexcept;
};

#endif
