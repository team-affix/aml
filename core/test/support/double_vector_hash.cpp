#include "support/double_vector_hash.hpp"

#include <functional>

size_t double_vector_hash::operator()(const std::vector<double>& elements) const {
    size_t seed = std::hash<size_t>{}(elements.size());
    for (const double element : elements)
        seed ^= std::hash<double>{}(element) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
