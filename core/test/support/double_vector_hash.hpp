#ifndef DOUBLE_VECTOR_HASH_HPP
#define DOUBLE_VECTOR_HASH_HPP

#include <cstddef>
#include <vector>

// Hashes a vector of doubles by its length and each element in order.
struct double_vector_hash {
    size_t operator()(const std::vector<double>& elements) const;
};

#endif
